/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include <string.h>

#include "paraconf.h"

#include "status.h"
#include "tools.h"

#include "ypath.h"

static const char* nodetype[4] = {"none", "scalar", "sequence", "mapping"};

/** The part of a message describing what was found at a position of a request: the character there, quoted, or the end of the request
 *
 * It takes the arguments PC_FOUND_ARGS gives for the position: %.1s prints nothing at the terminating NUL, where %c would cut the message short.
 */
#define PC_FOUND_FMT "%s%.1s%s"

/// The arguments of PC_FOUND_FMT for a position of a request
#define PC_FOUND_ARGS(position) (*(position) ? "`" : "the end of the request"), (position), (*(position) ? "'" : "")

static PC_tree_t get_seq_idx(const PC_tree_t tree, const char** req_index, const char* full_index)
{
	PC_tree_t restree = tree;
	PC_handle_tree(err0);

	const char* index = *req_index;

	// read '['
	if (*index != '[') {
		PC_handle_err_tree(
			PC_make_err(
				PC_INVALID_PARAMETER,
				"Expected `[' at char #%ld of `%s', but found " PC_FOUND_FMT,
				(long int)(index - full_index),
				full_index,
				PC_FOUND_ARGS(index)
			),
			err0
		);
	}
	++index;

	// read int
	char* post_index;
	long seq_idx = strtol(index, &post_index, 10);
	if (post_index == index) {
		PC_handle_err_tree(
			PC_make_err(
				PC_INVALID_PARAMETER,
				"Expected integer at char #%ld of `%s', but found " PC_FOUND_FMT,
				(long int)(index - full_index),
				full_index,
				PC_FOUND_ARGS(index)
			),
			err0
		);
	}
	index = post_index;

	// read ']'
	if (*index != ']') {
		PC_handle_err_tree(
			PC_make_err(
				PC_INVALID_PARAMETER,
				"Expected `]' at char #%ld of `%s', but found " PC_FOUND_FMT,
				(long int)(index - full_index),
				full_index,
				PC_FOUND_ARGS(index)
			),
			err0
		);
	}
	++index;

	// check type
	if (tree.node->type != YAML_SEQUENCE_NODE) {
		PC_handle_err_tree(
			PC_make_node_err(
				PC_INVALID_NODE_TYPE,
				tree,
				"Expected a sequence, found a %s (request was: `$tree%.*s')",
				nodetype[tree.node->type],
				(int)(index - full_index),
				full_index
			),
			err0
		);
	}

	// handle index
	if (seq_idx < 0 || seq_idx >= (tree.node->data.sequence.items.top - tree.node->data.sequence.items.start)) {
		PC_handle_err_tree(
			PC_make_node_err(
				PC_NODE_NOT_FOUND,
				tree,
				"Index %ld out of range [0...%ld) in sequence (request was: `$tree%.*s')",
				seq_idx,
				(long)(tree.node->data.sequence.items.top - tree.node->data.sequence.items.start),
				(int)(index - full_index),
				full_index
			),
			err0
		);
	}
	restree = subtree(tree, *(tree.node->data.sequence.items.start + seq_idx));

	*req_index = index;
	return restree;

err0:
	return restree;
}

/// How many merges are followed at most, so that a mapping merging itself through an alias does not loop forever
#define PC_MAX_MERGE_DEPTH 64

/** Looks a key up in a mapping, then in the mappings it merges with `<<' keys
 *
 * As YAML specifies merge keys, the keys of the mapping itself take precedence over the merged ones, and of several merged mappings, the first one
 * holding the key does.
 *
 * \param mapping the mapping to look the key up in
 * \param key the key, not NUL-terminated
 * \param key_len the length of the key
 * \param depth the number of merges followed to reach this mapping
 * \param[out] value the id of the value of the key, 0 if it is not found
 * \return the status of the execution
 */
