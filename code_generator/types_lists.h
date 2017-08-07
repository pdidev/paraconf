#ifndef _YAML2STRUCT_C_TYPES_DEFINITION_
#define _YAML2STRUCT_C_TYPES_DEFINITION_

typedef struct MAP_ITEM_0_s MAP_ITEM_0_t;
typedef struct UNION_LIST_0_s UNION_LIST_0_t;
typedef struct UNION_LIST_1_s UNION_LIST_1_t;
typedef struct key6_s key6_t;
typedef struct taratata_s taratata_t;
typedef struct titi_s titi_t;
typedef struct toto_s toto_t;


// Depth 5 types --- --- ---

struct taratata_s {
    int h20;
};


// Depth 4 types --- --- ---

struct toto_s {
    char***** bar;
    char**** foo;
    taratata_t o2;
};


// Depth 3 types --- --- ---

struct titi_s {
    char* example1;
    toto_t* exemple2;
};


// Depth 2 types --- --- ---

struct MAP_ITEM_0_s {
    char* key;
    int* value0;
};

struct UNION_LIST_0_s {
    int* value1;
    titi_t*** value0;
};

struct UNION_LIST_1_s {
    int* value0;
    int** value1;
};


// Depth 1 types --- --- ---

typedef double key1_t;

typedef double* key2_t;

typedef double** key3_t;

typedef double** key4_t;

typedef double*** key5_t;

struct key6_s {
    toto_t*** k1;
    struct key6_k2_s {
        taratata_t* _k;
        double*** k3;
        MAP_ITEM_0_t** k4;
        struct key6_k2_k5_s {
            UNION_LIST_0_t* k6;
        } k5;
    } k2;
};

typedef UNION_LIST_1_t* key7_t;


#endif
