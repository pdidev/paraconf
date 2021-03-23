/*******************************************************************************
 * Copyright (C) 2015-2019 Commissariat a l'energie atomique et aux energies alternatives (CEA)
 * Copyright (C) 2021 Institute of Bioorganic Chemistry Polish Academy of Science (PSNC)
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

#include <iostream>
#include <fstream>
#include <memory>

#include "paraconf/error.h"
#include "paraconf/PC_node.h"
#include "ypath_tools.h"

#include "paraconf.h"

using namespace PC;
using std::exception;
using std::ifstream;
using std::istreambuf_iterator;
using std::string;
using std::unique_ptr;
using YAML::NodeType;

struct Error_context {
	PC_errhandler_t handler;
	
	string errmsg;
	
	Error_context(): handler{PC_ASSERT_HANDLER} {}
	
	/** Return the C error and stores the message corresponding to the C++ exception
	 */
	PC_status_t return_err(const Error& err)
	{
		errmsg = err.what();
		if (handler.func) handler.func(err.status(), errmsg.c_str(), handler.context);
		return err.status();
	}
	
	/** Return the C error and stores the message corresponding to the C++ exception
	 */
	PC_status_t return_err(const exception& err)
	{
		errmsg = err.what();
		if (handler.func) handler.func(PC_SYSTEM_ERROR, errmsg.c_str(), handler.context);
		return PC_SYSTEM_ERROR;
	}
	
	/** Return the C error and stores the message corresponding to the C++ exception
	 */
	PC_status_t return_err()
	{
		errmsg = "Unexpected error";
		if (handler.func) handler.func(PC_SYSTEM_ERROR, errmsg.c_str(), handler.context);
		return PC_SYSTEM_ERROR;
	}
	
}; // struct Error_context

/// The thread-local error context
thread_local Error_context g_error_context;

void assert_status(PC_status_t status, const char *message, void*)
{
	if (status) {
		fprintf(stderr, "Error in paraconf: %s\n", message);
		abort();
	}
}

const PC_errhandler_t PC_ASSERT_HANDLER = { assert_status, NULL };

const PC_errhandler_t PC_NULL_HANDLER = { NULL, NULL };

PC_errhandler_t PC_errhandler(PC_errhandler_t new_handler)
{
	PC_errhandler_t old_handler = g_error_context.handler;
	g_error_context.handler = new_handler;
	return old_handler;
}


const char* PC_errmsg() {
	return g_error_context.errmsg.c_str();
}

PC_status_t PC_status(PC_tree_t tree)
try
{
	if (tree == nullptr) {
		return PC_NODE_NOT_FOUND;
	}
	return tree->status() ? PC_OK : PC_NODE_NOT_FOUND;
} catch (const Error& e)
{
	return g_error_context.return_err(e);
} catch (const exception& e)
{
	return g_error_context.return_err(e);
} catch (...)
{
	return g_error_context.return_err();
}