static PC_status_t find_key(const PC_tree_t mapping, const char* key, size_t key_len, int depth, int* value)
{
	PC_status_t status = PC_OK;
	*value = 0;

	int merges = 0;
	for (yaml_node_pair_t* pair = mapping.node->data.mapping.pairs.start; pair != mapping.node->data.mapping.pairs.top; ++pair) {
		char* found_key;
		PC_handle_err(PC_string(subtree(mapping, pair->key), &found_key), err0);
		int cmp = strlzcmp(key, found_key, key_len);
		merges |= !strcmp(found_key, "<<");
		free(found_key);
		if (!cmp) {
			*value = pair->value;
			return status;
		}
	}
	if (!merges || depth >= PC_MAX_MERGE_DEPTH) return status;

	for (yaml_node_pair_t* pair = mapping.node->data.mapping.pairs.start; pair != mapping.node->data.mapping.pairs.top; ++pair) {
		yaml_node_t* merge_key = subtree(mapping, pair->key).node;
		if (strcmp((char*)merge_key->data.scalar.value, "<<")) continue;
		PC_tree_t merged = subtree(mapping, pair->value);
		if (merged.node->type == YAML_MAPPING_NODE) {
			PC_handle_err(find_key(merged, key, key_len, depth + 1, value), err0);
		} else if (merged.node->type == YAML_SEQUENCE_NODE) {
			for (yaml_node_item_t* item = merged.node->data.sequence.items.start; !*value && item != merged.node->data.sequence.items.top; ++item) {
				PC_tree_t one_merged = subtree(merged, *item);
				if (one_merged.node->type == YAML_MAPPING_NODE) {
					PC_handle_err(find_key(one_merged, key, key_len, depth + 1, value), err0);
				}
			}
		}
		if (*value) return status;
	}
	return status;

err0:
	return status;
}

static PC_tree_t get_map_key_val(const PC_tree_t tree, const char** req_index, const char* full_index)
{
	PC_tree_t restree = tree;
	PC_handle_tree(err0);

	const char* index = *req_index;

	// read '.'
	if (*index != '.') {
		PC_handle_err_tree(
			PC_make_err(
				PC_INVALID_PARAMETER,
				"Expected `.' at char #%ld of `%s', but found " PC_FOUND_FMT,
				(long int)(index - full_index),
				full_index,
				PC_FOUND_ARGS(index)
			),
			err0
		);
	}
	++index;

	// read key
	const char* key = index;
	size_t key_len = 0;
	while (key[key_len] && key[key_len] != '.' && key[key_len] != '[' && key[key_len] != '{' && key[key_len] != '<')
		++key_len;
	index += key_len;

	// check type
	if (tree.node->type != YAML_MAPPING_NODE) {
		PC_handle_err_tree(
			PC_make_node_err(
				PC_INVALID_NODE_TYPE,
				tree,
				"Expected a mapping, found a %s (request was: `$tree%.*s')",
				nodetype[tree.node->type],
				(int)(index - full_index),
				full_index
			),
			err0
		);
	}

	// handle key
	int value = 0;
	PC_handle_err_tree(find_key(tree, key, key_len, 0, &value), err0);
	if (!value) {
		PC_handle_err_tree(
			PC_make_node_err(
				PC_NODE_NOT_FOUND,
				tree,
				"Key `%.*s' not found in mapping (request was: `$tree%.*s')",
				(int)key_len,
				key,
				(int)(index - full_index),
				full_index
			),
			err0
		);
	}
	restree = subtree(tree, value);

	*req_index = index;
	return restree;

err0:
	return restree;
}

static PC_status_t get_map_idx_pair(const PC_tree_t tree, const char** req_index, const char* full_index, yaml_node_pair_t** pair)
{
	PC_status_t status = PC_OK;
	PC_handle_tree_err(tree, err0);

	const char* index = *req_index;

	// read int
	char* post_index;
	long map_idx = strtol(index, &post_index, 10);
	if (post_index == index) {
		PC_handle_err(
			PC_make_err(
				PC_INVALID_PARAMETER,
				"Expected an integer at char #%ld of `%s', but found " PC_FOUND_FMT,
				(long int)(index - full_index),
				full_index,
				PC_FOUND_ARGS(index)
			),
			err0
		);
	}
	index = post_index;

	// check type
	if (tree.node->type != YAML_MAPPING_NODE) {
		PC_handle_err(
			PC_make_node_err(
				PC_INVALID_NODE_TYPE,
				tree,
				"Expected a mapping, found a %s (request was: `$tree%.*s')",
				nodetype[tree.node->type],
				(int)(index - full_index),
				full_index
			),
			err0
		);
	}

	// handle index
	if (map_idx < 0 || map_idx >= (tree.node->data.mapping.pairs.top - tree.node->data.mapping.pairs.start)) {
		PC_handle_err(
			PC_make_node_err(
				PC_NODE_NOT_FOUND,
				tree,
				"Index %ld out of range [0...%ld) in mapping (request was: `$tree%.*s')",
				map_idx,
				(long)(tree.node->data.mapping.pairs.top - tree.node->data.mapping.pairs.start),
				(int)(index - full_index),
				full_index
			),
			err0
		);
	}
	*pair = tree.node->data.mapping.pairs.start + map_idx;

	*req_index = index;
	return status;

err0:
	return status;
}

