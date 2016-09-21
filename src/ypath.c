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

#include "status.h"
#include "tools.h"

static const char *nodetype[4] = {
	"none",
	"scalar",
	"sequence",
	"mapping"
};

static PC_tree_t get_seq_idx(PC_tree_t tree, const char **req_index, const char *full_index)
{
	const char *index = *req_index;
	
	// read '['
	if ( *index != '[' ) {
		tree.status = make_error(PC_INVALID_PARAMETER, "Expected opening square bracket at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
		goto err0;
	}
	++index;
	
	// read int
	char *post_index;
	long seq_idx = strtol(index, &post_index, 0);
	if ( post_index == index ) {
		tree.status = make_error(PC_INVALID_PARAMETER, "Expected integer at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
		goto err0;
	}
	index = post_index;
	
	// read ']'
	if ( *index != ']' ) {
		tree.status = make_error(PC_INVALID_PARAMETER, "Expected closing square bracket at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
		goto err0;
	}
	++index;
	
	// check type
	if ( tree.node->type != YAML_SEQUENCE_NODE ) {
		tree.status = make_error(PC_INVALID_NODE_TYPE, "Expected sequence, found %s (ROOT)%.*s\n",
				nodetype[tree.node->type],
				(int)(*req_index-full_index),
				full_index);
		goto err0;
	}
	
	// handle index
	if ( seq_idx < 0 || seq_idx >= (tree.node->data.sequence.items.top - tree.node->data.sequence.items.start) ) {
		tree.status = make_error(PC_NODE_NOT_FOUND, "Index %ld out of range [0...%ld[ in (ROOT)%.*s\n",
				seq_idx,
				(long)(tree.node->data.sequence.items.top - tree.node->data.sequence.items.start),
				(int)(*req_index-full_index),
				full_index);
		goto err0;
	}
	tree.node = yaml_document_get_node(tree.document, *(tree.node->data.sequence.items.start + seq_idx));
	assert(tree.node);
	*req_index = index;
	
err0:
	
	return tree;
}

static PC_tree_t get_map_key_val(PC_tree_t tree, const char **req_index, const char *full_index)
{
	const char *index = *req_index;
		
	// read '.'
	if ( *index != '.' ) {
		tree.status = make_error(PC_INVALID_PARAMETER, "Expected dot at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
		goto err0;
	}
	++index;
	
	// read key
	const char *key = index;
	size_t key_len = 0;
	while ( key[key_len] && key[key_len] != '.' && key[key_len] != '[' && key[key_len] != '{' && key[key_len] != '<' ) ++key_len;
	index += key_len;
	
	// check type
	if ( tree.node->type != YAML_MAPPING_NODE ) {
		tree.status = make_error(PC_INVALID_NODE_TYPE, "Expected mapping, found %s (ROOT)%.*s\n",
				nodetype[tree.node->type],
				(int)(*req_index-full_index),
				full_index);
		goto err0;
	}
	
	// handle key
	yaml_node_pair_t *pair;
	for ( pair = tree.node->data.mapping.pairs.start; pair != tree.node->data.mapping.pairs.top; ++pair ) {
		// get the key string
		char *found_key;
		tree.status = PC_string(subtree(tree, pair->key), &found_key);
		if ( PC_status(tree) ) goto err0;
		
		// check if we found the key, in that case, leave
		int cmp = strlzcmp(key, found_key, key_len);
		free(found_key);
		if ( !cmp ) break;
	}
	if ( pair == tree.node->data.mapping.pairs.top ) {
		tree.status = make_error(PC_NODE_NOT_FOUND, "Key `%.*s' not found in (ROOT)%.*s\n",
				key_len,
				key,
				(int)(*req_index-full_index),
				full_index);
		goto err0;
	}
	tree.node = yaml_document_get_node(tree.document, pair->value);
	assert(tree.node);
	*req_index = index;
	
err0:
	
	return tree;
}

static PC_status_t get_map_idx_pair(PC_tree_t tree, const char **req_index, const char *full_index, yaml_node_pair_t **pair)
{
	const char *index = *req_index;
	PC_status_t status = PC_OK;
	
	// read int
	char *post_index;
	long map_idx = strtol(index, &post_index, 0);
	if ( post_index == index ) {
		status = make_error(PC_INVALID_PARAMETER, "Expected integer at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
		goto err0;
	}
	index = post_index;
	
	// check type
	if ( tree.node->type != YAML_MAPPING_NODE ) {
		status = make_error(PC_INVALID_NODE_TYPE, "Expected mapping, found %s (ROOT)%.*s\n",
				nodetype[tree.node->type],
				(int)(*req_index-full_index),
				full_index);
		goto err0;
	}
	
	// handle index
	if ( map_idx < 0 || map_idx >= (tree.node->data.mapping.pairs.top - tree.node->data.mapping.pairs.start) ) {
		status = make_error(PC_NODE_NOT_FOUND, "Index %ld out of range [0...%ld] in (ROOT)%.*s\n",
				map_idx,
				(long)(tree.node->data.mapping.pairs.top - tree.node->data.mapping.pairs.start),
				(int)(*req_index-full_index-1),
				full_index);
		goto err0;
	}
	*pair = tree.node->data.mapping.pairs.start + map_idx;
	assert(*pair);
	*req_index = index;
	
err0:

	return status;
}

static PC_tree_t get_map_idx_key(PC_tree_t tree, const char **req_index, const char *full_index)
{
	const char *index = *req_index;
	
	// read '{'
	if ( *index != '{' ) {
		tree.status = make_error(PC_INVALID_PARAMETER, "Expected opening curly bracket at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
		goto err0;
	}
	++index;
	
	// get pair
	yaml_node_pair_t *pair = NULL; 
	handle_error_tree(get_map_idx_pair(tree, &index, full_index, &pair),err0);
	
	// read '}'
	if ( *index != '}' ) {
		tree.status = make_error(PC_INVALID_PARAMETER, "Expected closing curly bracket at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
		goto err0;
	}
	++index;
	
	// handle pair
	tree.node = yaml_document_get_node(tree.document, pair->key);
	assert(tree.node);
	*req_index = index;
	
err0:
	
	return tree;
}

static PC_tree_t get_map_idx_val(PC_tree_t tree, const char **req_index, const char *full_index)
{
	const char *index = *req_index;
	
	// read '<'
	if ( *index != '<' ) {
		tree.status = make_error(PC_INVALID_PARAMETER, "Expected opening angle bracket at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
		goto err0;
	}
	++index;
	
	// get pair
	yaml_node_pair_t *pair = NULL; 
	handle_error_tree(get_map_idx_pair(tree, &index, full_index, &pair),err0);
	
	// read '>'
	if ( *index != '>' ) {
		tree.status = make_error(PC_INVALID_PARAMETER, "Expected closing angle bracket at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
		goto err0;
	}
	++index;
	
	// handle pair
	tree.node = yaml_document_get_node(tree.document, pair->value);
	assert(tree.node);
	*req_index = index;
	
err0:
	
	return tree;
}

PC_tree_t PC_sget(PC_tree_t tree, const char *index)
{
	const char *full_index = index;
	
	do {
		switch ( *index ) {
		case '[':
			tree = get_seq_idx(tree, &index, full_index);
			break;
		case '.': 
			tree = get_map_key_val(tree, &index, full_index);
			break;
		case '{': 
			tree = get_map_idx_key(tree, &index, full_index);
			break;
		case '<': 
			tree = get_map_idx_val(tree, &index, full_index);
			break;
		case 0:
			assert(tree.node);
			goto err0;
		default:
			tree.status = make_error(PC_INVALID_PARAMETER, "Invalid character at char #%ld of `%.*s'\n",
				(long int)(index-full_index),
				full_index);
			goto err0;
		}
	} while (tree.status == PC_OK);
	
err0:
	return tree;
}
