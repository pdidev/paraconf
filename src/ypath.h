/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 * 
 * SPDX-License-Identifier: MIT
 */

#ifndef YPATH_H__
#define YPATH_H__

#include "paraconf.h"

struct PC_document_s {
	/// The underlying YAML document
	yaml_document_t document;
	/// The path to the file from which the document was parsed
	const char* path;
};

PC_tree_t PARACONF_EXPORT PC_sget(PC_tree_t tree, const char* index);

/** Returns the int value of a scalar node, as PC_int does, and reports one outside a range as an error
 *
 * Exported for the Fortran interface, whose integers are narrower than a long.
 *
 * \param[in] tree the int-valued node
 * \param[out] value the int value of the scalar node, left as it was on failure
 * \param[in] min the smallest value accepted
 * \param[in] max the largest value accepted
 * \return the status of the execution
 */
PC_status_t PARACONF_EXPORT PC_int_range(PC_tree_t tree, long* value, long min, long max);

#endif // YPATH_H__
