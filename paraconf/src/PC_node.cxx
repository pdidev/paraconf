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

#include "PC_node.h"

#include <iostream>

using PC::Error;
using std::move;
using std::string;
using std::unique_ptr;
using YAML::Clone;
using YAML::LoadFile;
using YAML::Load;
using YAML::Node;

PC_node::PC_node(Node node):
	m_node{node}
{}

PC_node::PC_node(PC_node&& other):
	m_node{move(other.m_node)}
{}

PC_node& PC_node::operator =(PC_node&& other)
{
	m_node = move(other.m_node);
	return *this;
}

PC_node* PC_node::acquire(PC_node&& node)
{
	m_acquired.emplace_back(std::move(node));
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

PC_node PC_node::get(size_t index) const
{
	return m_node[index];
}

PC_node PC_node::get(const std::string& key) const
{
	return m_node[key];
}

PC_node PC_node::key(size_t index) const
{
	auto it = m_node.begin();
	for (size_t i = 0; i < index && it != m_node.end(); i++) {
		it++;
	}
	if (it == m_node.end()) {
		throw Error{PC_NODE_NOT_FOUND, "In line: %d: Key map index out of range: %zu (map size: %zu)", line(), index, m_node.size()};
	}
	return it->first;
}

PC_node PC_node::value(size_t index) const
{
	auto it = m_node.begin();
	for (int i = 0; i < index && it != m_node.end(); i++) {
		it++;
	}
	if (it == m_node.end()) {
		throw Error{PC_NODE_NOT_FOUND, "In line: %d: Value map index out of range: %zu (map size: %zu)", line(), index, m_node.size()};
	}
	return it->second;
}

size_t PC_node::size() const
{
	if (m_node.IsScalar()) {
		return m_node.Scalar().length();
	}
	return m_node.size();
}

YAML::NodeType::value PC_node::type() const
{
	return m_node.Type();
}

int PC_node::line() const
{
	return m_node.Mark().line + 1;
}

PC_node PC_load_file(const string& path)
{
	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0) {
		throw Error{PC_SYSTEM_ERROR, "File `%s' doesn't exist", path.c_str()};
	}
	return LoadFile(path);
}

PC_node PC_load(const string& document)
{
	return Load(document);
}
