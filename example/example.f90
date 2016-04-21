PROGRAM example
	USE paraconf
	USE PC_tree_t

	TYPE(PC_tree_t_f) :: tree1,tree2
	INTEGER, POINTER :: nb_iter

	print *, "test1 : PC_parse_path"

	call PC_parse_path("example.yml",tree1)

	print *, "tree.status = ", tree1%status

	print *, "test2 : PC_get"

	call PC_get(tree1,".iter",tree2)

	print *, "test3 : PC_int"

	call PC_int(tree2,nb_iter)

	print *, "nb_iter =", nb_iter

END PROGRAM example