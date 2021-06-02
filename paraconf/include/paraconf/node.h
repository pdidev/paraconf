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

#include <exception>
#include <forward_list>
#include <yaml-cpp/yaml.h>

#include "paraconf.h"
#include "paraconf/error.h"
#include "paraconf_export.h"

/// Paraconf node containing yaml-cpp node
namespace PC {

struct PARACONF_EXPORT Node {
private:
	/// Node from yaml-cpp library
	YAML::Node m_node;

	/// Filename if yaml was loaded from file
	std::string m_filename;

	/// Acquired referenced to this node
	std::forward_list<Node> m_sub_nodes;

public:
	/// Node iterator
	class Iterator {
		friend class Node;

		/// Node to iterate over
		const Node* m_node;

		/// Node index from the begin node of yaml-cpp
		YAML::const_iterator m_real_iterator;

		/// Creates new iterator from node (begin() iterator)
		Iterator(const Node* node);

		/// Creates new iterator from node and real iterator
		Iterator(const Node* node, YAML::const_iterator real_iterator);
	public:

		/** Returns map key if the node is a map
		 * 
		 * \return node key
		 */
		std::string key() const;

		/** Returns map value if the node is a map
		 * 
		 * \return node value
		 */
		Node value() const;

		/** Evaluate node as given type and return the value
		 * 
		 * \return node value as T type
		 */
		template<class T>
		T as() const
		{
			return m_real_iterator->as<T>();
		}

		/** Dereference iterator
		 * 
		 * \return dereferenced iterator
		 */
		Node operator *();

		/** Increments iterator
		 * 
		 * \return updated iterator
		 */
		Iterator& operator ++();

		/** Iterator comparator
		 * 
		 * \return true if iterators are different, false otherwise
		 */
		bool operator !=(const Iterator&) const;
	};

	/// Creates empty node
	Node();

	/** Creates paraconf node from yaml-cpp node
	 * \param[in] node yaml-cpp node
	 */
	Node(YAML::Node node, const std::string& filename = "");

	/** Deleted copy constructor
	 * \param[in] other node to not copy
	 */
	Node(const Node& other) = delete;

	/** Move constructor
	 * \param[in] other node to move
	 */
	Node(Node&& other);

	/** Deleted copy assignment operator
	 * \param[in] other node to not copy
	 * \return this object
	 */
	Node& operator =(const Node& other) = delete;

	/** Move assignment operator
	 * \param[in] other node to copy
	 * \return this object
	 */
	Node& operator =(Node&& other);

	/** Gets subnode of given index ( ypath [] operator ) 
	 * \param[in] index subnode index
	 * \return subnode of given index
	 */
	Node operator [](size_t index) const;

	/** Gets subnode of given key ( ypath . operator )
	 * \param[in] key subnode key
	 * \return subnode of given key
	 */
	Node operator [](const std::string& key) const;

	/** Returns begin iterator
	 * 
	 * \return begin iterator
	 */
	Iterator begin() const;
	
	/** Returns end iterator
	 * 
	 * \return end iterator
	 */
	Iterator end() const;

	/** Acquirees given node to this node.
	 *  Creates nested RAII.
	 * 
	 *  Given node will be stored in the list
	 *  and deleted in destructor of this node.
	 * 
	 * \param[in] node node to acquire
	 * \return pointer to acquired node
	 */
	Node* acquire(Node&& node);

	/** Return stored yaml-cpp node
	 * \return stored yaml-cpp node
	 */
	YAML::Node yaml_node() const;

	/** Returns status of the node
	 * \return true if node is defined, false otherwise
	 */
	bool status() const;

	/** Gets subnode of given index ( ypath [] operator ) 
	 * \param[in] index subnode index
	 * \return subnode of given index
	 */
	Node get(size_t index) const;

	/** Gets subnode of given key ( ypath . operator )
	 * \param[in] key subnode key
	 * \return subnode of given key
	 */
	Node get(const std::string& key) const;

	/** Gets subnode of given key index ( ypath {} operator )
	 * \param[in] index subnode key index
	 * \return subnode of given key index
	 */
	Node key(size_t index) const;

	/** Gets subnode of given value index ( ypath <> operator )
	 * \param[in] index subnode value index
	 * \return subnode of given value index
	 */
	Node value(size_t index) const;

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
	PC_tree_type_t type() const;

	/** Returns line of the node in document
	 * 
	 * \return line of the node in document
	 */
	int line() const;

	/** Returns message with node filename
	 * 
	 * \return message with node filename, empty if yaml created from string
	 */
	std::string filename() const;

	/** Returns message with node location in yaml
	 * 
	 * If filename is defined the message is: "`<filename>' file, line <line>".
	 * If filename is not defined, the message is: "line <line>".
	 * 
	 * \return message with node location in yaml
	 */
	std::string location() const;

	/** Evaluate node as given type and return the value
	 * 
	 * \return node value as T type
	 */
	template<class T>
	T as() const
	{
		try {
			T result = m_node.as<T>();
		} catch (const std::exception& e) {
			throw PC::Error{PC_SYSTEM_ERROR, "In %s: %s", location().c_str(), e.what()};
		}

		return m_node.as<T>();
	}
};

/** Creates Node from given yaml file path
 * 
 * \param[in] path path to yaml file
 */
Node PARACONF_EXPORT Load_file(const std::string& path);

/** Creates Node from given yaml document
 * 
 * \param[in] document yaml document
 */
Node PARACONF_EXPORT Load(const std::string& document);

} // namespace PC

#endif // PC_NODE_H_
