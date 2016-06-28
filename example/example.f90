PROGRAM example
	USE paraconf

	TYPE(PC_tree_t) :: tree1,tree2
	INTEGER :: nb_iter, lengh
	CHARACTER(20) :: nb_iter_string
	REAL(8) :: nb_iter_double

	call PC_parse_path("example.yml",tree1)

	print *, "tree.status = ", tree1%status

	call PC_get(tree1,".iter",tree2)

	call PC_int(tree2,nb_iter)

	print *, "nb_iter =", nb_iter

	call PC_len(tree2,lengh)

	print *, "lengh(nb_iter) =", lengh

	call PC_double(tree2,nb_iter_double)

	print *, "nb_iter_double =", nb_iter_double

	call PC_string(tree2,nb_iter_string)

	print *, "nb_iter_string =", trim(nb_iter_string)

	call PC_tree_destroy(tree1)

END PROGRAM example