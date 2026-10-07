/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 * 
 * SPDX-License-Identifier: MIT
 */

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "paraconf.h"

#include "status.h"
#include "ypath.h"

#define ERRBUF_SIZE 512

static const char* nodetype[4] = {"none", "scalar", "sequence", "mapping"};

static const char* PC_NO_PATH_STRING = "<string>";
static const char* PC_NO_PATH_FILE = "<file>";

/// a numeric locale that reads numbers as YAML writes them, whatever the locale of the program, (locale_t)0 if it could not be created
static locale_t c_numeric_locale = (locale_t)0;

static pthread_once_t c_numeric_locale_once = PTHREAD_ONCE_INIT;

static void c_numeric_locale_init(void)
{
	c_numeric_locale = newlocale(LC_NUMERIC_MASK, "C", (locale_t)0);
}

static inline void pc_path_free(const char* path)
{
	if (path != PC_NO_PATH_STRING && path != PC_NO_PATH_FILE) free((void*)path);
}

static inline PC_status_t pc_set_path(PC_tree_t tree, const char* path)
{
	size_t pathlen = strlen(path);
	char* pathcpy = malloc((pathlen + 1) * sizeof(char));
	if (!pathcpy) return PC_make_malloc_err();
	strncpy(pathcpy, path, pathlen);
	pathcpy[pathlen] = 0;
	pc_path_free(tree.pcdoc->path);
	tree.pcdoc->path = pathcpy;
	return PC_OK;
}

uint64_t PC_version()
{
	return PARACONF_VERSION;
}

static PC_tree_t pc_parse(const char* filename, yaml_parser_t* conf_parser)
{
	PC_tree_t restree = {PC_OK, NULL, NULL};

	yaml_document_t conf_doc;
	if (!yaml_parser_load(conf_parser, &conf_doc)) {
		if (conf_parser->context) {
			PC_handle_err_tree(
				PC_make_err(
					PC_INVALID_FORMAT,
					"%s:%lu:%lu: error: %s\n%s:%lu:%lu: note: %s",
					filename,
					(unsigned long)conf_parser->problem_mark.line + 1,
					(unsigned long)conf_parser->problem_mark.column + 1,
					conf_parser->problem,
					filename,
					(unsigned long)conf_parser->context_mark.line + 1,
					(unsigned long)conf_parser->context_mark.column + 1,
					conf_parser->context
				),
				err0
			);
		} else {
			PC_handle_err_tree(
				PC_make_err(
					PC_INVALID_FORMAT,
					"%s:%lu:%lu: error: %s",
					filename,
					(unsigned long)conf_parser->problem_mark.line + 1,
					(unsigned long)conf_parser->problem_mark.column + 1,
					conf_parser->problem
				),
				err0
			);
		}
	}

	restree = PC_root(&conf_doc);
	PC_handle_tree(err1);

	return restree;

err1:
	yaml_document_delete(&conf_doc);
err0:
	return restree;
}

static PC_tree_t pc_parse_file_helper(FILE* conf_file, const char* filename)
{
	PC_tree_t restree = {PC_OK, NULL, NULL};

	yaml_parser_t conf_parser;
	if (!yaml_parser_initialize(&conf_parser)) {
		PC_handle_err_tree(PC_make_err(PC_SYSTEM_ERROR, "unable to initialize yaml library"), err0);
	}

	yaml_parser_set_input_file(&conf_parser, conf_file);

	restree = pc_parse(filename, &conf_parser);
	PC_handle_tree(err1);

	yaml_parser_delete(&conf_parser);

	return restree;

err1:
	yaml_parser_delete(&conf_parser);
err0:
	return restree;
}

