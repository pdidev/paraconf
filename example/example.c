#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <paraconf.h>

int main(int argc, char *argv[])
{
	PC_tree_t conf = PC_parse_path("example.yml");
	
	long a_int; PC_int(PC_get(conf, ".a_int"), &a_int);
	double a_float; PC_double(PC_get(conf, ".a_float"), &a_float);
	char* a_string; PC_string(PC_get(conf, ".a_string"), &a_string);
	int a_yes; PC_bool(PC_get(conf, ".a_yes"), &a_yes);
	printf("a_int=%ld a_float=%f a_string=%s a_yes=%s\n", a_int, a_float, a_string, a_yes?"true":"false");
	free(a_string);
	
	printf("a_list=[ ");
	int a_list_len; PC_len(PC_get(conf, ".a_list"), &a_list_len);
	for ( int ii=0; ii<a_list_len; ++ii ) {
		long a_list_ii; PC_int(PC_get(conf, ".a_list[%d]", ii), &a_list_ii);
		printf("%ld ", a_list_ii);
	}
	printf("]\n");
	
	printf("a_map={   ");
	int a_map_len; PC_len(PC_get(conf, ".a_map"), &a_map_len);
	for ( int ii=0; ii<a_list_len; ++ii ) {
		char *a_map_ii_k; PC_string(PC_get(conf, ".a_map{%d}", ii), &a_map_ii_k);
		long a_map_ii_v; PC_int(PC_get(conf, ".a_map<%d>", ii), &a_map_ii_v);
		printf("%s => %ld   ", a_map_ii_k, a_map_ii_v);
		free(a_map_ii_k);
	}
	printf("}\n");
	
	PC_errhandler_t errh = PC_errhandler(PC_NULL_HANDLER);
	PC_tree_t some_key = PC_get(conf, ".some_key");
	PC_errhandler(errh);
	
	printf("config %s `some_key'\n", PC_status(some_key)?"does not contain":"contains");
	
	PC_tree_destroy(&conf);
	
	return 0;
}
