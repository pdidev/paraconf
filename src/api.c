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

PC_status_t PC_get(yaml_document_t* document, yaml_node_t* node, const char* index, yaml_node_t** value)
{
	yaml_node_t* result;
	if ( node ) {
		result = node;
	} else {
		result = yaml_document_get_root_node(document);
	}
	assert(result);
	
	while ( index ) {
		switch ( *index ) {
		case '[': {
			if ( result->type != YAML_SEQUENCE_NODE ) return PC_INVALID_NODE_TYPE;
			++index; // consume the starting '['
			char *post_index;
			long seq_idx = strtol(index, &post_index, 0);
			if ( post_index == index ) return PC_INVALID_PARAMETER;
			index = post_index;
			if ( seq_idx > result->data.sequence.items.top - result->data.sequence.items.start ) {
				return PC_INVALID_PARAMETER;
			}
			result = yaml_document_get_node(document, *(result->data.sequence.items.start + seq_idx));
			assert(result);
			if ( *index != ']' ) return PC_INVALID_PARAMETER;
			++index;
		}; break;
		case '.': {
			if ( result->type != YAML_MAPPING_NODE ) return PC_INVALID_NODE_TYPE;
			++index; // consume the starting '.'
			int id_len = 0;
			while ( index[id_len] && index[id_len] != '.' && index[id_len] != '[' ) ++id_len;
			yaml_node_pair_t *pair = result->data.mapping.pairs.start;
			while ( pair != result->data.mapping.pairs.top ) {
				char *key_str;
				yaml_node_t *key = yaml_document_get_node(document, pair->key);
				assert(key);
				PC_status_t errcode = PC_as_string(document, key, &key_str);
				if ( errcode ) return errcode;
				int cmp = strncmp(index, key_str, id_len);
				free(key_str);
				if ( !cmp ) break;
				++pair;
			}
			if ( pair == result->data.mapping.pairs.top ) return PC_NODE_NOT_FOUND;
			index += id_len;
			result = yaml_document_get_node(document, pair->value);
			assert(result);
		}; break;
		case 0: {
			assert(result);
			*value = result;
			return PC_OK;
		}
		default: {
			return PC_INVALID_PARAMETER;
		};
		}
	}
}

PC_status_t PC_get_len(yaml_document_t* document, yaml_node_t* node, const char* index, int* len)
{
	yaml_node_t *value_node;
	PC_status_t result = PC_get(document, node, index, &value_node);
	if ( result ) return result;
	return PC_as_len(document, value_node, len);
}


PC_status_t PC_get_double(yaml_document_t* document, yaml_node_t* node, const char* index, double* value)
{
	yaml_node_t *value_node;
	PC_status_t result = PC_get(document, node, index, &value_node);
	if ( result ) return result;
	return PC_as_double(document, value_node, value);
}

PC_status_t PC_get_int(yaml_document_t* document, yaml_node_t* node, const char* index, int* value)
{
	yaml_node_t *value_node;
	PC_status_t result = PC_get(document, node, index, &value_node);
	if ( result ) return result;
	return PC_as_int(document, value_node, value);
}

PC_status_t PC_get_string(yaml_document_t* document, yaml_node_t* node, const char* index, char** value)
{
	yaml_node_t *value_node;
	PC_status_t result = PC_get(document, node, index, &value_node);
	if ( result ) return result;
	return PC_as_string(document, value_node, value);
}

PC_status_t PC_broadcast(yaml_document_t* document, int count, int root, MPI_Comm comm)
{
	yaml_emitter_t *emitter;
	yaml_emitter_initialize(emitter);
	yaml_emitter_set_width(emitter, -1);
	yaml_emitter_set_canonical(emitter, 1);
	yaml_emitter_open(emitter);
	
	size_t buf_size = PC_BUFFER_SIZE/2;
	char *buf = 0;
	int err = YAML_WRITER_ERROR;
	unsigned long data_size;
	while ( err == YAML_WRITER_ERROR ) {
		buf_size *= 2;
		buf = realloc(buf, buf_size);
		yaml_emitter_set_output_string(emitter, buf, buf_size, &data_size);
		err = yaml_emitter_dump(emitter, document);
	}
	yaml_emitter_close(emitter);
	yaml_emitter_delete(emitter);
	
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
