!******************************************************************************
! Copyright (C) 2015-2018 Commissariat a l'energie atomique et aux energies
! alternatives (CEA)
! Copyright (C) 2021 Institute of Bioorganic Chemistry Polish Academy of Science (PSNC)
! All rights reserved.
!
! Redistribution and use in source and binary forms, with or without
! modification, are permitted provided that the following conditions are met:
! * Redistributions of source code must retain the above copyright
!   notice, this list of conditions and the following disclaimer.
! * Redistributions in binary form must reproduce the above copyright
!   notice, this list of conditions and the following disclaimer in the
!   documentation and/or other materials provided with the distribution.
! * Neither the name of CEA nor the names of its contributors may be used to
!   endorse or promote products derived from this software without specific
!   prior written permission.
!
! THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
! IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
! FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
! AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
! LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
! OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
! THE SOFTWARE.
!******************************************************************************

program example
  use paraconf

  type(pc_tree_t) :: tree1, tree_tmp
  integer :: a_int, tree_type, line
  character(20) :: a_string
  real(8) :: a_float
  logical :: a_log
  integer :: ierr
  character(len=pc_errmsg_maxlength) :: errmsg
  character(len=4096) :: infile

  if (command_argument_count() /= 2) then
    print *, "Error: expected 1 argument!"
    error stop
  endif

  call get_command_argument(1,infile)   !first, read in the two values
  
  call PC_parse_path(infile, tree1)

  ! null tree
  if (PC_status(tree_tmp) /= PC_NODE_NOT_FOUND) then
    print *, "error with null tree, ", PC_status(tree_tmp)
    error stop
  endif
  if (PC_type(tree_tmp) /= PC_EMPTY) then
    print *, "error with a_int type, ", PC_type(tree_tmp)
    error stop
  endif

  ! empty tree
  call get_command_argument(2,infile)   !first, read in the two values
  call PC_parse_path(infile, tree_tmp);
  if (PC_status(tree_tmp) /= PC_OK) then
    print *, "error with null tree, ", PC_status(tree_tmp)
    error stop
  endif
  if (PC_type(tree_tmp) /= PC_EMPTY) then
    print *, "error with a_int type, ", PC_type(tree_tmp)
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_int")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_int type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 2) then
    print *, "error with a_int line, ", line
  endif
  call PC_int(tree_tmp, a_int)
  if ( a_int /= 100 ) then
    print *, "error with a_int, ", a_int
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_float")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_float type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 4) then
    print *, "error with a_float line, ", line
  endif
  call PC_double(tree_tmp, a_float)
  if ( abs(a_float-100.1d0) > 1.0e-10 ) then
    print *, "error with a_float, ", a_float
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_string")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_string type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 6) then
    print *, "error with a_string line, ", line
  endif
  call PC_string(tree_tmp, a_string)
  if ( a_string /= "this is a string" ) then
    print *, "error with a_string, ", a_string
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_list")
  if (PC_type(tree_tmp) /= PC_SEQUENCE) then
    print *, "error with a_list type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 8) then
    print *, "error with a_list line, ", line
  endif
  call PC_len(tree_tmp, a_int)
  if ( a_int /= 2 ) then
    print *, "error with a_list len, ", a_int
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_list[0]")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_list[0] type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 8) then
    print *, "error with a_list[0] line, ", line
  endif
  call PC_int(tree_tmp, a_int)
  if ( a_int /= 10 ) then
    print *, "error with a_list[0], ", a_int
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_map")
  if (PC_type(tree_tmp) /= PC_MAP) then
    print *, "error with a_map type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 10) then
    print *, "error with a_map line, ", line
  endif
  call PC_len(tree_tmp, a_int)
  if ( a_int /= 2 ) then
    print *, "error with a_map len, ", a_int
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_map{0}")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_map{0} type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 10) then
    print *, "error with a_map{0} line, ", line
  endif
  call PC_string(tree_tmp, a_string)
  if ( a_string /= "first" ) then
    print *, "error with a_map{0}, ", a_string
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_map<0>")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_map<0> type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 10) then
    print *, "error with a_map<0> line, ", line
  endif
  call PC_int(tree_tmp, a_int)
  if ( a_int /= 20 ) then
    print *, "error with a_map<0>, ", a_int
    error stop
  endif

  tree_tmp = PC_get(tree1,".another_list[1]")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with another_list[1] type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 14) then
    print *, "error with another_list[1] line, ", line
  endif
  call PC_int(tree_tmp, a_int)
  if ( a_int /= 31 ) then
    print *, "error with another_list[1], ", a_int
    error stop
  endif

  tree_tmp = PC_get(tree1,".another_map.second")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with another_map.second type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 18) then
    print *, "error with another_map.second line, ", line
  endif
  call PC_int(tree_tmp, a_int)
  if ( a_int /= 41 ) then
    print *, "error with another_map.second, ", a_int
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_true")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_true type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 21) then
    print *, "error with a_true line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( .not. a_log ) then
    print *, "error with a_true, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_True")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_True type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 22) then
    print *, "error with a_True line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( .not. a_log ) then
    print *, "error with a_True, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_TRUE")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_TRUE type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 23) then
    print *, "error with a_TRUE line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( .not. a_log ) then
    print *, "error with a_TRUE, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_yes")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_yes type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 24) then
    print *, "error with a_yes line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( .not. a_log ) then
    print *, "error with a_yes, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_Yes")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_Yes type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 25) then
    print *, "error with a_Yes line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( .not. a_log ) then
    print *, "error with a_Yes, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_YES")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_YES type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 26) then
    print *, "error with a_YES line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( .not. a_log ) then
    print *, "error with a_YES, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_false")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_false type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 27) then
    print *, "error with a_false line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( a_log ) then
    print *, "error with a_false, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_False")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_False type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 28) then
    print *, "error with a_False line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( a_log ) then
    print *, "error with a_False, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_FALSE")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_FALSE type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 29) then
    print *, "error with a_FALSE line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( a_log ) then
    print *, "error with a_FALSE, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_no")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_no type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 30) then
    print *, "error with a_no line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( a_log ) then
    print *, "error with a_no, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_No")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_No type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 31) then
    print *, "error with a_No line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( a_log ) then
    print *, "error with a_No, ", a_log
    error stop
  endif

  tree_tmp = PC_get(tree1,".a_NO")
  if (PC_type(tree_tmp) /= PC_SCALAR) then
    print *, "error with a_NO type, ", PC_type(tree_tmp)
    error stop
  endif
  call PC_line(tree_tmp, line)
  if (line /= 32) then
    print *, "error with a_NO line, ", line
  endif
  call PC_log(tree_tmp, a_log)
  if ( a_log ) then
    print *, "error with a_NO, ", a_log
    error stop
  endif

  ! Test status values
  
  ! First we need to pass the NULL_HANDLER
  ! The default handler is the PC_ASSERT_HANDLER which aborts on error
  call PC_errhandler(PC_NULL_HANDLER)

  call PC_string(PC_get(tree1,".a_string"), a_string, ierr)
  if (ierr /= PC_OK) then
    print *, "error with ierr==PC_OK, got ", ierr
    error stop
  endif

  call PC_int(PC_get(tree1,".a_string"), a_int, ierr)
  if (ierr /= PC_INVALID_NODE_TYPE) then
    print *, "error with ierr==PC_INVALID_NODE_TYPE, got ", ierr
    error stop
  endif
  
  ierr = PC_status(PC_get(tree1,".invalid_node"))
  if (ierr /= PC_NODE_NOT_FOUND) then
    print *, "error with ierr==PC_NODE_NOT_FOUND, got ", ierr
    error stop
  endif
  
  call PC_string(PC_get(tree1,".invalid_node"), a_string, ierr)
  if (ierr /= PC_NODE_NOT_FOUND) then
    print *, "error with ierr==PC_NODE_NOT_FOUND, got ", ierr
    error stop
  endif
  call PC_errmsg(errmsg)
  if (trim(errmsg) /= ("Cannot interpret empty tree as string")) then
    print *, "error with error message, got `", trim(errmsg),"'"
    error stop
  endif
  
  call PC_tree_destroy(tree1)

end program example