PC_tree_t PC_parse_path(const char* path)
{
	PC_status_t status = PC_OK;
	PC_tree_t restree = {PC_OK, NULL, NULL};

	if (!path) PC_handle_err_tree(PC_make_err(PC_INVALID_PARAMETER, "no path passed to PC_parse_path"), err0);

	FILE* conf_file = fopen(path, "rb");
	if (!conf_file) {
		char errbuf[ERRBUF_SIZE];
		strerror_r(errno, errbuf, ERRBUF_SIZE);
		PC_handle_err_tree(PC_make_err(PC_SYSTEM_ERROR, "can not open file `%s': %s", path, errbuf), err0);
	}

	restree = pc_parse_file_helper(conf_file, path);
	PC_handle_tree(err1);

	PC_handle_err(pc_set_path(restree, path), err2);

	fclose(conf_file);
	return restree;

err2:
	PC_tree_destroy(&restree);
	restree.status = status;
err1:
	fclose(conf_file);
err0:
	return restree;
}

PC_tree_t PC_parse_file(FILE* conf_file)
{
	PC_tree_t restree = {PC_OK, NULL, NULL};
	if (!conf_file) PC_handle_err_tree(PC_make_err(PC_INVALID_PARAMETER, "no file passed to PC_parse_file"), err0);

	restree = pc_parse_file_helper(conf_file, PC_NO_PATH_FILE);
	PC_handle_tree(err0);

	restree.pcdoc->path = PC_NO_PATH_FILE;

	return restree;

err0:
	return restree;
}

PC_tree_t PC_parse_string(const char* document)
{
	PC_tree_t restree = {PC_OK, NULL, NULL};
	if (!document) PC_handle_err_tree(PC_make_err(PC_INVALID_PARAMETER, "no document passed to PC_parse_string"), err0);

	yaml_parser_t conf_parser;
	if (!yaml_parser_initialize(&conf_parser)) {
		PC_handle_err_tree(PC_make_err(PC_SYSTEM_ERROR, "unable to initialize yaml library"), err0);
	}

	yaml_parser_set_input_string(&conf_parser, (const unsigned char*)document, strlen(document));

	restree = pc_parse(PC_NO_PATH_STRING, &conf_parser);

	PC_handle_tree(err1);

	yaml_parser_delete(&conf_parser);

	return restree;

err1:
	yaml_parser_delete(&conf_parser);
err0:
	return restree;
}

PC_tree_t PC_root(yaml_document_t* document)
{
	PC_tree_t restree = {PC_OK, NULL, NULL};
	if (!document) PC_handle_err_tree(PC_make_err(PC_INVALID_PARAMETER, "no document passed to PC_root"), err0);

	restree.pcdoc = malloc(sizeof(PC_document_t));
	if (!restree.pcdoc) PC_handle_err_tree(PC_make_malloc_err(), err0);

	restree.node = yaml_document_get_root_node(document);
	PC_document_t pcdoc = {*document, PC_NO_PATH_STRING};
	*restree.pcdoc = pcdoc;
	return restree;

err0:
	return restree;
}

const char* PC_path(PC_tree_t tree)
{
	if (PC_status(tree)) return 0;
	if (!tree.pcdoc) return 0;
	return tree.pcdoc->path;
}

PC_tree_t PC_get(const PC_tree_t tree, const char* index_fmt, ...)
{
	va_list ap;
	va_start(ap, index_fmt);
	PC_tree_t res = PC_vget(tree, index_fmt, ap);
	va_end(ap);
	return res;
}

PC_tree_t PC_vget(const PC_tree_t tree, const char* index_fmt, va_list va)
{
	PC_tree_t restree = tree;
	PC_handle_tree(err0);

	if (!index_fmt) PC_handle_err_tree(PC_make_err(PC_INVALID_PARAMETER, "no index passed to PC_get"), err0);

	va_list va2;
	va_copy(va2, va);
	int index_size = vsnprintf(NULL, 0, index_fmt, va2) + 1;
	va_end(va2);
	if (index_size <= 0) {
		char errbuf[ERRBUF_SIZE];
		strerror_r(errno, errbuf, ERRBUF_SIZE);
		PC_handle_err_tree(PC_make_err(PC_INVALID_PARAMETER, "Invalid formatting in PC_get `%s': %s", index_fmt, errbuf), err0);
	}
	char* index = malloc(index_size);
	if (!index) PC_handle_err_tree(PC_make_malloc_err(), err0);
	if (vsnprintf(index, index_size, index_fmt, va) < 0) {
		char errbuf[ERRBUF_SIZE];
		strerror_r(errno, errbuf, ERRBUF_SIZE);
		PC_handle_err_tree(PC_make_err(PC_INVALID_PARAMETER, "Invalid formatting in PC_get `%s': %s", index_fmt, errbuf), err1);
	}

	restree = PC_sget(tree, index);
	PC_handle_tree(err1);

	free(index);
	return restree;

err1:
	free(index);
err0:
	return restree;
}

