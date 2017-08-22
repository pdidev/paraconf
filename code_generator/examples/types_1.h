#ifndef _PARACONF_C_TYPES_DEFINITION_
#define _PARACONF_C_TYPES_DEFINITION_

#include <paraconf.h>


typedef struct root_s root_t;


struct root_s {
    long key1;
    double key2;
    struct {
        size_t len;
        char* str;
    } key3;
    int key4;
    struct {
        long key1;
        double key2;
        struct {
            long key6;
            long key7;
            struct {
                double k1;
                long k2;
                double k3;
                struct {
                    size_t len;
                    PC_tree_t* node;
                }* generic;
            } key8;
            struct {
                size_t len;
                PC_tree_t* node;
            }* generic;
        } key3;
        struct {
            size_t len;
            PC_tree_t* node;
        }* generic;
    } key5;
    struct {
        size_t len;
        PC_tree_t* node;
    }* generic;
};


#endif
