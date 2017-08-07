#ifndef _YAML2STRUCT_C_TYPES_DEFINITION_
#define _YAML2STRUCT_C_TYPES_DEFINITION_

typedef struct MAP_ITEM_0_s MAP_ITEM_0_t;
typedef struct MAP_ITEM_1_s MAP_ITEM_1_t;
typedef struct MAP_ITEM_2_s MAP_ITEM_2_t;
typedef struct MAP_ITEM_3_s MAP_ITEM_3_t;
typedef struct MAP_ITEM_4_s MAP_ITEM_4_t;
typedef struct MAP_ITEM_5_s MAP_ITEM_5_t;
typedef struct MAP_ITEM_6_s MAP_ITEM_6_t;
typedef struct toto_s toto_t;


// Depth 3 types --- --- ---

struct toto_s {
    int k1;
    double* k2;
};


// Depth 2 types --- --- ---

struct MAP_ITEM_0_s {
    char* key;
    double* value0;
};

struct MAP_ITEM_1_s {
    char* key;
    double* value0;
};

struct MAP_ITEM_2_s {
    char* key;
    double** value0;
};

struct MAP_ITEM_3_s {
    char* key;
    double***** value0;
};

struct MAP_ITEM_4_s {
    char* key;
    toto_t** value0;
};

struct MAP_ITEM_5_s {
    char* key;
    char** value1;
    toto_t** value2;
    double**** value0;
};

struct MAP_ITEM_6_s {
    char* key;
    int* value0;
};


// Depth 1 types --- --- ---

typedef double a_t;

typedef double* b_t;

typedef MAP_ITEM_0_t* c_t;

typedef MAP_ITEM_1_t** key1_t;

typedef MAP_ITEM_2_t* key2_t;

typedef MAP_ITEM_3_t* key3_t;

typedef MAP_ITEM_4_t* key4_t;

typedef MAP_ITEM_5_t* key5_t;

typedef MAP_ITEM_6_t** key6_t;


#endif
