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
#include <yaml-cpp/yaml.h>

#include "paraconf.h"
#include "paraconf/error.h"
#include "paraconf_export.h"

/// Paraconf node containing yaml-cpp node
struct PARACONF_EXPORT PC_node {
private:
	/// Node from yaml-cpp library
	YAML::Node m_node;

	/// Filename if yaml was loaded from file
	std::string m_filename;

	/// Acquired referenced to this node
	std::list<PC_node> m_acquired;

	/** Checks if returned node is tagged as include
	 *  and sets the same filename
	 * 
	 * \param[in] node node to postprocess
	 * 
	 * \return postprocessed node
	 */
	PC_node postprocess_result(PC_node node) const;

public:
	/// PC_node iterator
	class Iterator {
		friend class PC_node;

		/// Begin node of yaml-cpp
		const PC_node* m_node;

		/// Node index from the begin node of yaml-cpp
		size_t m_index;

		/// Creates new iterator from node with index 0 (used in begin())
		Iterator(const PC_node* node);

		/// Creates new iterator with no node, but with index (used in end())
		Iterator(size_t index);
	public:
		/** Dereference iterator
		 * 
		 * \return dereferenced iterator
		 */
		PC_node operator *();

		/** Returns node pointer
		 * 
		 * \return node pointer
		 */
		std::unique_ptr<PC_node> operator ->();

		/** Returns map key if the node is a map
		 * 
		 * \return node key
		 */
		std::string key() const;

		/** Returns map value if the node is a map
		 * 
		 * \return node value
		 */
		PC_node value() const;

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
	PC_node();

	/** Creates paraconf node from yaml-cpp node
	 * \param[in] node yaml-cpp node
	 */
	PC_node(YAML::Node node, const std::string& filename = "");

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

	/** Gets subnode of given index ( ypath [] operator ) 
	 * \param[in] index subnode index
	 * \return subnode of given index
	 */
	PC_node operator [](size_t index) const;

	/** Gets subnode of given key ( ypath . operator )
	 * \param[in] key subnode key
	 * \return subnode of given key
	 */
	PC_node operator [](const std::string& key) const;

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
	PC_tree_type_t type() const;

	/** Returns line of the node in document
	 * 
	 * \return line of the node in document
	 */
	int line() const;

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

/** Creates PC_node from given yaml file path
 * 
 * \param[in] path path to yaml file
 */
PC_node PARACONF_EXPORT PC_load_file(const std::string& path);

/** Creates PC_node from given yaml document
 * 
 * \param[in] document yaml document
 */
PC_node PARACONF_EXPORT PC_load(const std::string& document);

#endif // PC_NODE_H_
