# PCgen --- A code generator for ParaConf

PCgen allows to generate C initialization code from a Yamale schema.
Check https://github.com/23andMe/Yamale for more details on Yamale.

## Getting started

To generate the source code, the following command can be entered:
```Bash
./pcgen PATH_TO_SCHEMA
```
Five files will be created: types.h, pcgen\_free.h, pcgen\_free.c, pcgen\_init.h, pcgen\_init.c. The first header defines the types and structures that will store the data. The pcgen\_init files declare and define the functions that will load the data. To use these resources, the three following headers should be included in the C source:
```C
#include <paraconf.h>
#include "pcgen_free.h"
#include "pcgen_init.h"
```

## How to use it

The function `load_root(PC_tree_t, root_t*)` loads all the data contained in a PC\_tree\_t object (validated by the Yamale schema) to the adress pointed by a root\_t pointer, and returns a PC\_status\_t value. Before exiting the program, the root\_t pointer should be deallocated by `free_root(root_t*)` (free\_root only deallocate the root's fields, not the root variable itself). The way of accessing the data from a root\_t variable/pointer is described bellow.

The root\_t type is a structure based on the Yamale schema scpecified to PCgen. Considering a root variable of type root\_t\*, the data of a main node can be accessed with `root->NODE_NAME`. Here are the types used to store the data, and the associated Yamale validators:
```
Yamale validators <-> C types
bool()            ... int
int()             ... long
num()             ... double
str()             ... char*
```

More complex types can be used:

### any() and enum() validators
```C
// any(type1_validator(), type2_validator(), ...) or enum(element_of_type1, element_of_type2, ...)
struct {
    enum {path_to_var_<type1>, path_to_var_<type2>, ...} type;
    union {
		type1 <type1>_value;
        type2 <type2>_value;
		// ...
    } item;
} c_variable;
```

### list() validator
```C
// list(type1_validator(), type2_validator(), ...)
struct {
    int len;
    struct {
        enum {path_to_var_<type1>, path_to_var_<type2>, ...} type;
        union {
			type1 <type1>_value;
            type2 <type2>_value;
			// ...
        } item;
    }* tab;
} c_variable;
```

### map() validator
```C
// map(type1_validator(), type2_validator(), ...)
struct {
    int len;
    struct {
        char* key;
        enum {path_to_var_<type1>, path_to_var_<type2>, ...} type;
        union {
			type1 <type1>_value;
            type2 <type2>_value;
			// ...
        } item;
    }* map;
} c_variable;
```

### Unspecified nodes
If a node of the YAML configuration file is not defined in the schema, it will be loaded as a PC\_tree\_t. Lists and maps of unspecified nodes are also possible, and lead to the following structure:
```C
// list() or map()
struct {
    int len;
    struct {
        char* key; // if c_variable refers to a map
        union {
			PC_tree_t node;
        } item;
    }* tab; // or map
} c_variable;
```

## Noticeable behaviors
When a node is defined in the schema as being optional (required=False), its associated type will be a pointer. If the type already is a pointer (for ex. str() <-> char*), adding required=False changes nothing. It is also useless to declare a node as optional inside an any/list/map object.

In an any object, only strings are pointers. In list/map objects, both strings and included types are pointers to allow recursive type definition.

Finally, adding a nested any inside an any/list/map object does not change anything. For example, `map(any(int(), num()))` will be equivalent to `map(int(), num())`. Similar behavior can be seen with nested enum: `list(enum(3.14, 'pi'))` will be equivalent to `list(num(), str())` in PCgen.

## Enum and union values names
any(), list() and map() validators are defined by using an union field. An additional `enum {} type;` field allows to know which type is currently being used. Enum types names are made of:
  - the path to the considered validator, each element separated by an underscore. The path must begin by the node's base, e.g. 'root' or an included node's name ('root\_node\_subnode...' or '<included\_node>\_node\_subnode...');
  - a string associated to the validator (int for bool(), long for int(), double for num(), str for str(), list<i> for list(), map<j> for map()). <i> (resp. <j>) is replaced by the apparition order from zero of the given list (resp. map) inside the initial validator. PCgen does not check if two lists or maps are equal;

Union fields are made of the same string associated to the validator, plus '\_value'.

For the sake of example:
```YAML
Node1:
  subnode: any(int(), num())
  # Enum names are: root_Node1_subnode_long, root_Node1_subnode_double
  # Union fields are: long_value, double_value

Node2:
  subnode: map(list(bool(), str()))
  # Enum name of the map: root_Node2_subnode_list0
  # Union field of the map is: list0_value
  # Enum names of the list: root_Node2_subnode_list0_int, root_Node2_subnode_list0_int
  # Union fields of the list are: int_value, str_value
  
Node3: list(int(min=10), int(max=-10))
  # Enum name of the list: root_Node3_long
  # Union field of the list: long_value

Node4: any(int(), list(int()), map(int()), list(str()))
  # Enum names of the any: root_Node4_long, root_Node4_list0, root_Node4_map0, root_Node4_list1
  # Union fields of the any: long_value, list0_value, map0_value, list1_value

Node5:
  subnode: include('usertype')
  # Enum names: usertype_a_long
  # Union field: long_value

---
usertype:
  a: list(int())
  # Enum names: usertype_a_long
  # Union field: long_value
```
