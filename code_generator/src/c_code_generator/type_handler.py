from yamale.validators import *
from c_code_generator.tools import replace_chars, make_union_names, find_nested_any


class Type_Handler():

    def __init__(self, type, pointer_order=0, included_types=[]):
        """Initialize the type handler corresponding to a given validator (type)"""

        self.pointer_order = pointer_order
        self.is_optional = False

        if isinstance(type, Any):
            self.__class__ = Union_Handler
            self.__init__(type, included_types=included_types)
        elif isinstance(type, Boolean):
            self.__class__ = Boolean_Handler
            self.__init__(type)
        elif isinstance(type, Include):
            self.__class__ = Include_Handler
            self.__init__(type)
        elif isinstance(type, Integer):
            self.__class__ = Integer_Handler
            self.__init__(type)
        elif isinstance(type, List):
            self.__class__ = List_Handler
            self.__init__(type, included_types=included_types)
        elif isinstance(type, Map):
            self.__class__ = Map_Handler
            self.__init__(type, included_types=included_types)
        elif isinstance(type, Number):
            self.__class__ = Double_Handler
            self.__init__(type)
        elif isinstance(type, String):
            self.__class__ = String_Handler
            self.__init__(type)

    def c_declare(self, name, indent_level, defined_key, path='', included_types=[]):
        
        raise NotImplementedError('c_declare() method not implemented for Type_Handler instances')


    def c_declare_generic(self, indent_level, defined_key):
        """Define a generic node PC_tree_t inside a struct"""
        
        c_code = [(indent_level, 'struct {', defined_key)]
        c_code.append((indent_level+1, 'int len;', defined_key))
        c_code.append((indent_level+1, 'PC_tree_t* node;', defined_key))
        c_code.append((indent_level, '} generic;', defined_key))
        return c_code


    def make_pointer_string(self):
        
        if self.pointer_order==0 and self.is_optional:
            return '*'
            
        string = ''
        for i in range(self.pointer_order):
            string += '*'
            
        return string



class Boolean_Handler(Type_Handler):

    def __init__(self, type):
        self.c_tag = 'int'
        self.is_optional = type.is_optional

    def __str__(self):
        return 'Boolean type'

    def c_declare(self, name, indent_level, defined_key, path='', included_types=[]):
        name = replace_chars(name)
        return [(indent_level, '{}{} {};'.format(self.c_tag, self.make_pointer_string(), name), defined_key)]


class Double_Handler(Type_Handler):

    def __init__(self, type):
        self.c_tag = 'double'
        self.is_optional = type.is_optional

    def __str__(self):
        return 'Double type'

    def c_declare(self, name, indent_level, defined_key, path='', included_types=[]):
        name = replace_chars(name)
        return [(indent_level, '{}{} {};'.format(self.c_tag, self.make_pointer_string(), name), defined_key)]


class Include_Handler(Type_Handler):

    def __init__(self, type):
        self.is_optional = type.is_optional
        self.include_name = replace_chars(type.args[0])

    def __str__(self):
        return 'Include type'

    def c_declare(self, name, indent_level, defined_key, path='', included_types=[]):
        name = replace_chars(name)
        c_code = []
        c_code.append((indent_level, self.include_name + '_t' + self.make_pointer_string() + ' ' + name + ';', defined_key))
        return c_code


class Integer_Handler(Type_Handler):

    def __init__(self, type):
        self.c_tag = 'long'
        self.is_optional = type.is_optional

    def __str__(self):
        return 'Integer type'

    def c_declare(self, name, indent_level, defined_key, path='', included_types=[]):
        name = replace_chars(name)
        return [(indent_level, '{}{} {};'.format(self.c_tag, self.make_pointer_string(), name), defined_key)]


