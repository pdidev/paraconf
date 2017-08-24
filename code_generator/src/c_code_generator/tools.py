from yamale.validators import *

AUTHORIZED_CHARACTERS = ('_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z')

FORBIDDEN_INCLUDE_NAMES = ('BOOL', 'LONG', 'DOUBLE', 'STR')


def replace_chars(string):
    """Modify the structure to replace all forbidden characters"""

    new_string = ''
    if string[0] in ('0', '1', '2', '3', '4', '5', '6', '7', '8', '9'):
        new_string += '_'
    for i in range(len(string)):
        if string[i] not in AUTHORIZED_CHARACTERS:
            new_string += '_'
        else:
            new_string += string[i]
    return new_string


def make_union_names(validators, path):
    """Generate enum names corresponding to the validators and the path to the node"""

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
        elif isinstance(validator, Map):
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
            if validator.args[0].upper() in FORBIDDEN_INCLUDE_NAMES:
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


def find_nested_any(validators):
    """Find all the nested any in "validators" and return a flattened list of validators"""

    nested_count = 0
    sub_types = []

    for validator in validators:
        if isinstance(validator, Any):
            if len(validator.validators)==0:
                return []
            sub_types.extend([k for k in validator.validators if not k in sub_types])
            nested_count += 1
        else:
            sub_types.append(validator)

    if nested_count:
        sub_types = find_nested_any(sub_types)

    return sub_types
