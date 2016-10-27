PROGRAM example
  USE paraconf

  TYPE(PC_tree_t) :: tree1,tree2
  INTEGER :: a_int
  CHARACTER(20) :: a_string
  REAL(8) :: a_float
  INTEGER :: ierr
  TYPE(PC_errhandler_t) :: old_handler, new_handler
  CHARACTER(LEN=PC_ERRMSG_MAXLENGTH) :: errmsg

  call PC_parse_path("example.yml",tree1)

  call PC_get(tree1,".a_int", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 100 ) then
    print *, "error with a_int, ", a_int
    stop 1
  endif
  call PC_get(tree1,".a_float", tree2)
  call PC_double(tree2, a_float)
  if ( abs(a_float-100.1d0) > 1.0e-10 ) then
    print *, "error with a_float, ", a_float
    stop 1
  endif
  call PC_get(tree1,".a_string", tree2)
  call PC_string(tree2, a_string)
  if ( a_string /= "this is a string" ) then
    print *, "error with a_string, ", a_string
    stop 1
  endif
  call PC_get(tree1, ".a_list", tree2)
  call PC_len(tree2, a_int)
  if ( a_int /= 2 ) then
    print *, "error with a_list len, ", a_int
    stop 1
  endif
  call PC_get(tree1, ".a_list[0]", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 10 ) then
    print *, "error with a_list[0], ", a_int
    stop 1
  endif
  call PC_get(tree1, ".a_map", tree2)
  call PC_len(tree2, a_int)
  if ( a_int /= 2 ) then
    print *, "error with a_map len, ", a_int
    stop 1
  endif
  call PC_get(tree1,".a_map{0}", tree2)
  call PC_string(tree2, a_string)
  if ( a_string /= "first" ) then
    print *, "error with a_map{0}, ", a_string
    stop 1
  endif
  call PC_get(tree1, ".a_map<0>", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 20 ) then
    print *, "error with a_map<0>, ", a_int
    stop 1
  endif
  call PC_get(tree1, ".another_list[1]", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 31 ) then
    print *, "error with another_list[1], ", a_int
    stop 1
  endif
  call PC_get(tree1, ".another_map.second", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 41 ) then
    print *, "error with another_map.second, ", a_int
    stop 1
  endif

  ! Test status values
  
  ! First we need to pass the NULL_HANDLER
  ! The default handler is the PC_ASSERT_HANDLER which aborts on error
  new_handler = PC_NULL_HANDLER
  call PC_errhandler(new_handler, old_handler)

  print *, "reading a string as a string should be ok"
  call PC_get(tree1,".a_string", tree2)
  call PC_string(tree2, a_string, ierr)
  if (ierr /= PC_OK) then
    print *, "error with ierr==PC_OK, got ", ierr
    stop 1
  endif

  print *, "reading a string as an int should not be ok"
  call PC_get(tree1,".a_string", tree2)
  call PC_int(tree2, a_int, ierr)
  if (ierr /= PC_INVALID_NODE_TYPE) then
    print *, "error with ierr==PC_INVALID_NODE_TYPE, got ", ierr
    stop 1
  endif
  
  print *, "trying to access an invalid node should not be ok"
  call PC_get(tree1,".invalid_node", tree2)
  call PC_status(tree2, ierr)
  if (ierr /= PC_NODE_NOT_FOUND) then
    print *, "error with ierr==PC_NODE_NOT_FOUND, got ", ierr
    stop 1
  endif

  print *, "trying to access an invalid node should print an error message"
  call PC_get(tree1,".invalid_node", tree2)
  call PC_status(tree2, ierr)
  call PC_errmsg(errmsg)
  ! watch out for the new line character (achar(10))
  if (trim(errmsg) /= ("Key `invalid_node' not found in (ROOT)"//achar(10))) then
     print *, "error with error message, got `", trim(errmsg),"'"
     stop 1
  endif
  
  call PC_tree_destroy(tree1)

END PROGRAM example
