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
#include "ytype.h"

PC_status_t PC_as_len(yaml_document_t* document, yaml_node_t* value_node, int* len)
{
	switch ( value_node->type ) {
	case YAML_SEQUENCE_NODE: {
		*len = value_node->data.sequence.items.top - value_node->data.sequence.items.start;
	} break;
	case YAML_MAPPING_NODE: {
		*len = value_node->data.mapping.pairs.top - value_node->data.mapping.pairs.start;
	} break;
	default: {
		return PC_INVALID_NODE_TYPE;
	} break;
	}
	return PC_OK;
}

PC_status_t PC_as_double(yaml_document_t* document, yaml_node_t* value_node, double* value)
{
	if ( value_node->type != YAML_SCALAR_NODE ) return PC_INVALID_NODE_TYPE;
	char *endptr;
	double result = strtod(value_node->data.scalar.value, &endptr);
	if ( *endptr ) return PC_INVALID_NODE_TYPE;
	*value = result;
	return PC_OK;
}

PC_status_t PC_as_int(yaml_document_t* document, yaml_node_t* node, int* value)
{
	if ( node->type != YAML_SCALAR_NODE ) return PC_INVALID_NODE_TYPE;
	char *endptr;
	long result = strtol(node->data.scalar.value, &endptr, 0);
	if ( *endptr ) return PC_INVALID_NODE_TYPE;
	*value = result;
	return PC_OK;
}

PC_status_t PC_as_string(yaml_document_t* document, yaml_node_t* value_node, char** value)
{
	if ( value_node->type != YAML_SCALAR_NODE ) return PC_INVALID_NODE_TYPE;
	switch ( value_node->data.scalar.style ) {
	case YAML_SINGLE_QUOTED_SCALAR_STYLE:
	case YAML_DOUBLE_QUOTED_SCALAR_STYLE: {
		*value = malloc(value_node->data.scalar.length-2+1);
		strncpy(*value, value_node->data.scalar.value, value_node->data.scalar.length-2+1);
		assert((*value)[value_node->data.scalar.length-2]==0);
	} break;
	default: {
		*value = malloc(value_node->data.scalar.length+1);
		strncpy(*value, value_node->data.scalar.value, value_node->data.scalar.length+1);
		assert((*value)[value_node->data.scalar.length]==0);
	} break;
	}
	return PC_OK;
}


