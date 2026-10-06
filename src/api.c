/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 * 
 * SPDX-License-Identifier: MIT
 */

#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "paraconf.h"

#include "status.h"
#include "ypath.h"

#define ERRBUF_SIZE 512

static const char* nodetype[4] = {"none", "scalar", "sequence", "mapping"};

static const char* PC_NO_PATH = "<string>";

static inline void pc_path_free(const char* path)
{
	if (path != PC_NO_PATH) free((void*)path);
}

static inline void pc_set_path(PC_tree_t tree, const char* path)
{
	pc_path_free(tree.pcdoc->path);
	size_t pathlen = strlen(path);
	char* pathcpy = malloc((pathlen + 1) * sizeof(char));
	strncpy(pathcpy, path, pathlen);
	pathcpy[pathlen] = 0;
	tree.pcdoc->path = pathcpy;
}

uint64_t PC_version()
{
	return PARACONF_VERSION;
}

PC_tree_t PC_parse_path(const char* path)
{
	PC_tree_t restree = {PC_OK, NULL, NULL};

	FILE* conf_file = fopen(path, "rb");
	if (!conf_file) {
		char errbuf[ERRBUF_SIZE];
		strerror_r(errno, errbuf, ERRBUF_SIZE);
		PC_handle_err_tree(PC_make_err(PC_SYSTEM_ERROR, errbuf), err0);
	}

	PC_errhandler_t handler = PC_errhandler(PC_NULL_HANDLER); // aka PC_try
	restree = PC_parse_file(conf_file);
	PC_errhandler(handler);
	if (PC_status(restree)) { // aka PC_catch
		PC_handle_err_tree(PC_make_err(restree.status, "can not parse file `%s`\n%s", path, PC_errmsg()), err1);
	}

	fclose(conf_file);
	pc_set_path(restree, path);
	return restree;

err1:
	fclose(conf_file);
err0:
	return restree;
}

PC_tree_t PC_parse_string(const char* document)
{
	PC_tree_t restree = {PC_OK, NULL, NULL};

	yaml_parser_t conf_parser;
	if (!yaml_parser_initialize(&conf_parser)) {
		PC_handle_err_tree(PC_make_err(PC_SYSTEM_ERROR, "unable to load yaml library"), err0);
	}

	yaml_parser_set_input_string(&conf_parser, (const unsigned char*)document, strlen(document));

	yaml_document_t conf_doc;
	if (!yaml_parser_load(&conf_parser, &conf_doc)) {
		if (conf_parser.context) {
			PC_handle_err_tree(
				PC_make_err(
					PC_INVALID_FORMAT,
					"%s\n  line %lu, column %lu\n%s\n  line %lu, column %lu",
					conf_parser.context,
					(unsigned long)conf_parser.context_mark.line + 1,
					(unsigned long)conf_parser.context_mark.column + 1,
					conf_parser.problem,
					(unsigned long)conf_parser.problem_mark.line + 1,
					(unsigned long)conf_parser.problem_mark.column + 1
				),
				err1
			);
		} else {
			PC_handle_err_tree(
				PC_make_err(
					PC_INVALID_FORMAT,
					"%s\n  line %lu, column %lu",
					conf_parser.problem,
					(unsigned long)conf_parser.problem_mark.line + 1,
					(unsigned long)conf_parser.problem_mark.column + 1
				),
				err1
			);
		}
	}

	yaml_parser_delete(&conf_parser);

	restree = PC_root(&conf_doc);

	PC_handle_tree(err0);

	return restree;
err1:
	yaml_parser_delete(&conf_parser);
err0:
	return restree;
}

PC_tree_t PC_parse_file(FILE* conf_file)
{
	PC_tree_t restree = {PC_OK, NULL, NULL};

	yaml_parser_t conf_parser;
	if (!yaml_parser_initialize(&conf_parser)) {
		PC_handle_err_tree(PC_make_err(PC_SYSTEM_ERROR, "unable to load yaml library"), err0);
	}

	yaml_parser_set_input_file(&conf_parser, conf_file);

	yaml_document_t conf_doc;
	if (!yaml_parser_load(&conf_parser, &conf_doc)) {
		if (conf_parser.context) {
			PC_handle_err_tree(
				PC_make_err(
					PC_INVALID_FORMAT,
					"%lu:%lu: Error: %s \n%lu:%lu: Error: %s",
					(unsigned long)conf_parser.problem_mark.line,
					(unsigned long)conf_parser.problem_mark.column,
					conf_parser.problem,
					(unsigned long)conf_parser.context_mark.line,
					(unsigned long)conf_parser.context_mark.column,
					conf_parser.context
				),
				err1
			);
		} else {
			PC_handle_err_tree(
				PC_make_err(
					PC_INVALID_FORMAT,
					"%lu:%lu: Error: %s",
					(unsigned long)conf_parser.problem_mark.line,
					(unsigned long)conf_parser.problem_mark.column,
					conf_parser.problem
				),
				err1
			);
		}
	}

	yaml_parser_delete(&conf_parser);

	restree = PC_root(&conf_doc);

	PC_handle_tree(err0);

	return restree;
err1:
	yaml_parser_delete(&conf_parser);
err0:
	return restree;
}

