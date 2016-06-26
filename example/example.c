#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <yaml.h>
#include <paraconf.h>

int main(int argc, char *argv[])
{

	PC_tree_t conf = PC_parse_path("example.yml");
	
	int nb_iter; PC_int(PC_get(conf, ".iter"), &nb_iter);

	printf("nb_iter = %d\n",nb_iter);

	char *nb_iter_string; PC_string(PC_get(conf, ".iter"), &nb_iter_string);

	printf("nb_iter = %s\n",nb_iter_string);

	free(nb_iter_string);
	PC_tree_destroy(&conf);

	return 0;
}
