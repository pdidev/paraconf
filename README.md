# Paraconf distribution

[Paraconf](paraconf/) is a library that provides a simple query language to
access a Yaml tree on top of [yaml-cpp](https://github.com/jbeder/yaml-cpp/wiki).

The paraconf distribution provides a single packages with paraconf, its
dependancies and examples.

## Getting the source

You can find a list of release at 
https://github.com/pdidev/paraconf/releases

For example, you can get release 0.5.0:
```
wget https://github.com/pdidev/paraconf/archive/0.5.0.tar.gz
tar -xzf 0.5.0.tar.gz
mv paraconf-0.5.0 paraconf
```


## Compilation

If the sources are stored in the folder `paraconf`:
```
cd paraconf
cmake -DCMAKE_INSTALL_PREFIX=~/.local/ .
make install
```

Change `-DCMAKE_INSTALL_PREFIX=~/.local/` to something else if you want to install
somewhere else than in `~/.local/`.

## Prerequisites

The paraconf distribution depends on:
  * [cmake](https://cmake.org), version >= 3.5,
  * C 99, C++ 11 and Fortran 95 compiler ([gcc](https://gcc.gnu.org/) 5.4 is tested),
  * a POSIX compatible OS (linux with GNU libc-2.27 is tested)
  * [yaml-cpp](https://github.com/jbeder/yaml-cpp/wiki) (distributed with Paraconf
    distribution)

Paraconf Fortran support depends on:
  * a Fortran-2003 compiler (gfort-5.4 is tested)

## Usage

Look at the [paraconf/](paraconf/) directory to some basic usage documentation.
Look at the [example/](example/) directory to get an example of usage.

## Quick guide

### Open yaml

C:
```c
#include <paraconf.h>
// parse yaml document from file
PC_tree_t root_from_file = PC_parse_path("example.yml"); 

// parse yaml document from string (only in C/C++)
PC_tree_t root_from_string = PC_parse_string("{first: 1, second: 2}");

...

// destroy root tree at the end
PC_tree_destroy(&root_from_file); 
PC_tree_destroy(&root_from_string);
```

Fortran:
```fortran
type(pc_tree_t) :: root

! parse yaml document from file
call PC_parse_path("example.yml", root)

...

! destroy root tree at the end
call PC_tree_destroy(root)
```

C++:
```cpp
#include <paraconf/PC_node.h>

// parse yaml document from file
PC_node root_from_file = PC_load_file("example.yml"); 

// parse yaml document from string (only in C/C++)
PC_node root_from_string = PC_load("{first: 1, second: two}");

// no need to destroy: destroyed when out of scope
```

### Get yaml subtree

C:
```c
#include <paraconf.h>

PC_tree_t root = PC_parse_string("{first: 1, second: two}");

// get first node from root
PC_tree_t first_node = PC_get(root, ".first");

// write value of first node (`1') to first_value variable
int first_value; PC_int(first_node, &first_value); 

// get second node from root
PC_tree_t second_node = PC_get(root, ".second");

// write value of second node (`two') to second_value variable
char* second_value; PC_string(second_node, &second_value);

...

// free string allocated by paraconf
free(second_value);

// destroy root tree at the end
PC_tree_destroy(&root);
```

Fortran:
```fortran
type(pc_tree_t) :: root
type(pc_tree_t) :: first_node
type(pc_tree_t) :: second_node
integer :: first_value
character(8) :: second_value

call PC_parse_path("yaml_from_c_example.yml", root)

! get first node from root
first_node = PC_get(root, ".first")

! write value of first node (`1') to first_value variable
call PC_int(first_node, first_value)

! get second node from root
second_node = PC_get(root, ".second")

! write value of second node (`two') to second_value variable
call PC_string(second_node, second_value)

...

! destroy root tree at the end
PC_tree_destroy(root)
```

C++:
```cpp
#include <paraconf/PC_node.h>

PC_node root = PC_load("{first: 1, second: two}");

// write value of first node (`1') to first_value variable
int first_value = root["first"].as<int>();

// write value of second node (`two') to second_value variable
std::string second_value = root["second"].as<std::string>();

```

### Check tree status

C:
```c
#include <paraconf.h>

PC_tree_t root = PC_parse_string("{first: 1, second: two}");

// get first node from root
PC_tree_t wrong_node = PC_get(root, ".wrong_name");

int correct_value = -1;
// write value of wrong_name node (only if exists) to correct_value variable
if (!PC_status(wrong_node)) {
  PC_int(wrong_node, &correct_value);
} else {
  printf("`wrong_name' doesn't exsist in root node!\n");
}

// destroy root tree at the end
PC_tree_destroy(&root);
```

Fortran:
```fortran
type(pc_tree_t) :: root
type(pc_tree_t) :: wrong_node
integer :: correct_value

call PC_parse_path("yaml_from_c_example.yml", root)

! get first node from root
wrong_node = PC_get(root, ".wrong_name")

! write value of wrong_name node (only if exists) to correct_value variable
if (PC_status(wrong_node) /= PC_OK) {
  PC_int(wrong_node, correct_value)
} else {
  print *, "`wrong_name' doesn't exsist in root node!"
}

! destroy root tree at the end
PC_tree_destroy(root)
```

C++:
```cpp
#include <paraconf/PC_node.h>

PC_node root = PC_load("{first: 1, second: two}");

// get first node from root
try {
  int correct_value = root["wrong_name"];
} catch (const PC::Error& e) {
  std::cerr << e.what() << std::endl;
}
```

### Check tree type

C:
```c
#include <paraconf.h>

// parse yaml document from file
PC_tree_t root = PC_parse_path("example.yml");

switch (PC_type(root))
​{
    case PC_EMPTY:
      ...
      break;
    case PC_SCALAR:
      ...
      break;
    case PC_SEQUENCE:
      ...
      break;
    case PC_MAP:
      ...
      break;
    case PC_UNDEFINED:
      ...
      break;
}

// destroy root tree at the end
PC_tree_destroy(&root);
```

Fortran:
```fortran
type(pc_tree_t) :: root

call PC_parse_path("example.yml", root)

select case (PC_type(root))
  case (PC_EMPTY) 
    ...

  case (PC_SCALAR)
    ...
  
  case (PC_SEQUENCE) 
    ... 

  case (PC_MAP)
    ... 

  case (PC_UNDEFINED)
    ... 
end select

! destroy root tree at the end
call PC_tree_destroy(root)
```

C++:
```cpp
#include <paraconf.h>
#include <paraconf/PC_node.h>

PC_node root = PC_load_file("example.yml");

switch (root.Type()))
​{
    case PC_tree_type_t::PC_EMPTY:
      ...
      break;
    case PC_tree_type_t::PC_SCALAR:
      ...
      break;
    case PC_tree_type_t::PC_SEQUENCE:
      ...
      break;
    case PC_tree_type_t::PC_MAP:
      ...
      break;
    case PC_tree_type_t::PC_UNDEFINED:
      ...
      break;
}
```

### Check tree line number in document

C:
```c
#include <paraconf.h>

// parse yaml document from file
PC_tree_t root = PC_parse_path("example.yml");

// Get root line in document
int root_line = PC_line(root); 

// destroy root tree at the end
PC_tree_destroy(&root);
```

Fortran:
```fortran
type(pc_tree_t) :: root
integer         :: root_line

call PC_parse_path("example.yml", root)

root_line = PC_line(root)

! destroy root tree at the end
call PC_tree_destroy(root)
```

C++:
```cpp
#include <paraconf/PC_node.h>

// parse yaml document from file
PC_node root = PC_load_file("example.yml");

// Get root line in document
int root_line = root.line(); 
```

### Get yaml-cpp node from PC_tree_t
```cpp
#include <paraconf.h>
#include <PC_node.h>
#include <yaml-cpp/yaml.h>

PC_tree_t root = PC_parse_path("example.yml");

//Get PC_node from PC_tree_t
PC_node& pc_node = *root;

//Get yaml-cpp node from PC_tree_t
YAML::Node root_node = root->node();

// destroy root tree at the end
PC_tree_destroy(&conf);
```

### Include other yaml as subtree
Main yaml:
```yaml
some_scalar: scalar
some_sequence: !include "some_sequence.yaml"
some_map: !include "some_map.yaml"
```

`some_sequence.yaml`:
```yaml
[0, 2, 4, 6]
```

`some_map.yaml`:
```yaml
key_1: value_1
inner_sequence: !include "some_sequence.yaml"
key_3: {k1: v1, k2: v2}
```
