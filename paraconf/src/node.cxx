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

#include "paraconf/node.h"

using PC::Error;
using std::exception;
using std::move;
using std::string;
using std::to_string;
using std::unique_ptr;

namespace PC {

Node::Node() = default;

Node::Node(YAML::Node node, const string& filename):
	m_node{node},
	m_filename{filename}
{}

Node::Node(Node&& other) = default;

Node& Node::operator =(Node&& other)
{
	m_node = move(other.m_node);
	m_filename = move(other.m_filename);
	return *this;
}

Node Node::operator [](size_t index) const
{
	return get(index);
}

Node Node::operator [](const std::string& key) const
{
	return get(key);
}

Node::Iterator::Iterator(const Node* node):
	m_node{node},
	m_real_iterator{m_node->yaml_node().begin()}
{}

Node::Iterator::Iterator(const Node* node, YAML::const_iterator real_iterator):
	m_node{node},
	m_real_iterator{real_iterator}
{}

string Node::Iterator::key() const
{
	return m_real_iterator->first.as<string>();
}

Node Node::Iterator::value() const
{
	return m_node->postprocess_result(Node{m_real_iterator->second});
}


Node Node::Iterator::operator *()
{
	return {*m_real_iterator};
}

Node::Iterator& Node::Iterator::operator ++()
{
	m_real_iterator++;
	return *this;
}

bool Node::Iterator::operator !=(const Iterator& it) const
{
	return m_real_iterator != it.m_real_iterator;
}

Node::Iterator Node::begin() const
{
	return Node::Iterator{this};
}
	
Node::Iterator Node::end() const
{
	return Node::Iterator{this, yaml_node().end()};
}

Node* Node::acquire(Node&& node)
{
	m_sub_nodes.emplace_front(move(node));
	return &m_sub_nodes.front();
}

YAML::Node Node::yaml_node() const
{
	return m_node;
}

bool Node::status() const
{
	return m_node.IsDefined();
}

Node Node::postprocess_result(Node new_node) const
{
	if (new_node.yaml_node().Tag() == "!include") {
		return Load_file(new_node.as<string>());
	}
	new_node.m_filename = m_filename;
	return new_node;
}

Node Node::get(size_t index) const
{
	if (m_node.Type() != YAML::NodeType::Sequence) {
		throw Error{PC_INVALID_NODE_TYPE, "In %s: Cannot access index `%zu' of not sequence tree", location().c_str(), index};
	}
	if (index >= m_node.size()) {
		throw Error{PC_NODE_NOT_FOUND, "In %s: Index out of range: %zu (sequence size: %zu)", location().c_str(), index, m_node.size()};
	}
	Node result;
	try {
		result = m_node[index];
	} catch (const exception& e) {
		throw Error{PC_SYSTEM_ERROR, "In %s: %s", location().c_str(), e.what()};
	}
	return postprocess_result(m_node[index]);
}

Node Node::get(const string& key) const
{
	if (m_node.Type() != YAML::NodeType::Map) {
		throw Error{PC_INVALID_NODE_TYPE, "In %s: Cannot access key `%s' of not map tree", location().c_str(), key.c_str()};
	}
	Node result;
	try {
		result = m_node[key];
	} catch (const exception& e) {
		throw Error{PC_SYSTEM_ERROR, "In %s: %s", location().c_str(), e.what()};
	}
	return postprocess_result(m_node[key]);
}

Node Node::key(size_t index) const
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

Node Node::value(size_t index) const
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

size_t Node::size() const
{
	if (!m_node.IsDefined()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get size of not defined tree"};
	}
	if (m_node.IsScalar()) {
		return m_node.Scalar().length();
	}
	return m_node.size();
}

PC_tree_type_t Node::type() const
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

int Node::line() const
{
	if (!m_node.IsDefined()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get line of not defined tree"};
	}
	return m_node.Mark().line + 1;
}

string Node::filename() const
{
	if (!m_node.IsDefined()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get location of not defined tree"};
	}
	return m_filename;
}

string Node::location() const
{
	if (!m_node.IsDefined()) {
		throw Error{PC_NODE_NOT_FOUND, "Cannot get location of not defined tree"};
	}
	if (m_filename.empty()) {
		return "line " + to_string(line());
	}
	return "`" + m_filename + "' file, line " + to_string(line());
}

Node Load_file(const string& path)
{
	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0) {
		throw Error{PC_SYSTEM_ERROR, "File `%s' doesn't exist", path.c_str()};
	}
	return {YAML::LoadFile(path), path};
}

Node Load(const string& document)
{
	return YAML::Load(document);
}

} // namespace PC
