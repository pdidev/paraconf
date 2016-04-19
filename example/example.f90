PROGRAM example
	USE paraconf
	USE PC_tree_t

	TYPE(PC_tree_t_f) :: tree1,tree2
	INTEGER, POINTER :: nb_iter

	call PC_parse_path("example.yml",tree1)

	call PC_get(tree1,".iter",tree2)

	call PC_int(tree2,nb_iter)

	print *, "nb_iter =", nb_iter

END PROGRAM example