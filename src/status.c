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
#include "ypath.h"

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

/** Builds the message of an error, led by a position when one is given, and reports it
 *
 * \param status the status of the error
 * \param path the name of the document, NULL for an error that has no position
 * \param line the line of the position, counted from 1
 * \param column the column of the position, counted from 1
 * \param message the printf-style format of the message
 * \param ap the values of the format
 * \return the status of the error, PC_SYSTEM_ERROR if the message could not be built
 */
static PC_status_t make_err(PC_status_t status, const char* path, unsigned long line, unsigned long column, const char* message, va_list ap)
{
	errctx_t* ctx = get_context();
	if (ctx == &malloc_err_context) return PC_make_malloc_err();

	int prefix_size = path ? snprintf(NULL, 0, "%s:%lu:%lu: error: ", path, line, column) : 0;
	va_list ap2;
	va_copy(ap2, ap);
	int message_size = vsnprintf(NULL, 0, message, ap2);
	va_end(ap2);
	if (prefix_size < 0 || message_size < 0) {
		char errbuf[ERRBUF_SIZE];
		strerror_r(errno, errbuf, ERRBUF_SIZE);
		PC_handle_err(PC_make_err(PC_SYSTEM_ERROR, "Could not handle error message: %s", errbuf), err0);
	}
	char* buffer = malloc(prefix_size + message_size + 1);
	if (!buffer) return PC_make_malloc_err();
	if (path) snprintf(buffer, prefix_size + 1, "%s:%lu:%lu: error: ", path, line, column);
	if (vsnprintf(buffer + prefix_size, message_size + 1, message, ap) < 0) {
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

PC_status_t PC_make_err(PC_status_t status, const char* message, ...)
{
	va_list ap;
	va_start(ap, message);
	status = make_err(status, NULL, 0, 0, message, ap);
	va_end(ap);
	return status;
}

PC_status_t PC_make_node_err(PC_status_t status, PC_tree_t tree, const char* message, ...)
{
	const char* path = NULL;
	unsigned long line = 0, column = 0;
	if (tree.pcdoc && tree.node) {
		path = tree.pcdoc->path;
		line = (unsigned long)tree.node->start_mark.line + 1;
		column = (unsigned long)tree.node->start_mark.column + 1;
	}
	va_list ap;
	va_start(ap, message);
	status = make_err(status, path, line, column, message, ap);
	va_end(ap);
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
