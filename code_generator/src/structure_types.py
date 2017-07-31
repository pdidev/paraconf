from yamale.validators import *

_BUFFER_LENGTH_ = 256


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
        if isinstance(self.sub_class, Struct_String):
            return '{} {}[{}];'.format(self.sub_class.C_tag, name, _BUFFER_LENGTH_)
        return '{}* {};'.format(self.sub_class.C_tag, name)



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

    def __str__(self):
        return 'Include()'

    

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
        return 'Map()'

    def declare(self, name):
        return self.sub_class.C_tag + '* ' + name + '; // CHANGE ME!  <--- Map type'


    
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
        return 'char ' + name + '[' + str(_BUFFER_LENGTH_) + '];'
