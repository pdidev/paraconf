from yamale.validators import *

_AUTHORIZED_CHARACTERS_ = ('_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z')

_FORBIDDEN_INCLUDE_NAMES_ = ('BOOL', 'LONG', 'DOUBLE', 'STR')


class Type_Handler():

    def __init__(self, type, pointer_order=0):

        self.pointer_order = pointer_order
        self.is_optional = False

        if isinstance(type, Any):
            self.__class__ = Union_Handler
            self.__init__(type)
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
            self.__init__(type)
        elif isinstance(type, Map):
            self.__class__ = Map_Handler
            self.__init__(type)
        elif isinstance(type, Number):
            self.__class__ = Double_Handler
            self.__init__(type)
        elif isinstance(type, String):
            self.__class__ = String_Handler
            self.__init__(type)
        # else:
        #     raise Exception('Error: type "{}" does not exist'.format(type))

    def c_declare(self, name, indent_level, defined_key, path=''):
        raise NotImplementedError('c_declare() method not implemented for Type_Handler instances')

    def c_declare_generic(self, indent_level, defined_key):
        c_code = [(indent_level, 'struct {', defined_key)]
        c_code.append((indent_level+1, 'int len;', defined_key))
        c_code.append((indent_level+1, 'PC_tree_t* node;', defined_key))
        c_code.append((indent_level, '}* generic;', defined_key))
        # c_code = [(indent_level, 'PC_tree_t* generic;', defined_key)]
        return c_code

    def make_pointer_string(self):
        string = ''
        if self.pointer_order==0 and self.is_optional:
            string = '*'
        else:
            for i in range(self.pointer_order):
                string += '*'
        return string



class Boolean_Handler(Type_Handler):

    def __init__(self, type):
        self.c_tag = 'int'
        self.is_optional = type.is_optional

    def __str__(self):
        return 'Boolean type'

    def c_declare(self, name, indent_level, defined_key, path=''):
        name = replace_chars(name)
        return [(indent_level, '{}{} {};'.format(self.c_tag, self.make_pointer_string(), name), defined_key)]


class Double_Handler(Type_Handler):

    def __init__(self, type):
        self.c_tag = 'double'
        self.is_optional = type.is_optional

    def __str__(self):
        return 'Double type'

    def c_declare(self, name, indent_level, defined_key, path=''):
        name = replace_chars(name)
        return [(indent_level, '{}{} {};'.format(self.c_tag, self.make_pointer_string(), name), defined_key)]


class Include_Handler(Type_Handler):

    def __init__(self, type):
        self.is_optional = type.is_optional
        self.include_name = replace_chars(type.args[0])

    def __str__(self):
        return 'Include type'

    def c_declare(self, name, indent_level, defined_key, path=''):
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

    def c_declare(self, name, indent_level, defined_key, path=''):
        name = replace_chars(name)
        return [(indent_level, '{}{} {};'.format(self.c_tag, self.make_pointer_string(), name), defined_key)]


class List_Handler(Type_Handler):

    def __init__(self, type):
        self.is_optional = type.is_optional
        self._sub_types = type.validators
        # while len(self._sub_types)==1 and isinstance(self._sub_types[0], List):
        #     self.pointer_order += 1
        #     self._sub_types = self._sub_types[0].validators
        # self.sub_types = Union_Handler(self._sub_types, pointer_order=self.pointer_order)
        self.sub_types = Union_Handler(type, pointer_order=1)

    def __str__(self):
        return 'List type'

    def c_declare(self, name, indent_level, defined_key, path=''):
        name = replace_chars(name)
        c_code = [(indent_level, 'struct {', defined_key)]
        c_code.append((indent_level+1, 'int len;', defined_key))
        c_code.extend(self.sub_types.c_declare('tab', indent_level+1, defined_key, path=path))
        c_code.append((indent_level, '}' + self.make_pointer_string() + ' ' + name + ';', defined_key))
        return c_code


