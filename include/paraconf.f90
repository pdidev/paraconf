!/*******************************************************************************
! * Copyright (c) 2015, Julien Bigot | Mohamed Gaalich - CEA (julien.bigot@cea.fr)
! * All rights reserved.
! *
! * Redistribution and use in source and binary forms, with or without
! * modification, are permitted provided that the following conditions are met:
! * * Redistributions of source code must retain the above copyright
! *   notice, this list of conditions and the following disclaimer.
! * * Redistributions in binary form must reproduce the above copyright
! *   notice, this list of conditions and the following disclaimer in the
! *   documentation and/or other materials provided with the distribution.
! * * Neither the name of CEA nor the names of its contributors may be used to
! *   endorse or promote products derived from this software without specific 
! *   prior written permission.
! *
! * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
! * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
! * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
! * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
! * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
! * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
! * THE SOFTWARE.
! ******************************************************************************/
MODULE PC_tree_t

	USE iso_C_binding

	TYPE, bind(C) :: PC_tree_t_f
		INTEGER(C_INT) :: status
		TYPE(C_PTR) :: document
		TYPE(C_PTR) :: node
	END TYPE PC_tree_t_f

END MODULE

MODULE paraconf

	USE iso_C_binding
	USE PC_tree_t

	IMPLICIT NONE

	INTERFACE
		FUNCTION PC_parse_path_f(path) &
			bind(C, name="PC_parse_path")   
			USE iso_C_binding 
			USE PC_tree_t
			TYPE(PC_tree_t_f) :: PC_parse_path_f
			TYPE(C_PTR), VALUE :: path
		END FUNCTION PC_parse_path_f
	END INTERFACE

	INTERFACE
		FUNCTION PC_get_f(tree,index_fmt) &
			bind(C, name="PC_get")   
			USE iso_C_binding 
			USE PC_tree_t
			TYPE(PC_tree_t_f) :: PC_get_f
			TYPE(PC_tree_t_f), VALUE :: tree
			TYPE(C_PTR), VALUE :: index_fmt
		END FUNCTION PC_get_f
	END INTERFACE

	INTERFACE
		FUNCTION PC_len_f(tree,value) &
			bind(C, name="PC_len")   
			USE iso_C_binding 
			USE PC_tree_t
			INTEGER(C_INT) :: PC_len_f
			TYPE(PC_tree_t_f), VALUE :: tree
			TYPE(C_PTR) :: value
		END FUNCTION PC_len_f
	END INTERFACE

	INTERFACE
		FUNCTION PC_int_f(tree,value) &
			bind(C, name="PC_int")   
			USE iso_C_binding 
			USE PC_tree_t
			INTEGER(C_INT) :: PC_int_f
			TYPE(PC_tree_t_f), VALUE :: tree
			TYPE(C_PTR) :: value
		END FUNCTION PC_int_f
	END INTERFACE

	INTERFACE
		FUNCTION PC_double_f(tree,value) &
			bind(C, name="PC_double")   
			USE iso_C_binding 
			USE PC_tree_t
			INTEGER(C_INT) :: PC_double_f
			TYPE(PC_tree_t_f), VALUE :: tree
			TYPE(C_PTR) :: value
		END FUNCTION PC_double_f
	END INTERFACE

	INTERFACE
		FUNCTION PC_string_f(tree,value) &
			bind(C, name="PC_string")   
			USE iso_C_binding 
			USE PC_tree_t
			INTEGER(C_INT) :: PC_string_f
			TYPE(PC_tree_t_f), VALUE :: tree
			TYPE(C_PTR) :: value
		END FUNCTION PC_string_f
	END INTERFACE

	CONTAINS 

	!=============================================================  
	SUBROUTINE PC_parse_path(path,tree)
		CHARACTER(LEN=*), INTENT(IN) :: path
		TYPE(PC_tree_t_f), INTENT(OUT) :: tree

		INTEGER :: i
		CHARACTER(C_CHAR), TARGET :: C_path(len_trim(path)+1)

		do i=1,len_trim(path)
      		C_path(i) = path(i:i)
    	end do
    	C_path(len_trim(path)+1) = C_NULL_CHAR

		tree = PC_parse_path_f(c_loc(C_path))

	END SUBROUTINE PC_parse_path
	!=============================================================
	!=============================================================  
	SUBROUTINE PC_get(tree_in,index_fmt,tree_out)
		TYPE(PC_tree_t_f), INTENT(IN) :: tree_in
		CHARACTER(LEN=*), INTENT(IN) :: index_fmt
		TYPE(PC_tree_t_f), INTENT(OUT) :: tree_out

		INTEGER :: i
		CHARACTER(C_CHAR), TARGET :: C_index_fmt(len_trim(index_fmt)+1)

		do i=1,len_trim(index_fmt)
      		C_index_fmt(i) = index_fmt(i:i)
    	end do
    	C_index_fmt(len_trim(index_fmt)+1) = C_NULL_CHAR

		tree_out = PC_get_f(tree_in,c_loc(C_index_fmt))

	END SUBROUTINE PC_get
	!=============================================================
	!=============================================================  
	SUBROUTINE PC_len(tree_in,value,status)
		TYPE(PC_tree_t_f), INTENT(IN) :: tree_in
		INTEGER, INTENT(OUT), POINTER :: value
		INTEGER, INTENT(OUT) :: status

		status = int(PC_len_f(tree_in,c_loc(value)))

	END SUBROUTINE PC_len
	!=============================================================
	!=============================================================  
	SUBROUTINE PC_int(tree_in,value,status)
		TYPE(PC_tree_t_f), INTENT(IN) :: tree_in
		INTEGER, INTENT(OUT), POINTER :: value
		INTEGER, INTENT(OUT) :: status

		status = int(PC_int_f(tree_in,c_loc(value)))

	END SUBROUTINE PC_int
	!=============================================================
	!=============================================================  
	SUBROUTINE PC_double(tree_in,value,status)
		TYPE(PC_tree_t_f), INTENT(IN) :: tree_in
		REAL(8), INTENT(OUT), POINTER :: value
		INTEGER, INTENT(OUT) :: status

		status = int(PC_double_f(tree_in,c_loc(value)))

	END SUBROUTINE PC_double
	!=============================================================
	!=============================================================  
	SUBROUTINE PC_string(tree_in,value,status)
		TYPE(PC_tree_t_f), INTENT(IN) :: tree_in
		CHARACTER(LEN=*), INTENT(OUT), POINTER :: value
		INTEGER, INTENT(OUT) :: status

		INTEGER :: i

		TYPE(C_PTR) :: C_value

		status = int(PC_string_f(tree_in,C_value))

		call c_f_pointer(C_value,value)


	END SUBROUTINE PC_string
	!=============================================================

END MODULE paraconf