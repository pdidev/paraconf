#include <stdio.h>
#include "types_1.h"


int main(int argc, char* argv[]) {

    root_t config;
    PC_tree_t conf = PC_parse_path("data_1.yaml");
    
    PC_int(PC_get(conf, ".key1"), &config.key1);
    PC_double(PC_get(conf, ".key2"), &config.key2);
    PC_string(PC_get(conf, ".key3"), &config.key3.str);
    PC_bool(PC_get(conf, ".key4"), &config.key4);
    PC_int(PC_get(conf, ".key5.key1"), &config.key5.key1);
    PC_double(PC_get(conf, ".key5.key2"), &config.key5.key2);

    printf("%ld\n", config.key1);
    printf("%lf\n", config.key2);
    printf("%s\n", config.key3.str);
    printf("%d\n", config.key4);
    printf("%ld\n", config.key5.key1);
    printf("%g\n", config.key5.key2);

    free(config.key3.str);

    PC_tree_destroy(&conf);

    return 0;
}
