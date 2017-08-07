#ifndef _YAML2STRUCT_C_TYPES_DEFINITION_
#define _YAML2STRUCT_C_TYPES_DEFINITION_

typedef struct MAP_ITEM_0_s MAP_ITEM_0_t;
typedef struct key1_s key1_t;
typedef struct taratata_s taratata_t;
typedef struct toto_s toto_t;


// Depth 3 types --- --- ---

struct toto_s {
    int bar;
    int foo;
};


// Depth 2 types --- --- ---

struct MAP_ITEM_0_s {
    char* key;
    toto_t* value0;
};

struct taratata_s {
    char* bar;
    int foo;
};


// Depth 1 types --- --- ---

struct key1_s {
    double** k1;
    taratata_t k2;
    MAP_ITEM_0_t**** k3;
    struct key1_k4_s {
        toto_t k5;
        struct key1_k4_k6_s {
            struct key1_k4_k6_k7_s {
                taratata_t* k8;
            } k7;
        } k6;
    } k4;
};


#endif