PC_tree_t PC_parse_path(const char* path)
try
{
	return new PC_node{PC_load_file(path)};
} catch (const Error& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (const exception& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (...)
{
	g_error_context.return_err();
	return nullptr;
}

PC_tree_t PC_parse_string(const char* document)
try
{
	return new PC_node{PC_load(document)};
} catch (const Error& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (const exception& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (...)
{
	g_error_context.return_err();
	return nullptr;
}

PC_tree_t PC_parse_file(FILE* conf_file)
try
{
	fseek(conf_file, 0, SEEK_END);
	long file_size = ftell(conf_file);
	rewind(conf_file);
	
	std::unique_ptr<char[]> buffer {new char[file_size + 1]};
	size_t read_bytes = fread(buffer.get(), 1, file_size, conf_file);
	buffer[file_size] = '\0';

	if (read_bytes != file_size) {
		throw Error{PC_SYSTEM_ERROR, "Read only %zu of %ld bytes of yaml file: %s\n", read_bytes, file_size, strerror(errno)};
	}

	return PC_parse_string(buffer.get());
} catch (const Error& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (const exception& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (...)
{
	g_error_context.return_err();
	return nullptr;
}

PC_tree_t PC_get(const PC_tree_t tree, const char* index_fmt, ...)
try
{
	va_list ap;
	va_start(ap, index_fmt);
	PC_tree_t res = PC_vget(tree, index_fmt, ap);
	va_end(ap);
	return res;
} catch (const Error& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (const exception& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (...)
{
	g_error_context.return_err();
	return nullptr;
}

PC_tree_t PC_vget(const PC_tree_t tree, const char* index_fmt, va_list va)
try
{	
	int index_size = 256;
	unique_ptr<char> index {new char[index_size]};
	while (vsnprintf(index.get(), index_size, index_fmt, va) > index_size) {
		index_size *= 2;
		index.reset(new char[index_size]);
	}
	return PC_sget(tree, index.get());
} catch (const Error& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (const exception& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (...)
{
	g_error_context.return_err();
	return nullptr;
}


PC_tree_t PC_sget(PC_tree_t tree, const char* orig_index_c)
try
{	
	if (tree == nullptr || !tree->status()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get %s from empty tree", orig_index_c};
	}

	PC_tree_t new_tree;
	string orig_index = orig_index_c;
	string index = orig_index;
	while (!index.empty()) {
		switch ( index[0] ) {
			case '[':
				new_tree = get_seq_idx(tree, index);
				break;
			case '.': 
				new_tree = get_map_key_val(tree, index);
				break;
			case '{': 
				new_tree = get_map_idx_key(tree, index);
				break;
			case '<': 
				new_tree = get_map_idx_val(tree, index);
				break;
			default:
				throw Error{PC_INVALID_PARAMETER, "Invalid character `%c' at %d in `%s'\n", index[0], orig_index.size() - index.size(), orig_index_c};
		}
		tree = new_tree;
	}
	return tree;
} catch (const Error& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (const exception& e)
{
	g_error_context.return_err(e);
	return nullptr;
} catch (...)
{
	g_error_context.return_err();
	return nullptr;
}

PC_tree_type_t PC_type(PC_tree_t tree)
{
	if (tree == nullptr) {
		return PC_tree_type_t::PC_EMPTY;
	}
	return tree->type();
}

PC_status_t PC_line(PC_tree_t tree, int* line)
try {
	if (tree == nullptr || !tree->status()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get tree line for empty tree"};
	}
	*line = tree->line();
	return PC_OK;
} catch (const Error& e)
{
	return g_error_context.return_err(e);
} catch (const exception& e)
{
	return g_error_context.return_err(e);
} catch (...)
{
	return g_error_context.return_err();
}

PC_status_t PC_location(PC_tree_t tree, char** location)
try {
	if (tree == nullptr || !tree->status()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get tree location for empty tree"};
	}
	string location_string = tree->location();
	*location = (char*)malloc(location_string.size() * sizeof(char) + 1);
	strcpy(*location, location_string.c_str());
	(*location)[location_string.size()] = '\0';
	return PC_OK;
} catch (const Error& e)
{
	return g_error_context.return_err(e);
} catch (const exception& e)
{
	return g_error_context.return_err(e);
} catch (...)
{
	return g_error_context.return_err();
}

PC_status_t PC_len(const PC_tree_t tree, int* res)
try
{
	if (tree == nullptr  || !tree->status()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get size from empty tree"};
	}
	*res = tree->size();
	return PC_OK;
} catch (const Error& e)
{
	return g_error_context.return_err(e);
} catch (const exception& e)
{
	return g_error_context.return_err(e);
} catch (...)
{
	return g_error_context.return_err();
}

PC_status_t PC_int(const PC_tree_t tree, long* value)
try
{
	if (tree == nullptr || !tree->status()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot interpret empty tree as integer"};
	}
	try {
		*value = tree->as<long>();
		return PC_OK;
	} catch (const exception& e) {
		throw Error{PC_INVALID_NODE_TYPE, "In %s: Cannot interpret `%s' as integer", tree->location().c_str(), tree->as<string>().c_str()};
	} 
} catch (const Error& e)
{
	return g_error_context.return_err(e);
} catch (const exception& e)
{
	return g_error_context.return_err(e);
} catch (...)
{
	return g_error_context.return_err();
}

PC_status_t PC_double(const PC_tree_t tree, double* value)
try
{
	if (tree == nullptr || !tree->status()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot interpret empty tree as double"};
	}
	try{
		*value = tree->as<double>();
	} catch (const exception& e) {
		throw Error{PC_INVALID_NODE_TYPE, "In %s: Cannot interpret `%s; as double", tree->location().c_str(), tree->as<string>().c_str()};
	} 
	return PC_OK;
} catch (const Error& e)
{
	return g_error_context.return_err(e);
} catch (const exception& e)
{
	return g_error_context.return_err(e);
} catch (...)
{
	return g_error_context.return_err();
}

PC_status_t PC_string(const PC_tree_t tree, char** value)
try
{
	if (tree == nullptr || !tree->status()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot interpret empty tree as string"};
	}
	string result_str = tree->as<string>();
	*value = (char*)malloc(result_str.size() * sizeof(char) + 1);
	strcpy(*value, result_str.c_str());
	(*value)[result_str.size()] = '\0';
	return PC_OK;
} catch (const Error& e)
{
	return g_error_context.return_err(e);
} catch (const exception& e)
{
	return g_error_context.return_err(e);
} catch (...)
{
	return g_error_context.return_err();
}

PC_status_t PC_bool(const PC_tree_t tree, int* value)
try
{
	if (tree == nullptr || !tree->status()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot interpret empty tree as bool"};
	}
	try {
		*value = tree->as<bool>();
	} catch (const exception& e) {
		throw Error{PC_INVALID_NODE_TYPE, "In %s: Cannot interpret `%s' as bool", tree->location().c_str(), tree->as<string>().c_str()};
	} 
	return PC_OK;
} catch (const Error& e)
{
	return g_error_context.return_err(e);
} catch (const exception& e)
{
	return g_error_context.return_err(e);
} catch (...)
{
	return g_error_context.return_err();
}

PC_status_t PC_tree_destroy(PC_tree_t* tree)
try
{ 
	delete *tree;
	return PC_OK;
} catch (const Error& e)
{
	return g_error_context.return_err(e);
} catch (const exception& e)
{
	return g_error_context.return_err(e);
} catch (...)
{
	return g_error_context.return_err();
}
