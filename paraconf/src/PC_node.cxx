/*******************************************************************************
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

#include <sys/stat.h>
#include <yaml-cpp/node/node.h>

#include "paraconf/error.h"

#include "paraconf/PC_node.h"

using PC::Error;
using std::exception;
using std::move;
using std::string;
using std::to_string;
using std::unique_ptr;
using YAML::Clone;
using YAML::LoadFile;
using YAML::Load;
using YAML::Node;

PC_node::PC_node() = default;

PC_node::PC_node(Node node, const string& filename):
	m_node{node},
	m_filename{filename}
{}

PC_node::PC_node(PC_node&& other):
	m_node{move(other.m_node)},
	m_filename{move(other.m_filename)}
{}

PC_node& PC_node::operator =(PC_node&& other)
{
	m_node = move(other.m_node);
	m_filename = move(other.m_filename);
	return *this;
}

PC_node PC_node::operator [](size_t index) const
{
	return get(index);
}

PC_node PC_node::operator [](const std::string& key) const
{
	return get(key);
}

PC_node::Iterator::Iterator(const PC_node* node):
	m_node{node},
	m_index{0}
{}

PC_node::Iterator::Iterator(size_t index):
	m_node{nullptr},
	m_index{index}
{}

PC_node PC_node::Iterator::operator *()
{
	auto it = m_node->node().begin();
	for (int i = 0; i < m_index; i++) {
		++it;
	}
	return *it;
}

unique_ptr<PC_node> PC_node::Iterator::operator ->()
{
	return unique_ptr<PC_node>{new PC_node{this->operator *()}};
}

string PC_node::Iterator::key() const
{
	if (m_node->type() != PC_tree_type_t::PC_MAP) {
		throw Error{PC_INVALID_NODE_TYPE, "Cannot get key from iterator of non map node"};
	}
	auto it = m_node->node().begin();
	for (int i = 0; i < m_index; i++) {
		++it;
	}
	return it->first.as<string>();
}

PC_node PC_node::Iterator::value() const
{
	if (m_node->type() != PC_tree_type_t::PC_MAP) {
		throw Error{PC_INVALID_NODE_TYPE, "Cannot get value from iterator of non map node"};
	}
	auto it = m_node->node().begin();
	for (int i = 0; i < m_index; i++) {
		++it;
	}
	return it->second;
}

PC_node::Iterator& PC_node::Iterator::operator ++()
{
	m_index++;
	return *this;
}

bool PC_node::Iterator::operator !=(const Iterator& it) const
{
	return m_index != it.m_index;
}

PC_node::Iterator PC_node::begin() const
{
	return PC_node::Iterator{this};
}
	
PC_node::Iterator PC_node::end() const
{
	auto it = m_node.begin();
	size_t i = 0; 
	for (; it != m_node.end(); i++) {
		it++;
	}
	return PC_node::Iterator{i};
}

PC_node* PC_node::acquire(PC_node&& node)
{
	m_acquired.emplace_back(move(node));
	return &m_acquired.back();
}

Node PC_node::node() const
{
	return m_node;
}

bool PC_node::status() const
{
	return m_node.IsDefined();
}

PC_node PC_node::postprocess_result(PC_node node) const
{
	if (node.node().Tag() == "!include") {
		return PC_load_file(node.as<string>());
	}
	node.m_filename = m_filename;
	return node;
}

PC_node PC_node::get(size_t index) const
{
	if (m_node.Type() != YAML::NodeType::Sequence) {
		throw Error{PC_INVALID_NODE_TYPE, "In %s: Cannot access index `%zu' of not sequence tree", location().c_str(), index};
	}
	if (index >= m_node.size()) {
		throw Error{PC_NODE_NOT_FOUND, "In %s: Index out of range: %zu (sequence size: %zu)", location().c_str(), index, m_node.size()};
	}
	PC_node result;
	try {
		result = m_node[index];
	} catch (const exception& e) {
		throw Error{PC_SYSTEM_ERROR, "In %s: %s", location().c_str(), e.what()};
	}
	return postprocess_result(m_node[index]);
}

PC_node PC_node::get(const string& key) const
{
	if (m_node.Type() != YAML::NodeType::Map) {
		throw Error{PC_INVALID_NODE_TYPE, "In %s: Cannot access key `%s' of not map tree", location().c_str(), key.c_str()};
	}
	PC_node result;
	try {
		result = m_node[key];
	} catch (const exception& e) {
		throw Error{PC_SYSTEM_ERROR, "In %s: %s", location().c_str(), e.what()};
	}
	return postprocess_result(m_node[key]);
}

PC_node PC_node::key(size_t index) const
{
	if (m_node.Type() != YAML::NodeType::Map) {
		throw Error{PC_INVALID_NODE_TYPE, "In %s: Cannot access key map index `%zu' of not map tree", location().c_str(), index};
	}
	if (index >= m_node.size()) {
		throw Error{PC_NODE_NOT_FOUND, "In %s: Key map index out of range: %zu (map size: %zu)", location().c_str(), index, m_node.size()};
	}
	auto it = m_node.begin();
	for (size_t i = 0; i < index && it != m_node.end(); i++) {
		it++;
	}
	return postprocess_result(it->first);
}

PC_node PC_node::value(size_t index) const
{
	if (m_node.Type() != YAML::NodeType::Map) {
		throw Error{PC_INVALID_NODE_TYPE, "In %s: Cannot access value map index `%zu' of not map tree", location().c_str(), index};
	}
	if (index >= m_node.size()) {
		throw Error{PC_NODE_NOT_FOUND, "In %s: Value map index out of range: %zu (map size: %zu)", location().c_str(), index, m_node.size()};
	}
	auto it = m_node.begin();
	for (int i = 0; i < index && it != m_node.end(); i++) {
		it++;
	}
	return postprocess_result(it->second);
}

size_t PC_node::size() const
{
	if (!m_node.IsDefined()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get size of not defined tree"};
	}
	if (m_node.IsScalar()) {
		return m_node.Scalar().length();
	}
	return m_node.size();
}

PC_tree_type_t PC_node::type() const
{
	if (!m_node.IsDefined()) {
		return PC_tree_type_t::PC_EMPTY;
	}
	switch (m_node.Type()) {
		case YAML::NodeType::Null:
			return PC_tree_type_t::PC_EMPTY;
		case YAML::NodeType::Scalar:
			return PC_tree_type_t::PC_SCALAR;
		case YAML::NodeType::Sequence:
			return PC_tree_type_t::PC_SEQUENCE;
		case YAML::NodeType::Map:
			return PC_tree_type_t::PC_MAP;
		default:
			return PC_tree_type_t::PC_UNDEFINED;
	}
}

int PC_node::line() const
{
	if (!m_node.IsDefined()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get line of not defined tree"};
	}
	return m_node.Mark().line + 1;
}

string PC_node::location() const
{
	if (!m_node.IsDefined()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get location of not defined tree"};
	}
	if (m_filename.empty()) {
		return "line " + to_string(line());
	}
	return "`" + m_filename + "' file, line " + to_string(line());
}

PC_node PC_load_file(const string& path)
{
	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0) {
		throw Error{PC_SYSTEM_ERROR, "File `%s' doesn't exist", path.c_str()};
	}
	return {LoadFile(path), path};
}

PC_node PC_load(const string& document)
{
	return Load(document);
}
