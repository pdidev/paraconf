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
	
	int a_int; PC_int(PC_get(conf, ".a_int"), &a_int);
	assert(a_int==100);
	double a_float; PC_double(PC_get(conf, ".a_float"), &a_float);
	assert(a_float==100.1);
	char* a_string; PC_string(PC_get(conf, ".a_string"), &a_string);
	assert(!strcmp("this is a string", a_string));
	free(a_string);
	int a_list_len; PC_len(PC_get(conf, ".a_list"), &a_list_len);
	assert(a_list_len==2);
	int a_list_0; PC_int(PC_get(conf, ".a_list[0]"), &a_list_0);
	assert(a_list_0==10);
	int a_map_len; PC_len(PC_get(conf, ".a_map"), &a_map_len);
	assert(a_map_len==2);
	char *a_map_0_k; PC_string(PC_get(conf, ".a_map{0}"), &a_map_0_k);
	assert(!strcmp("first", a_map_0_k));
	free(a_map_0_k);
	int a_map_0_v; PC_int(PC_get(conf, ".a_map<0>"), &a_map_0_v);
	assert(a_map_0_v == 20);
	int another_list_1; PC_int(PC_get(conf, ".another_list[1]"), &another_list_1);
	assert(another_list_1==31);
	int another_map_second; PC_int(PC_get(conf, ".another_map.second"), &another_map_second);
	assert(another_map_second==41);
	
	return 0;
}