PC_status_t PC_len(const PC_tree_t tree, int* res)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	if (!res) PC_handle_err(PC_make_err(PC_INVALID_PARAMETER, "no value passed to PC_len"), err0);

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree"), err0);
	}

	size_t len = 0;
	switch (tree.node->type) {
	case YAML_SEQUENCE_NODE: {
		len = (size_t)(tree.node->data.sequence.items.top - tree.node->data.sequence.items.start);
	} break;
	case YAML_MAPPING_NODE: {
		len = (size_t)(tree.node->data.mapping.pairs.top - tree.node->data.mapping.pairs.start);
	} break;
	case YAML_SCALAR_NODE: {
		len = tree.node->data.scalar.length;
	} break;
	default: {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Unknown yaml node type: #%d", tree.node->type), err0);
	} break;
	}
	if (len > INT_MAX) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Length %zu does not fit an int (range is: [0, %d])", len, INT_MAX), err0);
	}

	*res = (int)len;
	return status;

err0:
	return status;
}

/** Copies a number without the underscores YAML 1.1 lets separate its digits, which strtol and strtod do not read
 *
 * An underscore is dropped only after a digit, or after another dropped underscore, so that one anywhere else still makes the number invalid.
 *
 * \param text the number as written
 * \param[out] digits the number without its underscores, to free, or NULL when it has none and text can be read as it is
 * \return the status of the execution
 */
static PC_status_t pc_number_digits(const char* text, char** digits)
{
	*digits = NULL;
	if (!strchr(text, '_')) return PC_OK;

	char* copy = malloc(strlen(text) + 1);
	if (!copy) return PC_make_malloc_err();
	char* out = copy;
	int after_digit = 0;
	for (const char* in = text; *in; ++in) {
		if (*in == '_' && after_digit) continue;
		after_digit = (*in >= '0' && *in <= '9');
		*out++ = *in;
	}
	*out = 0;

	*digits = copy;
	return PC_OK;
}

PC_status_t PC_int(const PC_tree_t tree, long* res)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	if (!res) PC_handle_err(PC_make_err(PC_INVALID_PARAMETER, "no value passed to PC_int"), err0);

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree"), err0);
	}

	if (tree.node->type != YAML_SCALAR_NODE) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Expected a scalar, found %s", nodetype[tree.node->type]), err0);
	}

	if (!*tree.node->data.scalar.value) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Expected integer, found an empty string"), err0);
	}

	char* digits = NULL;
	PC_handle_err(pc_number_digits((char*)tree.node->data.scalar.value, &digits), err0);
	const char* text = digits ? digits : (char*)tree.node->data.scalar.value;

	char* endptr;
	errno = 0;
	long result;
	const char* unsigned_text = text + (text[0] == '+' || text[0] == '-');
	if (unsigned_text[0] == '0' && unsigned_text[1] == 'o') {
		// the octal of YAML 1.2, that strtol does not read
		if (unsigned_text[2] < '0' || unsigned_text[2] > '7') {
			PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Expected integer, found `%s'", (char*)tree.node->data.scalar.value), err1);
		}
		result = strtol(unsigned_text + 2, &endptr, 8);
		if (text[0] == '-') result = -result;
	} else {
		result = strtol(text, &endptr, 0);
	}
	if (errno == ERANGE) {
		PC_handle_err(
			PC_make_node_err(
				PC_INVALID_NODE_TYPE,
				tree,
				"Integer out of range: `%s' (range is: [%ld, %ld])",
				(char*)tree.node->data.scalar.value,
				LONG_MIN,
				LONG_MAX
			),
			err1
		);
	}
	if (*endptr) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Expected integer, found `%s'", (char*)tree.node->data.scalar.value), err1);
	}

	free(digits);
	*res = result;
	return status;

err1:
	free(digits);
err0:
	return status;
}

PC_status_t PC_int_range(const PC_tree_t tree, long* value, long min, long max)
{
	PC_status_t status = PC_OK;
	if (!value) PC_handle_err(PC_make_err(PC_INVALID_PARAMETER, "no value passed to PC_int"), err0);

	long result;
	PC_handle_err(PC_int(tree, &result), err0);
	if (result < min || result > max) {
		PC_handle_err(
			PC_make_node_err(
				PC_INVALID_NODE_TYPE,
				tree,
				"Integer out of range: `%s' (range is: [%ld, %ld])",
				(char*)tree.node->data.scalar.value,
				min,
				max
			),
			err0
		);
	}

	*value = result;
	return status;

