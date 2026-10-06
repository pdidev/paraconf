/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 * 
 * SPDX-License-Identifier: MIT
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "paraconf.h"

#include "status.h"

// file private stuff

#define ERRBUF_SIZE 512

typedef struct errctx_s {
	PC_errhandler_t handler;

	char* buffer;

} errctx_t;

static pthread_key_t context_key;

static pthread_once_t context_key_once = PTHREAD_ONCE_INIT;

/// whether context_key could be created
static int context_key_valid = 0;

static void assert_status(PC_status_t status, const char* message, void* context)
{
	(void)context; // prevent unused warning
	if (status) {
		fprintf(stderr, "Error in paraconf: %s\n", message);
		abort();
	}
}

/// the message reported when memory can not be allocated, preallocated since there would be no memory to allocate it then
static char malloc_errmsg[] = "unable to allocate memory";

/** The context used by a thread whose own context could not be allocated
 *
 * It is shared by all such threads, so it is never modified: it keeps the default handler and the message that explains why.
 */
static errctx_t malloc_err_context = {{assert_status, NULL}, malloc_errmsg};

static void buffer_free(char* buffer)
{
	if (buffer != malloc_errmsg) free(buffer);
}

/**
 * \param context taken as a void* but in fact a errctx_t*
 */
static void context_destroy(void* context)
{
	buffer_free(((errctx_t*)context)->buffer);
	free(context);
}

static void context_init()
{
	context_key_valid = !pthread_key_create(&context_key, context_destroy);
}

static errctx_t* get_context()
{
	pthread_once(&context_key_once, &context_init);
	if (!context_key_valid) return &malloc_err_context;

	errctx_t* context = pthread_getspecific(context_key);
	if (!context) {
		context = malloc(sizeof(errctx_t));
		if (!context) return &malloc_err_context;
		context->buffer = NULL;
		context->handler = PC_ASSERT_HANDLER;
		if (pthread_setspecific(context_key, context)) {
			free(context);
			return &malloc_err_context;
		}
	}

	return context;
}

// library private stuff

PC_status_t PC_make_err(PC_status_t status, const char* message, ...)
{
	va_list ap;

	errctx_t* ctx = get_context();
	if (ctx == &malloc_err_context) return PC_make_malloc_err();

	va_start(ap, message);
	int buffer_size = vsnprintf(NULL, 0, message, ap) + 1;
	va_end(ap);
	if (buffer_size <= 0) {
		char errbuf[ERRBUF_SIZE];
		strerror_r(errno, errbuf, ERRBUF_SIZE);
		PC_handle_err(PC_make_err(PC_SYSTEM_ERROR, "Could not handle error message: %s", errbuf), err0);
	}
	char* buffer = malloc(buffer_size);
	if (!buffer) return PC_make_malloc_err();
	va_start(ap, message);
	buffer_size = vsnprintf(buffer, buffer_size, message, ap) + 1;
	va_end(ap);
	if (buffer_size <= 0) {
		char errbuf[ERRBUF_SIZE];
		strerror_r(errno, errbuf, ERRBUF_SIZE);
		PC_handle_err(PC_make_err(PC_SYSTEM_ERROR, "Could not handle error message: %s", errbuf), err1);
	}
	buffer_free(ctx->buffer); // only now, since it might be used as one of the va_args
	ctx->buffer = buffer;
	if (ctx->handler.func) ctx->handler.func(status, ctx->buffer, ctx->handler.context);
	return status;

err1:
	free(buffer);
err0:
	return status;
}

PC_status_t PC_make_malloc_err()
{
	errctx_t* ctx = get_context();

	if (ctx->buffer != malloc_errmsg) { // never true for malloc_err_context, that must not be modified
		buffer_free(ctx->buffer);
		ctx->buffer = malloc_errmsg;
	}
	if (ctx->handler.func) ctx->handler.func(PC_SYSTEM_ERROR, ctx->buffer, ctx->handler.context);
	return PC_SYSTEM_ERROR;
}

// public stuff

const PC_errhandler_t PC_ASSERT_HANDLER = {assert_status, NULL};

const PC_errhandler_t PC_NULL_HANDLER = {NULL, NULL};

PC_errhandler_t PC_errhandler(PC_errhandler_t new_handler)
{
	errctx_t* ctx = get_context();
	PC_errhandler_t old_handler = ctx->handler;
	if (ctx == &malloc_err_context) {
		PC_make_malloc_err(); // no new handler can be recorded, so the default one reports it
	} else {
		ctx->handler = new_handler;
	}
	return old_handler;
}

char* PC_errmsg()
{
	return get_context()->buffer;
}
