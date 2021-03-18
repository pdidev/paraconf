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

#ifndef PC_ERROR_H_
#define PC_ERROR_H_

#include <exception>
#include <string>

#include "paraconf.h"

#include "paraconf_export.h"

namespace PC {

/// Paraconf Error
class PARACONF_EXPORT Error : public std::exception
{
protected:
	/// Status of the Error
	PC_status_t m_status;
	
	/// Message of the Error
	std::string m_what;
	
public:
	/** Creates a paraconf error
	 * \param[in] errcode the error code of the error to create
	 * \param[in] message an errror message as a printf-style format
	 * \param[in] ... the printf-style parameters for the message
	 * \see printf
	 */
	Error(PC_status_t errcode = PC_OK, const char* message = "", ...);
	
	/** Creates a paraconf error
	 * \param[in] errcode the error code of the error to create
	 * \param[in] message an errror message as a printf-style format
	 * \param[in] va the printf-style values
	 * \see printf
	 */
	Error(PC_status_t errcode, const char* message, va_list args);
	
	const char* what() const noexcept override;
	
	/** Returns status of the Error
	 * \return status of the Error
	 */
	PC_status_t status() const noexcept;
};

} // namespace PC

#endif // PC_ERROR_H_
