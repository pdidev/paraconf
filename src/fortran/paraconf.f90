! Copyright (C) The Paraconf development team, see COPYRIGHT.md file at the
!               root of the project or at https://github.com/pdidev/paraconf
! 
! SPDX-License-Identifier: MIT

module paraconf

  use ISO_C_binding

  implicit none

  include 'paraconf_f90.h'

end module paraconf


integer function PC_status(tree)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_c.h'

  type(PC_tree_t), intent(IN) :: tree

  PC_status = int(tree%status)

end function PC_status


subroutine PC_errmsg(errmsg)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_c.h'

  character(*), intent(OUT) :: errmsg
  character(kind = C_char), pointer, dimension(:) :: errmsg_array
  integer :: errmsg_length
  integer :: I

  errmsg_length = len(errmsg)
  errmsg = ""
  call C_F_pointer(PC_errmsg_C(), errmsg_array, [errmsg_length])
  if (associated(errmsg_array)) then

    ! truncated to the variable, as a Fortran assignment would
    do I = 1, errmsg_length
      if (errmsg_array(I) == C_NULL_CHAR) exit
      errmsg(I:I) = errmsg_array(I)
    end do
  end if

end subroutine PC_errmsg


subroutine PC_errhandler(new_handler, old_handler)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_c.h'

  type(PC_errhandler_t), intent(IN) :: new_handler
  type(PC_errhandler_t), intent(OUT), optional :: old_handler

  type(PC_errhandler_t) :: tmp_handler

  if (present(old_handler)) then
    old_handler = PC_errhandler_C(new_handler)
  else
    tmp_handler = PC_errhandler_C(new_handler)
  end if

end subroutine PC_errhandler


subroutine PC_parse_path(path, tree)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_c.h'

  character(len = *), intent(IN) :: path
  type(PC_tree_t), intent(OUT) :: tree

  integer :: i
  character(C_char), target :: C_path(len_trim(path)+1)

  do i = 1, len_trim(path)
      C_path(i) = path(i:i)
  end do
  C_path(len_trim(path)+1) = C_NULL_CHAR

  tree = PC_parse_path_C(c_loc(C_path))

end subroutine PC_parse_path


subroutine PC_len(tree_in, value, status)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_c.h'

  type(PC_tree_t), intent(IN) :: tree_in
  integer, intent(INOUT), target :: value
  integer, intent(OUT), optional :: status

  integer :: tmp


  if(present(status)) then
    status = int(PC_len_C(tree_in, c_loc(value)))
  else
    tmp = int(PC_len_C(tree_in, c_loc(value)))
  end if

end subroutine PC_len


type(PC_tree_t) function PC_get(tree, index_fmt)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_c.h'

  type(PC_tree_t), intent(IN) :: tree
  character(len = *), intent(IN) :: index_fmt
!   type(*), optional, intent(IN) :: arguments(:)

  integer :: i
  character(C_char), target :: C_index_fmt(len_trim(index_fmt)+1)

  do i = 1, len_trim(index_fmt)
      C_index_fmt(i) = index_fmt(i:i)
  end do
  C_index_fmt(len_trim(index_fmt)+1) = C_NULL_CHAR

  PC_get = PC_get_C(tree, c_loc(C_index_fmt))

end function PC_get


subroutine PC_int(tree_in, value, status)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_consts.h'
  include 'paraconf_f90_c.h'

  type(PC_tree_t), intent(IN) :: tree_in
  integer, intent(INOUT) :: value
  integer, intent(OUT), optional :: status

  integer :: tmp
  integer(C_long), target :: longvalue

  ! a long, that the C library reads, can hold values a default integer cannot
  tmp = int(PC_int_range_C(tree_in, c_loc(longvalue), int(-huge(value)-1, C_long), int(huge(value), C_long)))
  if (present(status)) status = tmp

  ! left as it was on failure
  if (tmp == PC_OK) value = int(longvalue)

end subroutine PC_int


subroutine PC_double(tree_in, value, status)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_consts.h'
  include 'paraconf_f90_c.h'

  type(PC_tree_t), intent(IN) :: tree_in
  real(8), intent(INOUT) :: value
  integer, intent(OUT), optional :: status

  integer :: tmp
  real(C_double), target :: doublevalue

  tmp = int(PC_double_C(tree_in, c_loc(doublevalue)))
  if (present(status)) status = tmp

  ! left as it was on failure
  if (tmp == PC_OK) value = real(doublevalue, 8)

end subroutine PC_double


subroutine PC_string(tree_in, value, status)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_consts.h'
  include 'paraconf_f90_c.h'

  type(PC_tree_t), intent(IN) :: tree_in
  character(len = *), intent(INOUT) :: value
  integer, intent(OUT), optional :: status

  integer :: i, tmp
  integer, dimension(1), target :: tab_lengh
  type(C_ptr), target :: C_pointer
  CHARACTER, dimension(:), pointer :: F_pointer

  ! left as it was on failure
  tmp = int(PC_string_C(tree_in, c_loc(C_pointer)))
  if (present(status)) status = tmp

  if (tmp ==  PC_OK) then

    tmp = int(PC_len_C(tree_in, C_loc(tab_lengh(1))))

    call C_F_pointer(C_pointer, F_pointer, tab_lengh)
    ! truncated to the variable, as a Fortran assignment would
    do i = 1, min(tab_lengh(1), len(value))
      value(i:i) = F_pointer(i)
    end do

    do i = tab_lengh(1)+1, len(value)
      value(i:i) = ' '
    end do

    call free_C(C_pointer)
  end if
end subroutine PC_string


subroutine PC_log(tree_in, value, status)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_consts.h'
  include 'paraconf_f90_c.h'

  type(PC_tree_t), intent(IN) :: tree_in
  logical, intent(INOUT) :: value
  integer, intent(OUT), optional :: status

  integer :: tmp
  integer, target :: ilog

  tmp = int(PC_bool_C(tree_in, c_loc(ilog)))
  if (present(status)) status = tmp

  ! left as it was on failure
  if (tmp == PC_OK) value = (ilog /= 0)

end subroutine PC_log


subroutine PC_tree_destroy(tree_in, status)

  use ISO_C_binding

  implicit none

  include 'paraconf_f90_types.h'
  include 'paraconf_f90_c.h'

  type(PC_tree_t), intent(INOUT), target :: tree_in
  integer, intent(OUT), optional :: status

  integer :: tmp

  if(present(status)) then
    status = int(PC_tree_destroy_C(tree_in))
  else
    tmp = int(PC_tree_destroy_C(tree_in))
  end if

end subroutine PC_tree_destroy