err0:
	return status;
}

PC_status_t PC_double(const PC_tree_t tree, double* value)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	if (!value) PC_handle_err(PC_make_err(PC_INVALID_PARAMETER, "no value passed to PC_double"), err0);

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree"), err0);
	}

	if (tree.node->type != YAML_SCALAR_NODE) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Expected a scalar, found %s", nodetype[tree.node->type]), err0);
	}

	if (!*tree.node->data.scalar.value) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Expected floating point, found an empty string"), err0);
	}

	// the infinities and not-a-number of YAML, that strtod does not read
	const char* yaml_value = (char*)tree.node->data.scalar.value;
	const char* unsigned_value = yaml_value + (yaml_value[0] == '+' || yaml_value[0] == '-');
	if (!strcmp(unsigned_value, ".inf") || !strcmp(unsigned_value, ".Inf") || !strcmp(unsigned_value, ".INF")) {
		*value = yaml_value[0] == '-' ? -INFINITY : INFINITY;
		return status;
	}
	if (!strcmp(yaml_value, ".nan") || !strcmp(yaml_value, ".NaN") || !strcmp(yaml_value, ".NAN")) {
		*value = NAN;
		return status;
	}

	char* digits = NULL;
	PC_handle_err(pc_number_digits(yaml_value, &digits), err0);

	// strtod follows the locale of the calling thread, so it reads in the C one
	pthread_once(&c_numeric_locale_once, c_numeric_locale_init);
	if (c_numeric_locale == (locale_t)0) {
		PC_handle_err(PC_make_malloc_err(), err1);
	}
	locale_t thread_locale = uselocale(c_numeric_locale);
	char* endptr;
	errno = 0;
	double result = strtod(digits ? digits : yaml_value, &endptr);
	int strtod_errno = errno;
	uselocale(thread_locale);
	if (strtod_errno == ERANGE) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Floating point out of range: `%s'", yaml_value), err1);
	}
	if (*endptr) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Expected floating point, found `%s'", yaml_value), err1);
	}

	free(digits);
	*value = result;
	return status;

err1:
	free(digits);
err0:
	return status;
}

PC_status_t PC_string(const PC_tree_t tree, char** value)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	if (!value) PC_handle_err(PC_make_err(PC_INVALID_PARAMETER, "no value passed to PC_string"), err0);

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree"), err0);
	}

	if (tree.node->type != YAML_SCALAR_NODE) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Expected a scalar, found %s", nodetype[tree.node->type]), err0);
	}

	int len = 0;
	PC_handle_err(PC_len(tree, &len), err0);

	char* result_value = malloc(len + 1);
	if (!result_value) PC_handle_err(PC_make_malloc_err(), err0);
	strncpy(result_value, (char*)tree.node->data.scalar.value, len + 1);
	assert(result_value[len] == 0);

	*value = result_value;

	return status;

err0:
	return status;
}

