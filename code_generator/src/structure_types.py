from yamale.validators import *


class Struct_Array():
    """Array primitive type"""
    # C_tag = 'void*'
    # Fortran_tag = 'INTEGER'

    def __init__(self, sub_class=Null(), type=None):
        if isinstance(sub_class, Number):
            self.sub_class = Struct_Float()
        elif isinstance(sub_class, Integer):
            self.sub_class = Struct_Integer()
        elif isinstance(sub_class, String):
            self.sub_class = Struct_String()
        elif isinstance(sub_class, Null):
            self.sub_class = Struct_Null()
        # self.type = type
        self.C_tag = self.sub_class.C_tag + '*'

    def __str__(self):
        return 'Array({})'.format(str(self.sub_class))

    def declare(self, name):
        return '{}* {};'.format(self.sub_class.C_tag, name)



class Struct_Boolean():
    """Boolean primitive type"""

    def __str__(self):
        return 'Boolean()'

    def declare(self, name):
        return 'int ' + name + ';'



class Struct_Float():
    """Float primitive type"""
    # type = None

    C_tag = 'double'
    Fortran_tag = 'DOUBLE PRECISION'

    def __str__(self):
        return 'Float()'

    def declare(self, name):
        return 'double ' + name + ';'

    

class Struct_Include():
    """Include primitive type"""
    # C_tag = 'int'
    # Fortran_tag = 'INTEGER'

    def __init__(self, name=None):
        self.included_type_name = name

    def __str__(self):
        return 'Include()'

    def declare(self, name):
        return '{} {};'.format(self.included_type_name, name)

    

class Struct_Integer():
    """Integer primitive type"""
    C_tag = 'int'
    Fortran_tag = 'INTEGER'

    def __str__(self):
        return 'Integer()'

    def declare(self, name):
        return 'int ' + name + ';'

    

class Struct_Map():
    """Map primitive type"""
    
    # type = None
    # C_tag = 'void*'
    # Fortran_tag = 'INTEGER'

    def __init__(self, sub_class=Null()):
        
        self.keys_sub_class = Struct_String()
        
        if isinstance(sub_class, Number):
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
        elif isinstance(sub_class, Null):
            self.sub_class = Struct_Null()

        if self.values_type_name != 'include':
            self.tag = 'MAP_ITEM_{}_t'.format(self.values_type_name)
        else:
            self.tag = 'MAP_ITEM_{}_t'.format(self.sub_class.included_type_name)

    def __str__(self):
        return 'Map({}, {})'.format(str(self.keys_sub_class), str(self.sub_class))

    def declare(self, name):
        return self.tag + '* ' + name + ';'


    
class Struct_None():
    def __str__(self):
        return 'None()'

    

class Struct_Null():
    def __str__(self):
        return 'Null()'



class Struct_String():
    """String primitive type"""
    C_tag = 'char*'
    Fortran_tag = 'CHARACTER (LEN=*)'

    def __str__(self):
        return 'String()'

    def declare(self, name):
        return 'char* {};'.format(name)
