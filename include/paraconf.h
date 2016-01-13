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

#ifndef PARACONF_H__
#define PARACONF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdarg.h>
#include <yaml.h>
#include <mpi.h>

#include <paraconf_export.h>

/** \file paraconf.h
 */

typedef enum PC_errcode_e {
	/// no error
	PC_OK=0,
	/// a parameter value is invalid
	PC_INVALID_PARAMETER,
	/// unexpected type found for a node
	PC_INVALID_NODE_TYPE,
	// The requested node doen't exist in the tree
	PC_NODE_NOT_FOUND
} PC_errcode_t;

extern const char *const PC_errmessage[5];

typedef struct PC_status_s
{
	/// The tree status
	PC_errcode_t code;
	
	/// The error message
	char *errmsg;
	
} PC_status_t;

typedef void (PC_errfunc_f)(PC_status_t status);

typedef struct PC_tree_s
{
	/// The tree status
	PC_status_t status;
	
	PC_errfunc_f *errfunc;
	
	/// The document containing the tree
	yaml_document_t* document;
	
	/// the node inside the tree
	yaml_node_t* node;
	
} PC_tree_t;


static inline PC_errcode_t PC_status(PC_tree_t tree)
{
	return tree.status.code;
}

static inline char *PC_errmsg(PC_tree_t tree)
{
	return tree.status.errmsg;
}

/** Prints the error message and aborts if the status is invalid
 */
void PARACONF_EXPORT PC_assert(PC_status_t status);

/** Returns the tree at the root of a document
 * 
 * \param[out] status status of the command execution
 * \param[in] document the document
 * \return the tree, valid as long as the containing document is
 */
PC_tree_t PARACONF_EXPORT PC_root(yaml_document_t *document, PC_errfunc_f *errfunc);

/** Looks for a node in a yaml document given a ypath index
 * 
 * Does nothing if the provided tree is in error and returns the input tree.
 *
 * A ypath expression can contain the following
 * * access to a mapping element using the dot syntax:
 *   e.g. .map.key
 * * access to a sequence element using square brackets (indices are 0-based):
 *   e.g. .seq[1]
 * * access to a mapping element key using braces (indices are 0-based):
 *   e.g. .map{1}
 * * access to a mapping element value by index using chevrons:
 *   e.g. .map<1> 
 *   PC_get(0,map,"<1>"); is similar to k=PC_get(0,map,"{1}"); PC_get(0,map,".%s",k);
 * 
 * \param[in,out] status status of the command execution, does nothing if not valid in input
 * \param[in] tree a yaml tree
 * \param[in] index_fmt the ypath index, can be a printf-style format string
 * \param[in] ... the printf-style values
 * \return the subtree corresponding to the ypath index
 */
PC_tree_t PARACONF_EXPORT PC_get(PC_tree_t tree, const char *index_fmt, ...);

/** Looks for a node in a yaml document given a ypath index
 * 
 * Does nothing if the provided tree is in error
 * 
 * \param[in,out] status status of the command execution, does nothing if not valid in input
 * \param[in] tree a yaml tree
 * \param[in] index_fmt the ypath index, can be a printf-style format string
 * \param[in] va the printf-style values
 * \return the subtree corresponding to the ypath index
 */
PC_tree_t PARACONF_EXPORT PC_vget(PC_tree_t tree, const char *index_fmt, va_list va);

/** Returns the length of a node, for a sequence, the number of nodes, for a mapping, the number of pairs, for a scalar, the string length
 * 
 * Does nothing if the provided tree is in error
 * 
 * \param[in,out] status status of the command execution, does nothing if not valid in input
 * \param[in] tree the sequence or mapping
 * \param[out] value the length
 * \return the status of the execution (valid until the next PC_* call in the same thread)
 */
PC_status_t PARACONF_EXPORT PC_len(PC_tree_t tree, int *value);

/** Returns the int value of a scalar node
 * 
 * Does nothing if the provided tree is in error
 * 
 * \param[in,out] status status of the command execution, does nothing if not valid in input
 * \param[in] tree the int-valued node
 * \param[out] value the int value of the scalar node
 * \return the status of the execution (valid until the next PC_* call in the same thread)
 */
PC_status_t PARACONF_EXPORT PC_int(PC_tree_t tree, int *value);

/** Returns the floating point value of a scalar node
 * 
 * Does nothing if the provided tree is in error
 * 
 * \param[in,out] status status of the command execution, does nothing if not valid in input
 * \param[in] tree the floating-point-valued node
 * \param[out] value the floating point value of the scalar node
 * \return the status of the execution (valid until the next PC_* call in the same thread)
 */
PC_status_t PARACONF_EXPORT PC_double(PC_tree_t tree, double *value);

/** Returns the string content of a scalar node
 * 
 * Does nothing if the provided tree is in error
 * 
 * \param[in,out] status status of the command execution, does nothing if not valid in input
 * \param[in] tree the node
 * \param[out] value the content of the scalar node as a newly allocated string that must be deallocated using free
 * \return the status of the execution (valid until the next PC_* call in the same thread)
 */
PC_status_t PARACONF_EXPORT PC_string(PC_tree_t tree, char **value);

/** Broadcasts yaml documents over MPI
 * 
 * \param[in,out] document an array of documents that should already be allocated.
 *                         Their content will be copied from the root node.
 * \param[in] count the number of documents in the array
 * \param[in] root the rank of the root node
 * \param[in] comm the set of precesses over wich to broadcast
 */
PC_status_t PARACONF_EXPORT PC_broadcast(yaml_document_t* document, int count, int root, MPI_Comm comm);

#ifdef __cplusplus
}
#endif

#endif // PARACONF_H__