static PC_tree_t get_map_idx_key(const PC_tree_t tree, const char** req_index, const char* full_index)
{
	PC_tree_t restree = tree;
	PC_handle_tree(err0);

	const char* index = *req_index;

	// read '{'
	if (*index != '{') {
		PC_handle_err_tree(
			PC_make_err(
				PC_INVALID_PARAMETER,
				"Expected `{' at char #%ld of `%s', but found " PC_FOUND_FMT,
				(long int)(index - full_index),
				full_index,
				PC_FOUND_ARGS(index)
			),
			err0
		);
	}
	++index;

	// get pair
	yaml_node_pair_t* pair = NULL;
	PC_handle_err_tree(get_map_idx_pair(tree, &index, full_index, &pair), err0);

	// read '}'
	if (*index != '}') {
		PC_handle_err_tree(
			PC_make_err(
				PC_INVALID_PARAMETER,
				"Expected `}' at char #%ld of `%s', but found " PC_FOUND_FMT,
				(long int)(index - full_index),
				full_index,
				PC_FOUND_ARGS(index)
			),
			err0
		);
	}
	++index;

	// handle pair
	restree = subtree(tree, pair->key);

	*req_index = index;
	return restree;

err0:
	return restree;
}

static PC_tree_t get_map_idx_val(const PC_tree_t tree, const char** req_index, const char* full_index)
{
	PC_tree_t restree = tree;
	PC_handle_tree(err0);

	const char* index = *req_index;

	// read '<'
	if (*index != '<') {
		PC_handle_err_tree(
			PC_make_err(
				PC_INVALID_PARAMETER,
				"Expected `<' at char #%ld of `%s', but found " PC_FOUND_FMT,
				(long int)(index - full_index),
				full_index,
				PC_FOUND_ARGS(index)
			),
			err0
		);
	}
	++index;

	// get pair
	yaml_node_pair_t* pair = NULL;
	PC_handle_err_tree(get_map_idx_pair(tree, &index, full_index, &pair), err0);

	// read '>'
	if (*index != '>') {
		PC_handle_err_tree(
			PC_make_err(
				PC_INVALID_PARAMETER,
				"Expected `>' at char #%ld of `%s', but found " PC_FOUND_FMT,
				(long int)(index - full_index),
				full_index,
				PC_FOUND_ARGS(index)
			),
			err0
		);
	}
	++index;

	// handle pair
	restree = subtree(tree, pair->value);

	*req_index = index;
	return restree;

err0:
	return restree;
}

PC_tree_t PC_sget(const PC_tree_t tree, const char* index)
{
	PC_tree_t restree = tree;
	PC_handle_tree(err0);

	if (!index) PC_handle_err_tree(PC_make_err(PC_INVALID_PARAMETER, "no index passed to PC_get"), err0);

	// check type
	if (*index && !tree.node) {
		PC_handle_err_tree(PC_make_err(PC_INVALID_NODE_TYPE, "Expected a node, found an empty tree"), err0);
	}

	const char* full_index = index;

	while (!PC_status(restree)) {
		switch (*index) {
		case '[':
			restree = get_seq_idx(restree, &index, full_index);
			break;
		case '.':
			restree = get_map_key_val(restree, &index, full_index);
			break;
		case '{':
			restree = get_map_idx_key(restree, &index, full_index);
			break;
		case '<':
			restree = get_map_idx_val(restree, &index, full_index);
			break;
		case 0:
			goto brake_out_of_while;
		default:
			PC_handle_err_tree(
				PC_make_err(
					PC_INVALID_PARAMETER,
					"Expected `[', `.', `{' or `<' at char #%ld of `%s', but found " PC_FOUND_FMT,
					(long int)(index - full_index),
					full_index,
					PC_FOUND_ARGS(index)
				),
				err0
			);
		}
	}
brake_out_of_while:

	return restree;

err0:
	return restree;
}