class Map_Handler(Type_Handler):

    def __init__(self, type):
        self.is_optional = type.is_optional
        self._sub_types = type.validators
        self.sub_types = Union_Handler(type, pointer_order=1, has_map_parent=True)

    def __str__(self):
        return 'Map type'

    def c_declare(self, name, indent_level, defined_key, path=''):
        name = replace_chars(name)
        c_code = [(indent_level, 'struct {', defined_key)]
        c_code.append((indent_level+1, 'int len;', defined_key))
        # c_code.append((indent_level+1, 'char* key;', defined_key))
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

    def c_declare(self, name, indent_level, defined_key, path=''):
        name = replace_chars(name)
        return [(indent_level, '{}{} {};'.format(self.c_tag, self.make_pointer_string(), name), defined_key)]
        # c_code = [(indent_level, 'struct {', defined_key)]
        # c_code.append((indent_level+1, 'int len;', defined_key))
        # c_code.append((indent_level+1, 'char* str;', defined_key))
        # c_code.append((indent_level, '}' + self.make_pointer_string() + ' ' + name + ';', defined_key))
        return c_code


class Union_Handler(Type_Handler):

    def __init__(self, type, pointer_order=0, has_map_parent=False):
        sub_type_list = type.validators
        self.sub_type_list = sub_type_list
        self.pointer_order=pointer_order
        self.is_optional = type.is_optional
        self.has_map_parent = has_map_parent

    def __str__(self):
        return 'Union type'

    def c_declare(self, name, indent_level, defined_key, path=''):
        name = replace_chars(name)
        c_code = [(indent_level, 'struct {', defined_key)]
        sorted_type_list = [k for k in self.sub_type_list]
        if len(sorted_type_list)==0: # The union is empty (any()) -> it can contain an array of PC_tree_t
            c_code.extend(self.c_declare_generic(indent_level+1, defined_key))
        else:
            enum_types, enum_types_string = make_union_names(sorted_type_list, path)
            c_code.append((indent_level+1, enum_types_string + ' type;', defined_key))
            c_code.append((indent_level+1, 'union {', defined_key))
            if self.has_map_parent:
                c_code.append((indent_level+2, 'char* key;', defined_key))
            for i, _type in enumerate(sorted_type_list):
                type = Type_Handler(type=_type)
                c_code.extend(type.c_declare('v'+str(i), indent_level+2, defined_key, path=enum_types[i]))
            c_code.append((indent_level+1, '}' + ' item;', defined_key))
        c_code.append((indent_level, '}' + self.make_pointer_string() + ' ' + name + ';', defined_key))
        return c_code


def replace_chars(string):
    """Modify the structure to replace all forbidden characters"""

    new_string = ''
    if string[0] in ('0', '1', '2', '3', '4', '5', '6', '7', '8', '9'):
        new_string += '_'
    for i in range(len(string)):
        if string[i] not in _AUTHORIZED_CHARACTERS_:
            new_string += '_'
        else:
            new_string += string[i]
    return new_string


def make_union_names(validators, path):

    names = []

    any_counter = 0
    list_counter = 0
    map_counter = 0

    validators_to_remove = []

    path = replace_chars(path).upper() + '_'

    for i, validator in enumerate(validators):
        
        if isinstance(validator, Any):
            names.append(path+'ANY{}'.format(any_counter))
            any_counter += 1
        elif isinstance(validator, Boolean):
            if not path+'INT' in names:
                names.append(path+'INT')
            else:
                validators_to_remove.append(i)
        elif isinstance(validator, Integer):
            if not path+'LONG' in names:
                names.append(path+'LONG')
            else:
                validators_to_remove.append(i)
        elif isinstance(validator, List):
            names.append(path+'LIST{}'.format(list_counter))
            list_counter += 1
        elif isinstance(validator, List):
            names.append(path+'MAP{}'.format(map_counter))
            map_counter += 1
        elif isinstance(validator, Number):
            if not path+'DOUBLE' in names:
                names.append(path+'DOUBLE')
            else:
                validators_to_remove.append(i)
        elif isinstance(validator, String):
            if not path+'STR' in names:
                names.append(path+'STR')
        elif isinstance(validator, Include):
            if validator.args[0].upper() in _FORBIDDEN_INCLUDE_NAMES_:
                raise ValueError('Included node ' + validator.args[0] + ' has an invalid name')
            elif not path+validator.args[0].upper() in names:
                names.append(path+validator.args[0].upper())
            else:
                validators_to_remove.append(i)

    for i in validators_to_remove:
        validators.pop(i)
    
    enum_names = 'enum {'
    for name in names[:-1]:
        enum_names += name + ', '
    enum_names += names[-1] + '}'

    return names, enum_names
