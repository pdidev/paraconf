/*******************************************************************************
 * Copyright (c) 2015, Julien Bigot - CEA (julien.bigot@cea.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of CEA nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific 
 *   prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "paraconf.h"

const char *PC_errmessage[5] = {
	"No error",
	"Invalid parameter",
	"Invalid node type",
	"node not found"
};

#define PC_BUFFER_SIZE 256

static int strlzcmp(const char *lstr, const char *zstr, size_t lstr_size)
{
	int res = strncmp(lstr, zstr, lstr_size);
	if ( res ) return res;
	if ( zstr[lstr_size] ) return -1;
	return res;
}

static void verror(PC_status_t *status, PC_errcode_t code, const char *message, va_list va)
{
	if ( status ) {
		status->code = code;
		int bufsize = PC_BUFFER_SIZE;
		status->errmsg = malloc(bufsize);
		while ( vsnprintf(status->errmsg, bufsize, message, va) >= bufsize ) {
			bufsize *= 2;
			status->errmsg = realloc(status->errmsg, bufsize);
		}
	} else {
		vfprintf(stderr, message, va);
		fprintf(stderr, "\n");
		abort();
	}
}

static void error(PC_status_t *status, PC_errcode_t code, const char *message, ...)
{
	va_list ap;
	va_start(ap, message);
	verror(status, code, message, ap);
	va_end(ap);
}

static const char *nodetype[4] = {
	"none",
	"scalar",
	"sequence",
	"mapping"
};

static PC_tree_t subtree(PC_tree_t tree, int key)
{
	PC_tree_t result = { tree.status, tree.errfunc, tree.document, yaml_document_get_node(tree.document, key) };
	assert(result.node);
	return result;
}

void PC_assert(PC_status_t status)
{
	if ( status.code ) {
		fprintf(stderr, "%s in paraconf: %s\n", PC_errmessage[status.code], status.errmsg);
		abort();
	}
}

static PC_tree_t PC_sget(PC_tree_t tree, const char *index)
{
	PC_tree_t result = tree;
	tree.errfunc = NULL; // Don't handle errors below, we'll do it
	const char *full_index = index;
	
	for(;;) {
		switch ( *index ) {
		case '[': {
			if ( result.node->type != YAML_SEQUENCE_NODE ) {
				error(&result.status, PC_INVALID_NODE_TYPE, "Expected sequence, found %s: `%.*s'\n",
						nodetype[result.node->type],
						(int)(index-full_index)-1,
						full_index);
				goto err1;
			}
			++index; // consume the starting '['
			char *post_index;
			long seq_idx = strtol(index, &post_index, 0);
			if ( post_index == index ) {
				error(&result.status, PC_INVALID_PARAMETER, "Expected integer, found `%s' at %td of `%s'\n", index, index-full_index, full_index);
				goto err1;
			}
			index = post_index;
			if ( seq_idx > result.node->data.sequence.items.top - result.node->data.sequence.items.start ) {
				error(&result.status, PC_NODE_NOT_FOUND, "Index %ld out of range in `%.*s'\n",
						seq_idx,
						(int)(index-full_index)-1,
						full_index);
				goto err1;
			}
			result.node = yaml_document_get_node(tree.document, *(result.node->data.sequence.items.start + seq_idx));
			assert(result.node);
			if ( *index != ']' ) {
				error(&result.status, PC_INVALID_PARAMETER, "Expected closing square bracket, found %c at %td of `%s'\n", *index, index-full_index, full_index);
				goto err1;
			}
			++index;
		}; break;
		case '.': {
			if ( result.node->type != YAML_MAPPING_NODE ) {
				error(&result.status, PC_INVALID_NODE_TYPE, "Expected mapping, found %s: `%.*s'\n",
						nodetype[result.node->type],
						(int)(index-full_index)-1,
						full_index);
				goto err1;
			}
			++index; // consume the starting '.'
			int id_len = 0;
			while ( index[id_len] && index[id_len] != '.' && index[id_len] != '[' && index[id_len] != '{' && index[id_len] != '<' ) ++id_len;
			yaml_node_pair_t *pair = result.node->data.mapping.pairs.start;
			while ( pair != result.node->data.mapping.pairs.top ) {
				char *key_str; result.status = PC_string(subtree(tree, pair->key), &key_str);
				if ( PC_status(result) ) goto err1;
				int cmp = strlzcmp(index, key_str, id_len);
				free(key_str);
				if ( !cmp ) break;
				++pair;
			}
			if ( pair == result.node->data.mapping.pairs.top ) {
				error(&result.status, PC_NODE_NOT_FOUND, "Key not found: %.*s in `%.*s'\n",
						id_len,
						index,
						(int)(index-full_index)-1,
						full_index);
				goto err1;
			}
			index += id_len;
			result.node = yaml_document_get_node(tree.document, pair->value);
			assert(result.node);
		}; break;
		case '{': {
			if ( result.node->type != YAML_MAPPING_NODE ) {
				error(&result.status, PC_INVALID_NODE_TYPE, "Expected mapping, found %s: `%.*s'\n",
						nodetype[result.node->type],
						(int)(index-full_index)-1,
						full_index);
				goto err1;
			}
			++index; // consume the starting '{'
			char *post_index;
			long map_idx = strtol(index, &post_index, 0);
			if ( post_index == index ) {
				error(&result.status, PC_INVALID_PARAMETER, "Expected integer, found `%s' at %td of `%s'\n", index, index-full_index, full_index);
				goto err1;
			}
			index = post_index;
			if ( map_idx > result.node->data.mapping.pairs.top - result.node->data.mapping.pairs.start ) {
				error(&result.status, PC_NODE_NOT_FOUND, "Index %ld out of range in `%.*s'\n",
						map_idx,
						(int)(index-full_index)-1,
						full_index);
				goto err1;
			}
			result.node = yaml_document_get_node(tree.document, (result.node->data.mapping.pairs.start + map_idx)->key);
			assert(result.node);
			if ( *index != '}' ) {
				error(&result.status, PC_INVALID_PARAMETER, "Expected closing curly bracket, found %c at %td of `%s'\n", *index, index-full_index, full_index);
				goto err1;
			}
			++index;
		}; break;
		case '<': {
			if ( result.node->type != YAML_MAPPING_NODE ) {
				error(&result.status, PC_INVALID_NODE_TYPE, "Expected mapping, found `%.*s': %s\n",
						nodetype[result.node->type],
						(int)(index-full_index)-1,
						full_index);
				goto err1;
			}
			++index; // consume the starting '<'
			char *post_index;
			const char *map_idx_str = index;
			long map_idx = strtol(index, &post_index, 0);
			if ( post_index == index ) {
				error(&result.status, PC_INVALID_PARAMETER, "Expected integer, found `%s' at %td of `%s'\n", index, index-full_index, full_index);
				goto err1;
			}
			index = post_index;
			if ( map_idx < 0 || map_idx > result.node->data.mapping.pairs.top - result.node->data.mapping.pairs.start ) {
				error(&result.status, PC_NODE_NOT_FOUND, "Index %ld out of range in `%.*s'\n",
						map_idx,
						(int)(map_idx_str-full_index)-1,
						full_index);
				goto err1;
			}
			result.node = yaml_document_get_node(tree.document, (result.node->data.mapping.pairs.start + map_idx)->value);
			assert(result.node);
			if ( *index != '>' ) {
				error(&result.status, PC_INVALID_PARAMETER, "Expected closing angle bracket, found %c at %td of `%s'\n", *index, index-full_index, full_index);
				goto err1;
			}
			++index;
		}; break;
		case 0: {
			assert(result.node);
			goto err1;
		}
		default: {
			error(&result.status, PC_INVALID_PARAMETER, "Invalid character %c at %td of `%s'\n", *index, index-full_index, full_index);
			goto err1;
		};
		}
	}
err1:
	if ( result.status.code && result.errfunc ) result.errfunc(result.status);
	return result;
}


PC_tree_t PC_root(yaml_document_t *document, PC_errfunc_f errfunc)
{
	PC_status_t status = { PC_OK, NULL };
	PC_tree_t res = { status, errfunc, document, yaml_document_get_root_node(document) };
	return res;
}

PC_tree_t PC_get(PC_tree_t tree, const char *index_fmt, ...)
{
	va_list ap;
	va_start(ap, index_fmt);
	PC_tree_t res = PC_vget(tree, index_fmt, ap);
	va_end(ap);
	return res;
}

PC_tree_t PC_vget(PC_tree_t tree, const char *index_fmt, va_list va)
{
	if ( tree.status.code ) return tree;
	
	int index_size = PC_BUFFER_SIZE;
	char *index = malloc(index_size);
	while ( vsnprintf(index, index_size, index_fmt, va) > index_size ) {
		index_size *= 2;
		index = realloc(index, index_size);
	}
	
	PC_tree_t res = PC_sget(tree, index);
	
	free(index);
	return res;
}

PC_status_t PC_len(PC_tree_t tree, int *res)
{
	if ( tree.status.code ) return tree.status;
	
	switch ( tree.node->type ) {
	case YAML_SEQUENCE_NODE: {
		*res = tree.node->data.sequence.items.top - tree.node->data.sequence.items.start;
	} break;
	case YAML_MAPPING_NODE: {
		*res = tree.node->data.mapping.pairs.top - tree.node->data.mapping.pairs.start;
	} break;
	case YAML_SCALAR_NODE: {
		*res = tree.node->data.scalar.length;
	} break;
	}
	// the above cases should be exhaustive
	return tree.status;
}

PC_status_t PC_int(PC_tree_t tree, int *res)
{
	if ( tree.status.code ) return tree.status;
	
	if ( tree.node->type != YAML_SCALAR_NODE ) {
		fprintf(stderr, "PC_int: bad type %s\n", tree.status.errmsg);
		error(&tree.status, PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]);
		return tree.status;
	}
	char *endptr;
	long result = strtol((char*)tree.node->data.scalar.value, &endptr, 0);
	if ( *endptr ) {
		char *content; tree.status = PC_string(tree, &content);
// 		error(&tree.status, PC_INVALID_NODE_TYPE, "Expected integer, found `%s'\n", content);
		free(content);
		return tree.status;
	}
	*res = result;
	return tree.status;
}

PC_status_t PC_double(PC_tree_t tree, double* value)
{
	if ( tree.status.code ) return tree.status;
	
	if ( tree.node->type != YAML_SCALAR_NODE ) {
		error(&tree.status, PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]);
	}
	char *endptr;
	double result = strtod((char*)tree.node->data.scalar.value, &endptr);
	if ( *endptr ) {
		char *content; tree.status = PC_string(tree, &content);
		error(&tree.status, PC_INVALID_PARAMETER, "Expected floating point, found `%s'\n", content);
		free(content);
	}
	return tree.status;
}

PC_status_t PC_string(PC_tree_t tree, char** value)
{
	if ( tree.status.code ) return tree.status;
	
	if ( tree.node->type != YAML_SCALAR_NODE ) {
		error(&tree.status, PC_INVALID_NODE_TYPE, "Expected a scalar, found %s\n", nodetype[tree.node->type]);
	}
	
	int len; tree.status = PC_len(tree, &len); if (tree.status.code) return tree.status;
	*value = malloc(len+1);
	
	strncpy(*value, (char*)tree.node->data.scalar.value, len+1);
	assert((*value)[len]==0);
	
	return tree.status;
}

PC_status_t PC_broadcast(yaml_document_t* document, int count, int root, MPI_Comm comm)
{
	yaml_emitter_t emitter;
	yaml_emitter_initialize(&emitter);
	yaml_emitter_set_width(&emitter, -1);
	yaml_emitter_set_canonical(&emitter, 1);
	yaml_emitter_open(&emitter);
	
	size_t buf_size = PC_BUFFER_SIZE/2;
	unsigned char *buf = 0;
	int err = YAML_WRITER_ERROR;
	unsigned long data_size;
	while ( err == YAML_WRITER_ERROR ) {
		buf_size *= 2;
		buf = realloc(buf, buf_size);
		yaml_emitter_set_output_string(&emitter, buf, buf_size, &data_size);
		err = yaml_emitter_dump(&emitter, document);
	}
	yaml_emitter_close(&emitter);
	yaml_emitter_delete(&emitter);
	
	MPI_Bcast(&data_size, 1, MPI_LONG, root, comm);
	MPI_Bcast(buf, data_size, MPI_LONG, root, comm);
	
	int rank; MPI_Comm_rank(comm, &rank);
	if ( rank != root ) {
		yaml_parser_t parser;
		yaml_parser_initialize(&parser);
		yaml_parser_set_input_string(&parser, buf, data_size);
		yaml_parser_load(&parser, document);
	}
	PC_status_t res = { PC_OK, NULL };
	return res;
}
