! Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
!               root of the project or at https://github.com/pdidev/paraconf
! 
! SPDX-License-Identifier: MIT

!> Status of function execution
enum, bind(C)
  enumerator :: PC_OK = 0            ! No error
  enumerator :: PC_INVALID_PARAMETER ! A parameter value is invalid
  enumerator :: PC_INVALID_NODE_TYPE ! Unexpected type found for a node
  enumerator :: PC_NODE_NOT_FOUND    ! The requested node doesn exist in the tree
  enumerator :: PC_INVALID_FORMAT    ! The provided input is invalid
  enumerator :: PC_SYSTEM_ERROR      ! An error occured with the system
end enum


integer, parameter :: PC_ERRMSG_MAXLENGTH = 1024


!> Error handlers
!! They are constants and not the variables of the C library, which Fortran would have to define as common blocks aligned differently.
!! PC_ASSERT_HANDLER is a placeholder (no function, context 1) that PC_errhandler replaces by the handler of the C library.
!! \{
type(PC_errhandler_t), parameter :: PC_ASSERT_HANDLER = PC_errhandler_t(c_null_funptr, transfer(1_c_intptr_t, c_null_ptr))
type(PC_errhandler_t), parameter :: PC_NULL_HANDLER = PC_errhandler_t(c_null_funptr, c_null_ptr)
!! \}
