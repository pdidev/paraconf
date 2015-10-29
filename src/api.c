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

#define PC_BUFFER_SIZE 256

PC_status PC_as_double(yaml_document_t* document, yaml_node_t* value_node, double* value);
PC_status PC_as_int(yaml_document_t* document, yaml_node_t* node, int* value);
PC_status PC_as_string(yaml_document_t* document, yaml_node_t* value_node, char** value);

PC_status PC_get(yaml_document_t* document, yaml_node_t* node, const char* index, yaml_node_t** value)
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
				PC_status errcode = PC_as_string(document, key, &key_str);
				if ( errcode ) return errcode;
				if ( !strncmp(index, key_str, id_len) ) break;
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

PC_status PC_as_double(yaml_document_t* document, yaml_node_t* value_node, double* value)
{
	if ( value_node->type != YAML_SCALAR_NODE ) return PC_INVALID_NODE_TYPE;
	char *endptr;
	double result = strtod(value_node->data.scalar.value, &endptr);
	if ( *endptr ) return PC_INVALID_NODE_TYPE;
	*value = result;
	return PC_OK;
}

PC_status PC_get_double(yaml_document_t* document, yaml_node_t* node, const char* index, double* value)
{
	yaml_node_t *value_node;
	PC_status result = PC_get(document, node, index, &value_node);
	if ( result ) return result;
	return PC_as_double(document, value_node, value);
}

PC_status PC_as_int(yaml_document_t* document, yaml_node_t* node, int* value)
{
	if ( node->type != YAML_SCALAR_NODE ) return PC_INVALID_NODE_TYPE;
	char *endptr;
	long result = strtol(node->data.scalar.value, &endptr, 0);
	if ( *endptr ) return PC_INVALID_NODE_TYPE;
	*value = result;
	return PC_OK;
}

PC_status PC_get_int(yaml_document_t* document, yaml_node_t* node, const char* index, int* value)
{
	yaml_node_t *value_node;
	PC_status result = PC_get(document, node, index, &value_node);
	if ( result ) return result;
	return PC_as_int(document, value_node, value);
}

PC_status PC_as_string(yaml_document_t* document, yaml_node_t* value_node, char** value)
{
	if ( value_node->type != YAML_SCALAR_NODE ) return PC_INVALID_NODE_TYPE;
	*value = value_node->data.scalar.value;
	switch ( value_node->data.scalar.style ) {
	case YAML_SINGLE_QUOTED_SCALAR_STYLE: {
		assert(**value == '\'');
		++(*value);
		//TODO: remove the terminating quote
	} break;
	case YAML_DOUBLE_QUOTED_SCALAR_STYLE: {
		assert(**value == '"');
		++(*value);
		//TODO: remove the terminating double-quote
	} break;
	default: {
		// nothing to do here
	} break;
	}
	return PC_OK;
}

PC_status PC_get_string(yaml_document_t* document, yaml_node_t* node, const char* index, char** value)
{
	yaml_node_t *value_node;
	PC_status result = PC_get(document, node, index, &value_node);
	if ( result ) return result;
	return PC_as_string(document, value_node, value);
}

PC_status PC_broadcast(yaml_document_t* document, yaml_node_t* node, int count, int root, MPI_Comm comm)
{
	char data[255];
	MPI_Bcast(data, 255, MPI_CHAR, root, comm);
	return PC_NOT_IMPLEMENTED;
}

