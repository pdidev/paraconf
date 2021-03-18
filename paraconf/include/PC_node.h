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

#ifndef PC_NODE_H_
#define PC_NODE_H_

#include <yaml-cpp/yaml.h>

#include "paraconf_export.h"

/// Paraconf node containing yaml-cpp node
struct PARACONF_EXPORT PC_node {
private:
	/// Node from yaml-cpp library
	YAML::Node m_node;

	/// Acquired referenced to this node
	std::list<PC_node> m_acquired;

public:
	/** Creates paraconf node from yaml-cpp node
	 * \param[in] node yaml-cpp node
	 */
	PC_node(YAML::Node node);

	/** Deleted copy constructor
	 * \param[in] other node to not copy
	 */
	PC_node(const PC_node& other) = delete;

	/** Move constructor
	 * \param[in] other node to move
	 */
	PC_node(PC_node&& other);

	/** Deleted copy assignment operator
	 * \param[in] other node to not copy
	 * \return this object
	 */
	PC_node& operator =(const PC_node& other) = delete;

	/** Move assignment operator
	 * \param[in] other node to copy
	 * \return this object
	 */
	PC_node& operator =(PC_node&& other);

	/** Acquirees given node to this node.
	 *  Creates nested RAII.
	 * 
	 *  Given node will be stored in the list
	 *  and deleted in destructor of this node.
	 * 
	 * \param[in] node node to acquire
	 * \return pointer to acquired node
	 */
	PC_node* acquire(PC_node&& node);

	/** Return stored yaml-cpp node
	 * \return stored yaml-cpp node
	 */
	YAML::Node node() const;

	/** Returns status of the node
	 * \return true if node is defined, false otherwise
	 */
	bool status() const;

	/** Gets subnode of given index ( ypath [] operator ) 
	 * \param[in] index subnode index
	 * \return subnode of given index
	 */
	PC_node get(size_t index) const;

	/** Gets subnode of given key ( ypath . operator )
	 * \param[in] key subnode key
	 * \return subnode of given key
	 */
	PC_node get(const std::string& key) const;

	/** Gets subnode of given key index ( ypath {} operator )
	 * \param[in] index subnode key index
	 * \return subnode of given key index
	 */
	PC_node key(size_t index) const;

	/** Gets subnode of given value index ( ypath <> operator )
	 * \param[in] index subnode value index
	 * \return subnode of given value index
	 */
	PC_node value(size_t index) const;

	/** Returns size of the node
	 * 	
	 *  If the node is a scalar, the length will be returned
	 * 
	 * \return size of the node
	 */
	size_t size() const;

	/** Returns type of the node
	 * 
	 * \return type of the node
	 */
	YAML::NodeType::value type() const;

	/** Returns line of the node in document
	 * 
	 * \return line of hte node in document
	 */
	int line() const;

	/** Evaluate node as given type and return the value
	 * 
	 * \return node value as T type
	 */
	template<class T>
	T as() const
	{
		return m_node.as<T>();
	}
};

/** Creates PC_node from given yaml file path
 * 
 * \param[in] path path to yaml file
 */
PC_node PC_load_file(const std::string& path);

/** Creates PC_node from given yaml document
 * 
 * \param[in] document yaml document
 */
PC_node PC_load(const std::string& document);

#endif // PC_NODE_H_