PC_tree_t PC_root(yaml_document_t* document)
{
	PC_tree_t restree = {PC_OK, malloc(sizeof(PC_document_t)), yaml_document_get_root_node(document)};
	PC_document_t pcdoc = {*document, PC_NO_PATH};
	*restree.pcdoc = pcdoc;
	return restree;
}

const char* PC_path(PC_tree_t tree)
{
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

	va_list va2;
	va_copy(va2, va);
	int index_size = vsnprintf(NULL, 0, index_fmt, va2) + 1;
	va_end(va2);
	char* index = malloc(index_size);
	vsnprintf(index, index_size, index_fmt, va);

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

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree\n"), err0);
	}

	switch (tree.node->type) {
	case YAML_SEQUENCE_NODE: {
		*res = tree.node->data.sequence.items.top - tree.node->data.sequence.items.start;
	} break;
	case YAML_MAPPING_NODE: {
		*res = tree.node->data.mapping.pairs.top - tree.node->data.mapping.pairs.start;
	} break;
	case YAML_SCALAR_NODE: {
		*res = tree.node->data.scalar.length;
	} break;
	default: {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Unknown yaml node type: #%d", tree.node->type), err0);
	} break;
	}

	return status;

err0:
	return status;
}

PC_status_t PC_int(const PC_tree_t tree, long* res)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree\n"), err0);
	}

	if (tree.node->type != YAML_SCALAR_NODE) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]), err0);
	}

	if (!*tree.node->data.scalar.value) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected integer, found an empty string\n"), err0);
	}

	char* endptr;
	errno = 0;
	long result = strtol((char*)tree.node->data.scalar.value, &endptr, 0);
	if (errno == ERANGE) {
		PC_handle_err(
			PC_make_err(
				PC_INVALID_NODE_TYPE,
				"Integer out of range: `%s' (range is: [%ld, %ld])\n",
				(char*)tree.node->data.scalar.value,
				LONG_MIN,
				LONG_MAX
			),
			err0
		);
	}
	if (*endptr) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected integer, found `%s'\n", (char*)tree.node->data.scalar.value), err0);
	}

	*res = result;
	return status;

err0:
	return status;
}

PC_status_t PC_double(const PC_tree_t tree, double* value)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree\n"), err0);
	}

	if (tree.node->type != YAML_SCALAR_NODE) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]), err0);
	}

	if (!*tree.node->data.scalar.value) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected floating point, found an empty string\n"), err0);
	}

	char* endptr;
	errno = 0;
	*value = strtod((char*)tree.node->data.scalar.value, &endptr);
	if (errno == ERANGE) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Floating point out of range: `%s'\n", (char*)tree.node->data.scalar.value), err0);
	}
	if (*endptr) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected floating point, found `%s'\n", (char*)tree.node->data.scalar.value), err0);
	}

	return status;

err0:
	return status;
}

PC_status_t PC_string(const PC_tree_t tree, char** value)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree\n"), err0);
	}

	if (tree.node->type != YAML_SCALAR_NODE) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]), err0);
	}

	int len = 0;
	PC_handle_err(PC_len(tree, &len), err0);

	*value = malloc(len + 1);
	strncpy(*value, (char*)tree.node->data.scalar.value, len + 1);
	assert((*value)[len] == 0);

	return status;

err0:
	return status;
}

#define PARACONF_PC_BOOL_NOT_A_BOOLEAN(free_stamp)                                                                                                   \
	PC_handle_err(                                                                                                                                   \
		PC_make_err(                                                                                                                                 \
			PC_INVALID_NODE_TYPE,                                                                                                                    \
			"expected a boolean (y|Y|yes|Yes|YES|n|N|no|No|NO|true|True|TRUE|false|False|FALSE|on|On|ON|off|Off|OFF), but got `%s'\n",               \
			strval                                                                                                                                   \
		),                                                                                                                                           \
		free_stamp                                                                                                                                   \
	)

PC_status_t PC_bool(const PC_tree_t tree, int* res)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	// check type
	if (!tree.node) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected node, found empty tree\n"), err0);
	}

	if (tree.node->type != YAML_SCALAR_NODE) {
		PC_handle_err(PC_make_err(PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]), err0);
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
