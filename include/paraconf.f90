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

  use iso_C_binding

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
      INTEGER(C_INT) :: value
    END FUNCTION PC_len_f
  END INTERFACE

  INTERFACE
    FUNCTION PC_int_f(tree,value) &
      bind(C, name="PC_int")   
      USE iso_C_binding 
      USE PC_tree_t
      INTEGER(C_INT) :: PC_int_f
      TYPE(PC_tree_t_f), VALUE :: tree
      INTEGER(C_INT) :: value
    END FUNCTION PC_int_f
  END INTERFACE

  INTERFACE
    FUNCTION PC_double_f(tree,value) &
      bind(C, name="PC_double")   
      USE iso_C_binding 
      USE PC_tree_t
      INTEGER(C_INT) :: PC_double_f
      TYPE(PC_tree_t_f), VALUE :: tree
      REAL(C_DOUBLE) :: value
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




 END MODULE paraconf