#define PARACONF_PC_BOOL_NOT_A_BOOLEAN(free_stamp)                                                                                                   \
	PC_handle_err(                                                                                                                                   \
		PC_make_node_err(                                                                                                                            \
			PC_INVALID_NODE_TYPE,                                                                                                                    \
			tree,                                                                                                                                    \
			"expected a boolean (y|Y|yes|Yes|YES|n|N|no|No|NO|true|True|TRUE|false|False|FALSE|on|On|ON|off|Off|OFF), but got `%s'",                 \
			strval                                                                                                                                   \
		),                                                                                                                                           \
		free_stamp                                                                                                                                   \
	)

PC_status_t PC_bool(const PC_tree_t tree, int* res)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	if (!res) PC_handle_err(PC_make_err(PC_INVALID_PARAMETER, "no value passed to PC_bool"), err0);

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree"), err0);
	}

	if (tree.node->type != YAML_SCALAR_NODE) {
		PC_handle_err(PC_make_node_err(PC_INVALID_NODE_TYPE, tree, "Expected a scalar, found %s", nodetype[tree.node->type]), err0);
	}

	char* strval = (char*)tree.node->data.scalar.value;

	switch (strval[0]) {
	case 'y':
		switch (strval[1]) {
		case 0:
			*res = 1;
			break;
		case 'e':
			if (strval[2] == 's' && strval[3] == 0) {
				*res = 1;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		default:
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	case 'Y':
		switch (strval[1]) {
		case 0:
			*res = 1;
			break;
		case 'e':
			if (strval[2] == 's' && strval[3] == 0) {
				*res = 1;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		case 'E':
			if (strval[2] == 'S' && strval[3] == 0) {
				*res = 1;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		default:
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	case 'n':
		switch (strval[1]) {
		case 0:
			*res = 0;
			break;
		case 'o':
			if (strval[2] == 0) {
				*res = 0;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		default:
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	case 'N':
		switch (strval[1]) {
		case 0:
			*res = 0;
			break;
		case 'o': // same as 'O'
		case 'O':
			if (strval[2] == 0) {
				*res = 0;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		default:
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	case 't':
		if (!strcmp(strval + 1, "rue")) {
			*res = 1;
		} else {
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	case 'T':
		switch (strval[1]) {
		case 'r':
			if (!strcmp(strval + 2, "ue")) {
				*res = 1;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		case 'R':
			if (!strcmp(strval + 2, "UE")) {
				*res = 1;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		default:
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	case 'f':
		if (!strcmp(strval + 1, "alse")) {
			*res = 0;
		} else {
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	case 'F':
		switch (strval[1]) {
		case 'a':
			if (!strcmp(strval + 2, "lse")) {
				*res = 0;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		case 'A':
			if (!strcmp(strval + 2, "LSE")) {
				*res = 0;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		default:
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	case 'o':
		switch (strval[1]) {
		case 'n':
			if (strval[2] == 0) {
				*res = 1;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		case 'f':
			if (strval[2] == 'f' && strval[3] == 0) {
				*res = 0;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		default:
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	case 'O':
		switch (strval[1]) {
		case 'n': // same as 'N'
		case 'N':
			if (strval[2] == 0) {
				*res = 1;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		case 'f':
			if (strval[2] == 'f' && strval[3] == 0) {
				*res = 0;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		case 'F':
			if (strval[2] == 'F' && strval[3] == 0) {
				*res = 0;
			} else {
				PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
			}
			break;
		default:
			PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
		}
		break;
	default:
		PARACONF_PC_BOOL_NOT_A_BOOLEAN(err0);
	}

	return status;

err0:
	return status;
}

PC_status_t PC_tree_destroy(PC_tree_t* tree)
{
	PC_status_t status = PC_OK;

	if (!tree) PC_handle_err(PC_make_err(PC_INVALID_PARAMETER, "no tree passed to PC_tree_destroy"), err0);
	if (tree->pcdoc) {
		yaml_document_delete(&tree->pcdoc->document);
		pc_path_free(tree->pcdoc->path);
		tree->pcdoc->path = NULL;
		free(tree->pcdoc);
		tree->pcdoc = NULL;
	}
	tree->node = NULL;
	tree->status = PC_INVALID_PARAMETER;

	return status;

err0:
	return status;
}
