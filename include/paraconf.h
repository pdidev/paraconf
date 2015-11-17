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

#include <stdarg.h>
#include <yaml.h>
#include <mpi.h>

#include "paraconf_export.h"

typedef enum {
	/// no error
	PC_OK=0,
	/// a parameter value is invalid
	PC_INVALID_PARAMETER,
	/// unexpected type found for a node
	PC_INVALID_NODE_TYPE,
	PC_NODE_NOT_FOUND,
	/// The provided buffer is not large enough
	PC_ERR_BUFFER_SIZE
} PC_status_t;

/** Looks for a node in a yaml document given a ypath index
 * 
 * \param[in] document the yaml document
 * \param[in] index the ypath index
 * \param[out] value the node found
 * \return error status
 */
PC_status_t PARACONF_EXPORT PC_get(yaml_document_t* document, yaml_node_t* node, const char* index, yaml_node_t** value, ...);

PC_status_t PARACONF_EXPORT PC_vget(yaml_document_t* document, yaml_node_t* node, const char* index, yaml_node_t** value, va_list va);

/** Looks for a sequence or mapping node in a yaml document given a ypath index and returns its size/length
 * 
 * \param[in] document the yaml document
 * \param[in] index the ypath index
 * \param[out] value the number of elements in the mapping/sequence
 * \return error status
 */
PC_status_t PARACONF_EXPORT PC_get_len(yaml_document_t* document, yaml_node_t* node, const char* index, int* len, ...);

PC_status_t PARACONF_EXPORT PC_vget_len(yaml_document_t* document, yaml_node_t* node, const char* index, int* len, va_list va);

/** Looks for an integer value in a yaml document given a ypath index
 * 
 * \param[in] document the yaml document
 * \param[in] index the ypath index
 * \param[out] value the integer value found
 * \return error status
 */
PC_status_t PARACONF_EXPORT PC_get_int(yaml_document_t* document, yaml_node_t* node, const char *index, int *value, ...);

PC_status_t PARACONF_EXPORT PC_vget_int(yaml_document_t* document, yaml_node_t* node, const char *index, int *value, va_list va);

/** Looks for a floating point value in a yaml document given a ypath index
 * 
 * \param[in] document the yaml document
 * \param[in] index the ypath index
 * \param[out] value the floating point value found
 * \return error status
 */
PC_status_t PARACONF_EXPORT PC_get_double(yaml_document_t* document, yaml_node_t* node, const char *index, double *value, ...);

PC_status_t PARACONF_EXPORT PC_vget_double(yaml_document_t* document, yaml_node_t* node, const char *index, double *value, va_list va);

/** Looks for a character string value in a yaml document given a ypath index
 * 
 * \param[in] document the yaml document
 * \param[in] index the ypath index
 * \param[out] value the character string value found
 * \param[in,out] value_len the length of value (if not NULL)
 * \return error status
 * 
 * There are 3 cases regarding the memory allocation for value:
 * * if value_len == NULL, a new buffer is allocated for value and returned, the old buffer is discarded
 * * if *value_len > 0 the buffer is used as-is if large enough, PC_ERR_BUFFER_SIZE is returned otherwise
 * * if *value_len <= 0 the actual size is the absolute value and the string can be reallocated
 */
PC_status_t PARACONF_EXPORT PC_get_string(yaml_document_t* document, yaml_node_t* node, const char *index, char **value, int *value_len, ...);

PC_status_t PARACONF_EXPORT PC_vget_string(yaml_document_t* document, yaml_node_t* node, const char *index, char **value, int *value_len, va_list va);

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
