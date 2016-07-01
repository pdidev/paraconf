PROGRAM example
  USE paraconf

  TYPE(PC_tree_t) :: tree1,tree2
  INTEGER :: a_int
  CHARACTER(20) :: a_string
  REAL(8) :: a_float

  call PC_parse_path("example.yml",tree1)

  call PC_get(tree1,".a_int", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 100 ) then
    print *, "error with a_int, ", a_int
    stop
  endif
  call PC_get(tree1,".a_float", tree2)
  call PC_double(tree2, a_float)
  if ( a_float /= 100.1 ) then
    print *, "error with a_float, ", a_float
    stop
  endif
  call PC_get(tree1,".a_string", tree2)
  call PC_string(tree2, a_string)
  if ( a_string /= "this is a string" ) then
    print *, "error with a_string, ", a_string
    stop
  endif
  call PC_get(tree1, ".a_list", tree2)
  call PC_len(tree2, a_int)
  if ( a_int /= 2 ) then
    print *, "error with a_list len, ", a_int
    stop
  endif
  call PC_get(tree1, ".a_list[0]", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 10 ) then
    print *, "error with a_list[0], ", a_int
    stop
  endif
  call PC_get(tree1, ".a_map", tree2)
  call PC_len(tree2, a_int)
  if ( a_int /= 2 ) then
    print *, "error with a_map len, ", a_int
    stop
  endif
  call PC_get(tree1,".a_map{0}", tree2)
  call PC_string(tree2, a_string)
  if ( a_string /= "first" ) then
    print *, "error with a_map{0}, ", a_string
    stop
  endif
  call PC_get(tree1, ".a_map<0>", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 20 ) then
    print *, "error with a_map<0>, ", a_int
    stop
  endif
  call PC_get(tree1, ".another_list[1]", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 31 ) then
    print *, "error with another_list[1], ", a_int
    stop
  endif
  call PC_get(tree1, ".another_map.second", tree2)
  call PC_int(tree2, a_int)
  if ( a_int /= 41 ) then
    print *, "error with another_map.second, ", a_int
    stop
  endif

  call PC_tree_destroy(tree1)

END PROGRAM example