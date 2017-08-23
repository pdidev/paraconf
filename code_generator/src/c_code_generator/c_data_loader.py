from yamale.validators import *
from yamale.util import get_value

_INDENT_SPACE_ = 4


class C_DataLoader():

    def __init__(self, schema, data):
        
        self.data = data
        self.schema = schema
        self.main_code = [] # int main() function
        self.init_code = [] # Initialization functions


    def define_init_function(self):

        indent_level = 0

        # self.init_code.append((indent_level, '#include <stdio.h>', None))
        self.init_code.append((indent_level, '#include <paraconf.h>', None))
        self.init_code.append((indent_level, '#include "init.h"', None))

        self._insert_space_init(n=2)

        self.init_code.append((indent_level, 'root_t* load_data(PC_tree_t conf) {'))
        self._insert_space_init()

        # self.init_code.append((indent_level+1, 'PC_tree_t sub_tree;'))
        self.init_code.append((indent_level+1, 'root_t* root = NULL;'))
        self.init_code.append((indent_level+1, 'root = (root_t*) calloc(1, sizeof(root_t));'))
        self._insert_space_init()

        sorted_items = [k for k in self.schema._schema.items()]
        sorted_items.sort()

        for key, validator in sorted_items:
            self._type_init(key, validator, key, validator.is_optional+int(isinstance(validator, String)), indent_level)
            
        self._insert_space_init()
        self.init_code.append((indent_level+1, 'return root;'))
        self._insert_space_init()
        self.init_code.append((indent_level, '}'))


    def _type_init(self, position, validator, c_variable, is_pointer, indent_level):
        
        if not is_pointer:
            
            if isinstance(validator, Boolean):
                self.init_code.append((indent_level+1, 'PC_bool(PC_get(conf, ".{}"), &(root->{}));'.format(position, c_variable))) # Second position : split '.', replace forbidden chars, join '.'
                
            elif isinstance(validator, Integer):
                self.init_code.append((indent_level+1, 'PC_int(PC_get(conf, ".{}"), &(root->{}));'.format(position, c_variable)))
                
            elif isinstance(validator, Number):
                self.init_code.append((indent_level+1, 'PC_double(PC_get(conf, ".{}"), &(root->{}));'.format(position, c_variable)))

            elif isinstance(validator, String):
                self.init_code.append((indent_level+1, 'PC_string(PC_get(conf, ".{}"), &(root->{}));'.format(position, c_variable)))
                
            elif isinstance(validator, Include):
                included_type = self.schema._schema[position].args[0]
                sorted_sub_keys = [k for k in self.schema.includes[included_type]._schema.keys()]
                sorted_sub_keys.sort()
                for sub_key in sorted_sub_keys:
                    is_pointer = self.schema.includes[included_type]._schema[sub_key].is_optional
                    self._type_init(position+'.{}'.format(sub_key), self.schema.includes[included_type]._schema[sub_key], c_variable+'.{}'.format(sub_key), is_pointer, indent_level)
                    
            elif isinstance(validator, List):
                pass
            
            elif isinstance(validator, Map):
                self.init_code.append((indent_level+1, 'PC_len(PC_get(conf, ".{}"), &(root->{}.len));'.format(position, c_variable)))
                self.init_code.append((indent_level+1, 'root->{}.map = calloc(root->{}.len, sizeof root->{}.map);'.format(c_variable, c_variable, c_variable)))
                self.init_code.append((indent_level+1, 'for (int i{} = 0 ; i{} < root->{}.len ; ++i{})'.format(indent_level, indent_level, c_variable, indent_level) + ' {'))
                self.init_code.append((indent_level+2, 'PC_string(PC_get(conf, ".' + position + '{%d}", i'+str(indent_level)+'), &(root->'+c_variable+'.map[i'+str(indent_level)+'].item.key));'))
                # self.init_code.append((indent_level+2, 'char* s;'))
                # self.init_code.append((indent_level+2, 'PC_string(PC_get(conf, ".' + position + '{%d}", i'+str(indent_level)+'), &s);'))
                # self.init_code.append((indent_level+2, 'root->{}.map[i{}].item.key = malloc(strlen(s)*sizeof(char));'.format(c_variable, indent_level)))
                # self.init_code.append((indent_level+2, 'for (unsigned int i{} = 0 ; i{} < strlen(s) ; ++i{})'.format(indent_level+1, indent_level+1, indent_level+1) + ' {'))
                # self.init_code.append((indent_level+3, 'root->{}.map[i{}].item.key[i{}] = s[i{}];'.format(c_variable, indent_level, indent_level+1, indent_level+1)))
                # self.init_code.append((indent_level+2, '}'))
                # self.init_code.append((indent_level+2, 'free(s);'))
                self.init_code.append((indent_level+1, '}'))

                
        else:
            
            if validator.is_optional:
                self.init_code.append((indent_level+1, 'if (PC_NODE_NOT_FOUND != PC_get_status(conf, ".' + c_variable + '")) {'))

                if isinstance(validator, Boolean):
                    self.init_code.append((indent_level+2, 'root->{} = (int*) calloc(1, sizeof(int));'.format(c_variable)))
                    self.init_code.append((indent_level+2, 'PC_bool(PC_get(conf, ".{}"), root->{});'.format(position, c_variable)))

                elif isinstance(validator, Integer):
                    self.init_code.append((indent_level+2, 'root->{} = (long*) calloc(1, sizeof(long));'.format(c_variable)))
                    self.init_code.append((indent_level+2, 'PC_int(PC_get(conf, ".{}"), root->{});'.format(position, c_variable)))

                elif isinstance(validator, Number):
                    self.init_code.append((indent_level+2, 'root->{} = (double*) malloc(sizeof(double));'.format(c_variable)))
                    self.init_code.append((indent_level+2, 'PC_double(PC_get(conf, ".{}"), root->{});'.format(position, c_variable)))

                elif isinstance(validator, String):
                    self.init_code.append((indent_level+2, 'PC_string(PC_get(conf, ".{}"), &(root->{}));'.format(position, c_variable)))

                elif isinstance(validator, Include):
                    included_type = self.schema._schema[position].args[0]
                    self.init_code.append((indent_level+2, 'root->{} = ({}_t*) calloc(1, sizeof({}_t));'.format(c_variable, included_type, included_type)))
                    sorted_sub_keys = [k for k in self.schema.includes[included_type]._schema.keys()]
                    sorted_sub_keys.sort()
                    for sub_key in sorted_sub_keys:
                        is_pointer = self.schema.includes[included_type]._schema[sub_key].is_optional
                        self._type_init(position+'.{}'.format(sub_key), self.schema.includes[included_type]._schema[sub_key], c_variable+'->{}'.format(sub_key), is_pointer, indent_level+1)

                elif isinstance(validator, List):
                    self.init_code.append((indent_level+2, 'root->{} = calloc(1, sizeof(root->{}));'.format(c_variable, c_variable)))
                    self.init_code.append((indent_level+2, 'PC_len(PC_get(conf, ".{}"), &(root->{}->len));'.format(position, c_variable)))
                    self.init_code.append((indent_level+2, 'root->{}->tab = calloc(root->{}->len, sizeof(root->{}->tab));'.format(c_variable, c_variable, c_variable)))
                    self.init_code.append((indent_level+2, 'for (int i = 0 ; i < root->{}->len ; ++i)'.format(c_variable) + ' {'))
                    self.init_code.append((indent_level+3, '//'))
                    self.init_code.append((indent_level+2, '}'))

                elif isinstance(validator, Map):
                    self.init_code.append((indent_level+2, 'root->{} = calloc(1, sizeof(root->{}));'.format(c_variable, c_variable)))
                    self.init_code.append((indent_level+2, 'PC_len(PC_get(conf, ".{}"), &(root->{}->len));'.format(position, c_variable)))
                    self.init_code.append((indent_level+2, 'root->{}->map = calloc(root->{}->len, sizeof(root->{}->map));'.format(c_variable, c_variable, c_variable)))
                    self.init_code.append((indent_level+2, 'for (int i = 0 ; i < root->{}->len ; ++i)'.format(c_variable) + ' {'))
                    self.init_code.append((indent_level+3, 'PC_string(PC_get(conf, ".' + position + '{%d}", i), &(root->'+c_variable+'->map[i].item.key));'))
                    self.init_code.append((indent_level+2, '}'))
                
                self.init_code.append((indent_level+1, '}'))


            else:
                if isinstance(validator, Boolean):
                    self.init_code.append((indent_level+1, 'root->{} = (int*) calloc(1, sizeof(int));'.format(c_variable)))
                    self.init_code.append((indent_level+1, 'PC_bool(PC_get(conf, ".{}"), root->{});'.format(position, c_variable)))

                elif isinstance(validator, Integer):
                    self.init_code.append((indent_level+1, 'root->{} = (long*) calloc(1, sizeof(long));'.format(c_variable)))
                    self.init_code.append((indent_level+1, 'PC_int(PC_get(conf, ".{}"), root->{});'.format(position, c_variable)))

                elif isinstance(validator, Number):
                    self.init_code.append((indent_level+1, 'root->{} = (double*) malloc(sizeof(double));'.format(c_variable)))
                    self.init_code.append((indent_level+1, 'PC_double(PC_get(conf, ".{}"), root->{});'.format(position, c_variable)))

                elif isinstance(validator, String):
                    self.init_code.append((indent_level+1, 'PC_string(PC_get(conf, ".{}"), &(root->{}));'.format(position, c_variable)))
                
                elif isinstance(validator, Include):
                    included_type = self.schema._schema[position].args[0]
                    self.init_code.append((indent_level+1, 'root->{} = ({}_t*) calloc(1, sizeof({}_t));'.format(c_variable, included_type, included_type)))
                    sorted_sub_keys = [k for k in self.schema.includes[included_type]._schema.keys()]
                    sorted_sub_keys.sort()
                    for sub_key in sorted_sub_keys:
                        is_pointer = self.schema.includes[included_type]._schema[sub_key].is_optional
                        self._type_init(position+'.{}'.format(sub_key), self.schema.includes[included_type]._schema[sub_key], c_variable+'->{}'.format(sub_key), is_pointer, indent_level)

                elif isinstance(validator, List):
                    self.init_code.append((indent_level+1, 'root->{} = calloc(1, sizeof(root->{}));'.format(c_variable, c_variable)))
                    self.init_code.append((indent_level+1, 'PC_len(PC_get(conf, ".{}"), &(root->{}->len));'.format(position, c_variable)))
                    self.init_code.append((indent_level+1, 'root->{}->tab = calloc(root->{}->len, sizeof(root->{}->tab));'.format(c_variable, c_variable, c_variable)))
                    self.init_code.append((indent_level+1, 'for (int i = 0 ; i < root->{}->len ; ++i)'.format(c_variable) + ' {'))
                    self.init_code.append((indent_level+1, '}'))

                elif isinstance(validator, Map):
                    self.init_code.append((indent_level+1, 'root->{} = calloc(1, sizeof(root->{}));'.format(c_variable, c_variable)))
                    self.init_code.append((indent_level+1, 'PC_len(PC_get(conf, ".{}"), &(root->{}->len));'.format(position, c_variable)))
                    self.init_code.append((indent_level+1, 'root->{}->map = calloc(root->{}->len, sizeof(root->{}->map));'.format(c_variable, c_variable, c_variable)))
                    self.init_code.append((indent_level+1, 'for (int i = 0 ; i < root->{}->len ; ++i)'.format(c_variable) + ' {'))
                    self.init_code.append((indent_level+2, 'PC_string(PC_get(conf, ".' + position + '{%d}", i), &(root->'+c_variable+'->map[i].item.key));'))
                    self.init_code.append((indent_level+1, '}'))

        if indent_level==0:
            self._insert_space_init()


    def _insert_space_init(self, n=1):
        """Insert n space(s) at the end of depth_level expressions"""

        while n > 0:
            self.init_code.append((0, ''))
            n += -1



    def dump_init(self, init_header_path):

        f = open(init_header_path, "w")

        for line in self.init_code:
            indent = ''
            for i in range(_INDENT_SPACE_*line[0]):
                indent += ' '
            f.write(indent + line[1] + '\n')

        f.close()




def c_generate_header(path):

    code_string = '''#ifndef _PARACONF_DATA_LOADER_GENERATOR_
#define _PARACONF_DATA_LOADER_GENERATOR_

#include "types.h"


root_t* load_data(PC_tree_t conf);
    
#endif
'''
    
    f = open(path, "w")
    f.write(code_string)
