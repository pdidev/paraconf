/* Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
 *               root of the project or at https://github.com/pdidev/paraconf
 * 
 * SPDX-License-Identifier: MIT
 */

#ifndef STATUS_H__
#define STATUS_H__

#include "paraconf.h"

#define PC_handle_err(callstatus, free_stamp)                                                                                                        \
	do {                                                                                                                                             \
		status = callstatus;                                                                                                                         \
		if (status) goto free_stamp;                                                                                                                 \
	} while (0)

#define PC_handle_tree(free_stamp)                                                                                                                   \
	do {                                                                                                                                             \
		if (PC_status(restree)) goto free_stamp;                                                                                                     \
	} while (0)

#define PC_handle_err_tree(callstatus, free_stamp)                                                                                                   \
	do {                                                                                                                                             \
		restree.status = callstatus;                                                                                                                 \
		if (PC_status(restree)) goto free_stamp;                                                                                                     \
	} while (0)

#define PC_handle_tree_err(calltree, free_stamp)                                                                                                     \
	do {                                                                                                                                             \
		status = calltree.status;                                                                                                                    \
		if (status) goto free_stamp;                                                                                                                 \
	} while (0)


// clang-format off
PC_status_t PC_make_err(PC_status_t status, const char* message, ...)
#if defined(__GNUC__) || defined(__clang__)
/// Lets the compiler check the arguments of a printf-like function against its format
__attribute__((format(printf, 2, 3)))
#endif
;
// clang-format on

/** Reports an error about a node, as PC_make_err does, with a message led by the position of the node: `file:line:column: error: '
 *
 * \param status the status of the error
 * \param tree the tree whose node the error is about, the message has no position if it has no node
 * \param message the printf-style format of the message
 * \return the status of the error
 */
// clang-format off
PC_status_t PC_make_node_err(PC_status_t status, PC_tree_t tree, const char* message, ...)
#if defined(__GNUC__) || defined(__clang__)
/// Lets the compiler check the arguments of a printf-like function against its format
__attribute__((format(printf, 3, 4)))
#endif
;
// clang-format on

/** Reports a failure to allocate memory, as PC_make_err would do with PC_SYSTEM_ERROR, but without allocating memory
 *
 * \return PC_SYSTEM_ERROR
 */
PC_status_t PC_make_malloc_err();

#endif // STATUS_H__
