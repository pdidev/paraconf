/*******************************************************************************
 * Copyright (C) 2015-2019 Commissariat a l'energie atomique et aux energies alternatives (CEA)
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

#include <cstring>
#include <string>

#include "paraconf/error.h"
#include "paraconf/node.h"

using PC::Error;
using std::exception;
using std::string;

namespace PC {

PC_tree_t get_seq_idx(PC_tree_t tree, string& index)
{		
	// skip '['
	index = index.substr(1);
	
	// read int
	char* post_index;
	long seq_idx = strtol(index.c_str(), &post_index, 0);
	if ( post_index == index ) {
		throw Error{PC_INVALID_PARAMETER, "Expected integer in place of %s", index.c_str()};
	}
	index = index.substr(post_index - index.c_str());

	// read ']'
	if ( index[0] != ']' ) {
		throw Error{PC_INVALID_PARAMETER, "Expected closing square bracket in place of %s", index.c_str()};
	}
	index = index.substr(1);

	// get and return new tree
	return tree->acquire(tree->get(seq_idx));
}

PC_tree_t get_map_key_val(PC_tree_t tree, string& index)
{		
	// skip '.'
	index = index.substr(1);
	
	// read key
	size_t found = index.find_first_of(".[{<");
	if (found == string::npos) {
		found = index.size();
	}
	string key = index.substr(0, found);

	// cut key from index
	index = index.substr(found);

	// get and return new tree
	return tree->acquire(tree->get(key));
}

PC_tree_t get_map_idx_key(PC_tree_t tree, string& index)
{
	// skip '{'
	index = index.substr(1);
	
	// read int
	char* post_index;
	long map_key_idx = strtol(index.c_str(), &post_index, 0);
	if ( post_index == index ) {
		throw Error{PC_INVALID_PARAMETER, "Expected integer in place of %s", index.c_str()};
	}
	index = index.substr(post_index - index.c_str());

	// read '}'
	if ( index[0] != '}' ) {
		throw Error{PC_INVALID_PARAMETER, "Expected closing curly bracket in place of %s", index.c_str()};
	}
	index = index.substr(1);

	// get and return new tree
	return tree->acquire(tree->key(map_key_idx));
}

PC_tree_t get_map_idx_val(PC_tree_t tree, string& index)
{
	// skip '<'
	index = index.substr(1);
	
	// read int
	char* post_index;
	long map_value_idx = strtol(index.c_str(), &post_index, 0);
	if ( post_index == index ) {
		throw Error{PC_INVALID_PARAMETER, "Expected integer in place of %s", index.c_str()};
	}
	index = index.substr(post_index - index.c_str());
	
	// read '>'
	if ( index[0] != '>' ) {
		throw Error{PC_INVALID_PARAMETER, "Expected closing angle bracket in place of %s", index.c_str()};
	}
	index = index.substr(1);

	// get and return new tree
	return tree->acquire(tree->value(map_value_idx));
}

} // namespace PC
