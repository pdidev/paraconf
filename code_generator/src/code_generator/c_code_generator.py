from structure_types import Struct_Include

_INDENTATION_SPACE_ = 4
_HEADER_STRING_ = '_YAML2STRUCT_C_TYPES_DEFINITION_'


def generate_c_structure_code(IR, schedule):
    """This function gathers all the expressions (lines) required to produce structure code"""

    print('Generating code expressions', end=' ... ')

    c_code_expressions = []
    indent_level = 0

    # We generate the header
    generate_header(c_code_expressions, indent_level, _HEADER_STRING_)
    insert_space(c_code_expressions)

    # We write the typedef definitions
    generate_typedef(c_code_expressions, indent_level, IR, schedule)
    insert_space(c_code_expressions)

    # We iterate over all sub-schedules (the deepest is the first)
    for i, sub_schedule in enumerate(schedule):

        # Writing the types' depth in a comment
        insert_space(c_code_expressions)
        c_code_expressions.append((indent_level, '// Depth ' + str(len(schedule)-i) + ' types --- --- ---'))
        insert_space(c_code_expressions)

        # We iterate over each type in the sub-schedule
        for type in sub_schedule:

            # If there is at least one sub-type (not defined by an include)
            if not IR[type].is_leaf() and not isinstance(IR[type].primitive_type, Struct_Include):
                generate_struct(c_code_expressions, indent_level, IR, type)
                insert_space(c_code_expressions)

            # If the type is defined by an include
            elif isinstance(IR[type].primitive_type, Struct_Include):
                c_code_expressions.append((indent_level, 'typedef ' + IR[type].primitive_type.included_type_name + ' ' + type + ';'))
                insert_space(c_code_expressions)

            # If the type is primitive (not defined by the user) and root (no parent)
            # elif IR[type].is_root():   # Useless since the schedule is optimized (all non-root primitive types are suppressed)
            else:
                generate_primitive(c_code_expressions, indent_level, IR, type)
                insert_space(c_code_expressions)

    insert_space(c_code_expressions)
    c_code_expressions.append((indent_level, '#endif'))

    print('[Success, {} lines generated]'.format(len(c_code_expressions)))

    return c_code_expressions


def generate_typedef(c_code_expressions, indent_level, IR, schedule):
    """Generate expressions corresponding to a typedef"""

    for sub_schedule in schedule:
        for element in sub_schedule:
            if not IR[element].is_leaf() and not isinstance(IR[element].primitive_type, Struct_Include):
                c_code_expressions.append((indent_level, 'typedef struct {} {};'.format(element[:-2]+'_s', element)))


def generate_struct(code_expressions, indent_level, IR, type):
    """Generate expressions defining a struct type"""

    code_expressions.append((indent_level, 'struct ' + type[:-2] + '_s {'))
    # We iterate over all the sub-types required by type
    for sub_type in IR[type].dependencies:
        # If the sub-type is a leaf it corresponds to a primitive type
        if IR[sub_type].is_leaf():
            declaration = IR[sub_type].declare()
            code_expressions.append((indent_level+1, declaration))
        elif not isinstance(IR[sub_type].primitive_type, Struct_Include):
            generate_struct(code_expressions, indent_level+1, IR, sub_type)
        # Else the sub-type is defined by the user in an include
        else:
            declaration = IR[sub_type].dependencies[0] + ' ' + IR[sub_type].var_name + ';'
            code_expressions.append((indent_level+1, declaration))
    if indent_level == 0:
        code_expressions.append((indent_level, '};'))
    else:
        code_expressions.append((indent_level, '} ' + IR[type].var_name + ';'))


def generate_primitive(code_expressions, indent_level, IR, type):
    """Generate expressions defining a prmimtive (predifined) type"""
    declaration = 'typedef {}'.format(IR[type].declare(name=type))
    code_expressions.append((indent_level, declaration))


def generate_header(code_expressions, indent_level, string):
    """Generate header expressions (lines)"""
    code_expressions.append((indent_level, '#ifndef ' + string))
    code_expressions.append((indent_level, '#define ' + string))


def insert_space(code_expressions, n=1):
    """Appends n vertical spaces (lines) to the code expressions"""
    for i in range(n):
        code_expressions.append((0, ''))


def dump_c_code(code_expressions):
    """Return a string containing all the code"""
    code = ''

    for expression in code_expressions:
        indent_level = expression[0]
        string = expression[1]
        indentation = ''
        for i in range(indent_level*_INDENTATION_SPACE_):
            indentation += ' '
        code += indentation + string + '\n'

    return code
