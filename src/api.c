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

#include "ytype.h"

#include "paraconf.h"

#define PC_BUFFER_SIZE 256

static char *msprintf(const char *fmt, va_list ap)
{
	int index_size = PC_BUFFER_SIZE;
	char *index = malloc(index_size);
	while ( vsnprintf(index, index_size, fmt, ap) > index_size ) {
		index_size *= 2;
		index = realloc(index, PC_BUFFER_SIZE);
	}
	return index;
}

PC_status_t PC_get(PC_tree_t tree, const char* index, PC_tree_t* value, ...)
{
	va_list ap;
	va_start(ap, value);
	PC_status_t res = PC_vget(tree, index, value, ap);
	va_end(ap);
	return res;
}

PC_status_t PC_vget(PC_tree_t tree, const char* index_fmt, PC_tree_t* value, va_list va)
{
	yaml_node_t* result;
	if ( tree.node ) {
		result = tree.node;
	} else {
		result = yaml_document_get_root_node(tree.document);
	}
	assert(result);
	
	PC_status_t err = PC_OK;
	
	char *index = msprintf(index_fmt, va);
	char *index_free = index;
	
	for(;;) {
		switch ( *index ) {
		case '[': {
			if ( result->type != YAML_SEQUENCE_NODE ) {
				err = PC_INVALID_NODE_TYPE;
				goto vget_free;
			}
			++index; // consume the starting '['
			char *post_index;
			long seq_idx = strtol(index, &post_index, 0);
			if ( post_index == index ) {
				err = PC_INVALID_PARAMETER;
				goto vget_free;
			}
			index = post_index;
			if ( seq_idx > result->data.sequence.items.top - result->data.sequence.items.start ) {
				err = PC_INVALID_PARAMETER;
				goto vget_free;
			}
			result = yaml_document_get_node(tree.document, *(result->data.sequence.items.start + seq_idx));
			assert(result);
			if ( *index != ']' ) {
				err = PC_INVALID_PARAMETER;
				goto vget_free;
			}
			++index;
		}; break;
		case '.': {
			if ( result->type != YAML_MAPPING_NODE ) {
				err = PC_INVALID_NODE_TYPE;
				goto vget_free;
			}
			++index; // consume the starting '.'
			int id_len = 0;
			while ( index[id_len] && index[id_len] != '.' && index[id_len] != '[' ) ++id_len;
			yaml_node_pair_t *pair = result->data.mapping.pairs.start;
			while ( pair != result->data.mapping.pairs.top ) {
				yaml_node_t *key = yaml_document_get_node(tree.document, pair->key);
				assert(key);
				PC_tree_t key_tree = { tree.document, key };
				char *key_str = NULL; PC_status_t errcode = PC_as_string(key_tree, &key_str, 0);
				if ( errcode ) return errcode;
				int cmp = strncmp(index, key_str, id_len);
				free(key_str);
				if ( !cmp ) break;
				++pair;
			}
			if ( pair == result->data.mapping.pairs.top ) {
				err = PC_NODE_NOT_FOUND;
				goto vget_free;
			}
			index += id_len;
			result = yaml_document_get_node(tree.document, pair->value);
			assert(result);
		}; break;
		case '{': {
			if ( result->type != YAML_MAPPING_NODE ) {
				err = PC_INVALID_NODE_TYPE;
				goto vget_free;
			}
			++index; // consume the starting '{'
			char *post_index;
			long map_idx = strtol(index, &post_index, 0);
			if ( post_index == index ) {
				err = PC_INVALID_PARAMETER;
				goto vget_free;
			}
			index = post_index;
			if ( map_idx > result->data.mapping.pairs.top - result->data.mapping.pairs.start ) {
				err = PC_INVALID_PARAMETER;
				goto vget_free;
			}
			result = yaml_document_get_node(tree.document, (result->data.mapping.pairs.start + map_idx)->key);
			assert(result);
			if ( *index != '}' ) {
				err = PC_INVALID_PARAMETER;
				goto vget_free;
			}
			++index;
		}; break;
		case '<': {
			if ( result->type != YAML_MAPPING_NODE ) {
				err = PC_INVALID_NODE_TYPE;
				goto vget_free;
			}
			++index; // consume the starting '<'
			char *post_index;
			long map_idx = strtol(index, &post_index, 0);
			if ( post_index == index ) {
				err = PC_INVALID_PARAMETER;
				goto vget_free;
			}
			index = post_index;
			if ( map_idx > result->data.mapping.pairs.top - result->data.mapping.pairs.start ) {
				err = PC_INVALID_PARAMETER;
				goto vget_free;
			}
			result = yaml_document_get_node(tree.document, (result->data.mapping.pairs.start + map_idx)->value);
			assert(result);
			if ( *index != '>' ) {
				err = PC_INVALID_PARAMETER;
				goto vget_free;
			}
			++index;
		}; break;
		case 0: {
			assert(result);
			value->node = result;
			value->document = tree.document;
			err = PC_OK;
			goto vget_free;
		}
		default: {
			err = PC_INVALID_PARAMETER;
			goto vget_free;
		};
		}
	}
vget_free:
	free(index_free);
	return err;
}