class List_Handler(Type_Handler):

    def __init__(self, type, included_types=[]):
        self.is_optional = type.is_optional
        self._sub_types = type.validators
        self.sub_types = Union_Handler(type, pointer_order=1, included_types=included_types)

    def __str__(self):
        return 'List type'

    def c_declare(self, name, indent_level, defined_key, path='', included_types=[]):
        name = replace_chars(name)
        c_code = [(indent_level, 'struct {', defined_key)]
        c_code.append((indent_level+1, 'int len;', defined_key))
        c_code.extend(self.sub_types.c_declare('tab', indent_level+1, defined_key, path=path))
        c_code.append((indent_level, '}' + self.make_pointer_string() + ' ' + name + ';', defined_key))
        return c_code


class Map_Handler(Type_Handler):

    def __init__(self, type, included_types=[]):
        self.is_optional = type.is_optional
        self._sub_types = type.validators
        self.sub_types = Union_Handler(type, pointer_order=1, has_map_parent=True, included_types=included_types)

    def __str__(self):
        return 'Map type'

    def c_declare(self, name, indent_level, defined_key, path='', included_types=[]):
        name = replace_chars(name)
        c_code = [(indent_level, 'struct {', defined_key)]
        c_code.append((indent_level+1, 'int len;', defined_key))
        c_code.extend(self.sub_types.c_declare('map', indent_level+1, defined_key, path=path))
        c_code.append((indent_level, '}' + self.make_pointer_string() + ' ' + name + ';', defined_key))
        return c_code


class String_Handler(Type_Handler):

    def __init__(self, type):
        if self.pointer_order==0:
            self.pointer_order = 1
        self.c_tag = 'char'
        self.is_optional = type.is_optional

    def __str__(self):
        return 'String type'

    def c_declare(self, name, indent_level, defined_key, path='', included_types=[]):
        name = replace_chars(name)
        return [(indent_level, '{}{} {};'.format(self.c_tag, self.make_pointer_string(), name), defined_key)]
        return c_code


class Union_Handler(Type_Handler):

    def __init__(self, type, pointer_order=0, has_map_parent=False, included_types=[]):
        sub_type_list = type.validators

        sub_type_list = find_nested_any(sub_type_list)
        for sub_type in sub_type_list:
            if isinstance(sub_type, Include):
                if sub_type.args[0] not in included_types and not sub_type.is_optional:
                    included_types.append(sub_type.args[0])
        
        self.sub_type_list = sub_type_list
        self.pointer_order=pointer_order
        self.is_optional = type.is_optional
        self.has_map_parent = has_map_parent

    def __str__(self):
        return 'Union type'

    def c_declare(self, name, indent_level, defined_key, path='', included_types=[]):
        name = replace_chars(name)
        c_code = [(indent_level, 'struct {', defined_key)]
        sorted_type_list = [k for k in self.sub_type_list]
        if len(sorted_type_list)==0: # The union is empty or there is a nested empty any in it (any(val1(), ..., any())) -> array of PC_tree_t
            if self.has_map_parent: # A union of map values is treated separately
                c_code.append((indent_level+1, 'char* key;', defined_key))
            c_code.append((indent_level+1, 'union {', defined_key))
            c_code.append((indent_level+2, 'PC_tree_t node;', defined_key))
            c_code.append((indent_level+1, '}' + ' item;', defined_key))
        else:
            if self.has_map_parent: # A union of map values is treated separately
                c_code.append((indent_level+1, 'char* key;', defined_key))
            enum_types, enum_types_string = make_union_names(sorted_type_list, path)
            c_code.append((indent_level+1, enum_types_string + ' type;', defined_key))
            c_code.append((indent_level+1, 'union {', defined_key))
            for i, _type in enumerate(sorted_type_list):
                type = Type_Handler(type=_type, included_types=included_types)
                type_name = enum_types[i].split('_')[-1].lower() + '_value'
                c_code.extend(type.c_declare(type_name, indent_level+2, defined_key, path=enum_types[i]))
            c_code.append((indent_level+1, '}' + ' item;', defined_key))
        c_code.append((indent_level, '}' + self.make_pointer_string() + ' ' + name + ';', defined_key))
        return c_code

