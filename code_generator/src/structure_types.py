from yamale.validators import *


class Struct_Type():
    
    def __init__(self, type=Null(), pointer_order=0):
        if isinstance(type, Boolean):
            self.__class__ = Struct_Boolean
            self.__init__(pointer_order=pointer_order+type.is_optional)
        elif isinstance(type, Include):
            self.__class__ = Struct_Include
            self.__init__(name=type.args[0], pointer_order=pointer_order+type.is_optional)
        elif isinstance(type, Integer):
            self.__class__ = Struct_Integer
            self.__init__(pointer_order=pointer_order+type.is_optional)
        elif isinstance(type, List):
            # self.__init__(type=type.validators[0], pointer_order=pointer_order+1+type.is_optional)
            if len(type.validators) == 1:
                self.__class__ = Struct_Type
                self.__init__(type=type.validators[0], pointer_order=pointer_order+1+type.is_optional)
            else:
                self.__class__ = Struct_List
                self.__init__(sub_types=[Struct_Type(type=k, pointer_order=0) for k in type.validators], pointer_order=pointer_order+1+type.is_optional)
        elif isinstance(type, Map):
            self.__class__ = Struct_Map
            self.__init__(sub_types=[Struct_Type(type=k, pointer_order=0) for k in type.validators], sub_class=type.validators[0], pointer_order=pointer_order+1+type.is_optional)
        elif isinstance(type, Null):
            self.__class__ = Struct_Null
            self.__init__(pointer_order=pointer_order+type.is_optional)
        elif isinstance(type, Number):
            self.__class__ = Struct_Float
            self.__init__(pointer_order=pointer_order+type.is_optional)
        elif isinstance(type, String):
            self.__class__ = Struct_String
            self.__init__(pointer_order=pointer_order+type.is_optional)
        else:
            raise Exception('Unrecognised validator: {}'.format(type))

    def make_pointer_string(self):
        string = ''
        for i in range(self.pointer_order):
            string += '*'
        return string



class Struct_Boolean(Struct_Type):
    """Boolean primitive type"""

    def __init__(self, pointer_order=0):
        self.pointer_order = pointer_order
        self.C_tag = 'int'
        self.Fortran_tag = 'INTEGER'

    def __str__(self):
        return 'Boolean{}()'.format(self.make_pointer_string())

    def declare(self, name):
        return 'int{} {};'.format(self.make_pointer_string(), name)



class Struct_Float(Struct_Type):
    """Float primitive type"""

    def __init__(self, pointer_order=0):
        self.pointer_order = pointer_order
        self.C_tag = 'double'
        self.Fortran_tag = 'DOUBLE PRECISION'

    def __str__(self):
        return 'Float{}()'.format(self.make_pointer_string())

    def declare(self, name):
        return 'double{} {};'.format(self.make_pointer_string(), name)



class Struct_Include(Struct_Type):
    """Include primitive type"""

    def __init__(self, name, pointer_order=0):
        self.pointer_order = pointer_order
        self.included_type_name = name
        self.C_tag = name+'_t'

    def __str__(self):
        return "Include{}('{}_t')".format(self.make_pointer_string(), self.included_type_name)

    def declare(self, name):
        return '{}{} {};'.format(self.C_tag, self.make_pointer_string(), name)



class Struct_Integer(Struct_Type):
    """Integer primitive type"""

    def __init__(self, pointer_order=0):
        self.pointer_order = pointer_order
        self.C_tag = 'int'
        Fortran_tag = 'INTEGER'

    def __str__(self):
        return 'Integer{}()'.format(self.make_pointer_string())

    def declare(self, name):
        return 'int{} {};'.format(self.make_pointer_string(), name)



class Struct_List(Struct_Type):
    """Map primitive type"""

    def __init__(self, sub_types = [], pointer_order=0):

        self.pointer_order = pointer_order
        self.sub_types = sub_types
        self.C_tag = 'CHANGE_ME_t' # TO BE CHANGED!

    def __str__(self):
        return 'List{}({})'.format(self.make_pointer_string(), str(self.sub_types))

    def declare(self, name):
        return '{}{} {};'.format(self.C_tag, self.make_pointer_string(), name)



class Struct_Map(Struct_Type):
    """Map primitive type"""

    def __init__(self, sub_types = [], sub_class=Null(), pointer_order=0):

        self.pointer_order = pointer_order
        self.sub_types = sub_types
        self.keys_sub_class = Struct_String()

        if isinstance(sub_class, Boolean):
            self.sub_class = Struct_Boolean()
            self.values_type_name = 'boolean'
        elif isinstance(sub_class, Number):
            self.sub_class = Struct_Float()
            self.values_type_name = 'double'
        elif isinstance(sub_class, Integer):
            self.sub_class = Struct_Integer()
            self.values_type_name = 'integer'
        elif isinstance(sub_class, String):
            self.sub_class = Struct_String()
            self.values_type_name = 'string'
        elif isinstance(sub_class, Include):
            self.sub_class = Struct_Include(name=sub_class.get_name())
            self.values_type_name = 'include'
        elif isinstance(sub_class, List):
            self.sub_class = Struct_Type(type=sub_class, pointer_order=1)
            self.values_type_name = 'list'
        elif isinstance(sub_class, Null):
            self.sub_class = Struct_Null()
        else:
            raise Exception('Unrecognised validator: {}'.format(sub_types))

        if self.values_type_name != 'include':
            self.C_tag = 'MAP_ITEM_{}_t'.format(self.values_type_name)
        else:
            self.C_tag = 'MAP_ITEM_{}_t'.format(self.sub_class.included_type_name)

    def __str__(self):
        return 'Map{}({}, {})'.format(self.make_pointer_string(), str(self.keys_sub_class), str(self.sub_class))

    def declare(self, name):
        return '{}{} {};'.format(self.C_tag, self.make_pointer_string(), name)



class Struct_None(Struct_Type):

    def __init__(self):
        pass
    
    def __str__(self):
        return 'None()'

    

class Struct_Null(Struct_Type):

    def __init__(self):
        pass
    
    def __str__(self):
        return 'Null()'



class Struct_String(Struct_Type):
    """String primitive type"""

    def __init__(self, pointer_order=0):
        self.pointer_order = pointer_order
        self.C_tag = 'char*'
        self.Fortran_tag = 'CHARACTER (LEN=*)'

    def __str__(self):
        return 'String{}()'.format(self.make_pointer_string())

    def declare(self, name):
        return 'char{}* {};'.format(self.make_pointer_string(), name)