PC_status_t PC_get_len(PC_tree_t tree, const char* index, int* len, ...)
{
	va_list ap;
	va_start(ap, len);
	PC_status_t res = PC_vget_len(tree, index, len, ap);
	va_end(ap);
	return res;
}

PC_status_t PC_vget_len(PC_tree_t tree, const char* index_fmt, int* len, va_list ap)
{
	char *index = msprintf(index_fmt, ap);
	PC_tree_t value_node; PC_status_t err = PC_get(tree, index, &value_node); if ( err ) goto vget_len_free; 
	err = PC_as_len(value_node, len);
vget_len_free:
	free(index);
	return err;
}


PC_status_t PC_get_double(PC_tree_t tree, const char* index, double* value, ...)
{
	va_list ap;
	va_start(ap, value);
	PC_status_t res = PC_vget_double(tree, index, value, ap);
	va_end(ap);
	return res;
}

PC_status_t PC_vget_double(PC_tree_t tree, const char* index_fmt, double* value, va_list ap)
{
	char *index = msprintf(index_fmt, ap);
	PC_tree_t value_node; PC_status_t err = PC_get(tree, index, &value_node); if ( err ) goto vget_double_free; 
	err = PC_as_double(value_node, value);
vget_double_free:
	free(index);
	return err;
}

PC_status_t PC_get_int(PC_tree_t tree, const char* index, int* value, ...)
{
	va_list ap;
	va_start(ap, value);
	PC_status_t res = PC_vget_int(tree, index, value, ap);
	va_end(ap);
	return res;
}

PC_status_t PC_vget_int(PC_tree_t tree, const char* index_fmt, int* value, va_list ap)
{
	char *index = msprintf(index_fmt, ap);
	PC_tree_t value_node; PC_status_t err = PC_get(tree, index, &value_node); if ( err ) goto vget_int_free; 
	err = PC_as_int(value_node, value);
vget_int_free:
	free(index);
	return err;
}

PC_status_t PC_get_string(PC_tree_t tree, const char* index, char** value, int* value_len, ...)
{
	va_list ap;
	va_start(ap, value_len);
	PC_status_t res = PC_vget_string(tree, index, value, value_len, ap);
	va_end(ap);
	return res;
}

PC_status_t PC_vget_string(PC_tree_t tree, const char* index_fmt, char** value, int* value_len, va_list ap)
{
	char *index = msprintf(index_fmt, ap);
	PC_tree_t value_node; PC_status_t err = PC_get(tree, index, &value_node); if ( err ) goto vget_string_free;
	err = PC_as_string(value_node, value, value_len);
vget_string_free:
	free(index);
	return err;
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
	return PC_OK;
}
