from yamale.schema import Schema
from yamale.validators import *
from structure_types import *


_FORBIDDEN_CHARS_ = (' ', '-', '+', '*', '/', '\\', '.', '(', ')', '[', ']', '|', '%')
_REPLACEMENT_CHAR_ = '_'


class Structure(Schema):
    def __init__(self, schema):
        # self.dict = schema.dict
        self._schema = schema._schema
        # self.name = schema.name
        self.includes = schema.includes
        self.structure = {}


    def transpile(self):
        """Generate Intermediate Representation and schedule"""
        self._process_structure()
        self._merge_included_types()
        # self._handle_complex_types()
        self._handle_list_of_unions_()
        self._handle_map_types()
        max_depth = self._update_depth()
        self._replace_forbidden_chars()
        schedule = self._make_schedule(max_depth)
        schedule = self._optimize(schedule)
        return self.structure, schedule


    def _process_structure(self):
        """Extract usefull information from self._schema and put it in dict self.structure"""

        print('Processing parsed schema', end=' ... ')
        
        for key, value in self._schema.items():
            parent = []
            var_name = []
            keys = key.split('.')
            for i, _key in enumerate(keys):
                dependencies = []
                parents = []
                if i != 0:
                    parents.append('.'.join(keys[:i])+'_t')
                if i != len(keys)-1:
                    dependencies.append('.'.join(keys[:i+2])+'_t')
                    self.update_struct(name='.'.join(keys[:i+1])+'_t', struct_var_name=_key, struct_dependencies=dependencies, struct_parents=parents, struct_path='.'.join(keys[:i+1]))
                else:
                    self.update_struct(name='.'.join(keys[:i+1])+'_t', struct_var_name=_key, struct_type=value, struct_dependencies=dependencies, struct_parents=parents, struct_path='.'.join(keys[:i+1]))

        for sub_schema in self.includes.values():
            parent_key = sub_schema.name
            dependencies = [parent_key+'.'+k+'_t' for k in sub_schema._schema.keys()]
            self.update_struct(name=parent_key+'_t', struct_var_name=parent_key, struct_dependencies=dependencies, struct_path=parent_key)
            for key, value in sub_schema._schema.items():
                parent = [parent_key+'_t']
                self.update_struct(name='.'.join([parent_key, key])+'_t', struct_var_name=key, struct_type=value, struct_parents=parent, struct_path='.'.join([parent_key, key]))

        print('[Success]')


    def _handle_list_of_unions_(self):
        """Detect and handle map types"""

        print('Detecting lists of unions', end=' ... ')

        L = []
        list_types = [k for k in self.structure.keys() if isinstance(self.structure[k].primitive_type, Struct_List)]
        list_types.sort()
        i = 0
        for key in list_types:
            pointer_order = []
            list_sub_types = []
            sub_types = self.structure[key].primitive_type.sub_types
            for sub_type in sub_types:
                pointer_order.append(sub_type.pointer_order)
                if isinstance(sub_type, Struct_List):
                    raise Exception('Error: nested lists not implemented')
                elif isinstance(sub_type, Struct_Map):
                    raise Exception('Error: list of unions does not support maps')
                list_sub_types.append(sub_type)
            L.append((i, key, list_sub_types, pointer_order))
            i += 1

        for element in L:
            n = element[0]
            key = element[1]
            list_sub_types = element[2]
            pointer_order = element[3]

            list_item_name = 'UNION_LIST_{}_t'.format(n)
            types = []
            dependencies = []
            for i in range(len(list_sub_types)):
                s = ''
                for j in range(pointer_order[i]):
                    s += '_'
                types.append(list_sub_types[i])
                if isinstance(types[-1], Struct_Boolean):
                    dependencies.append(s+'ITEM_VALUE_{}_bool_t'.format(i))
                    self.update_struct(name=dependencies[-1], struct_parents=[list_item_name], struct_var_name='value'+str(i))
                elif isinstance(types[-1], Struct_Float):
                    dependencies.append(s+'ITEM_VALUE_{}_double_t'.format(i))
                    self.update_struct(name=dependencies[-1], struct_parents=[list_item_name], struct_var_name='value'+str(i))
                elif isinstance(types[-1], Struct_Include):
                    dependencies.append(s+'ITEM_VALUE_{}_{}_t'.format(i, types[-1].included_type_name))
                    self.update_struct(name=dependencies[-1], struct_parents=[list_item_name], struct_dependencies=[types[-1].included_type_name+'_t'], struct_var_name='value'+str(i))
                    if not dependencies[-1] in self.structure[types[-1].included_type_name+'_t'].parents:
                        self.structure[types[-1].included_type_name+'_t'].parents.append(dependencies[-1])
                elif isinstance(types[-1], Struct_Integer):
                    dependencies.append(s+'ITEM_VALUE_{}_int_t'.format(i))
                    self.update_struct(name=dependencies[-1], struct_parents=[list_item_name], struct_var_name='value'+str(i))
                elif isinstance(types[-1], Struct_String):
                    dependencies.append(s+'ITEM_VALUE_{}_string_t'.format(i))
                    self.update_struct(name=dependencies[-1], struct_parents=[list_item_name], struct_var_name='value'+str(i))

                self.structure[dependencies[-1]].primitive_type = types[-1]
                self.structure[dependencies[-1]].primitive_type.pointer_order = pointer_order[i]+1 # The values of the list are initialized to NULL by default --> the pointer's "order" is at least 1

            self.update_struct(name=list_item_name, struct_parents=[key], struct_dependencies=dependencies)
            self.structure[key].primitive_type = Struct_Include(name=list_item_name[:-2], pointer_order=self.structure[key].primitive_type.pointer_order)
            self.structure[key].dependencies.append(list_item_name)

        print('[Success, {} root lists of unions were found]'.format(len(L)))


    def _handle_complex_types(self):
        """Detect and handle complex types (maps of unions and lists of unions)"""

        print('Detecting nested unions types', end=' ... ')

        _nested_unions = []
        complex_types = [(isinstance(self.structure[k].primitive_type, Struct_Map), k) for k in self.structure.keys() if isinstance(self.structure[k].primitive_type, Struct_List)]

        list_counter = 0
        nested_lists_counter = 0
        map_counter = 0
        nested_maps_counter = 0
        for element in complex_types:
            type = element[0]
            key = element[1]
            list_counter += (type==0)
            map_counter += (type==1)
            _pointer_order = []
            _sub_types = self.structure[key].primitive_type.sub_types
            for sub_type in _sub_types:
                _pointer_order.append(sub_type.pointer_order)
                if isinstance(sub_type, Struct_Map): # Nested map
                    complex_types.append((1, 'NESTED_MAP_{}_t'.format(nested_maps_counter)))
                    self.update_struct(name='NESTED_MAP_{}_t'.format(nested_maps_counter))
                    self.structure['NESTED_MAP_{}_t'.format(nested_maps_counter)].primitive_type = sub_type
                    nested_maps_counter += 1
                elif isinstance(sub_type, Struct_List): # Nested list of unions
                    complex_types.append((1, 'NESTED_UNION_LIST_{}_t'.format(nested_lists_counter)))
                    self.update_struct(name='NESTED_UNION_LIST_{}_t'.format(nested_maps_counter))
                    self.structure['NESTED_UNION_LIST_{}_t'.format(nested_maps_counter)].primitive_type = sub_type
                    nested_lists_counter += 1
            if type == 0:
                _nested_unions.append((type, list_counter, _sub_types, _pointer_order))
            else:
                _nested_unions.append((type, map_counter, _sub_types[:], _pointer_order[:]))

        print()
        for i, union in enumerate(_nested_unions):
            print(complex_types[i][1], [str(k) for k in union[2]])
            print()

        print('[Success]')
                
        

    def _handle_map_types(self):
        """Detect and handle map types"""

        print('Detecting map types', end=' ... ')

        L = []
        map_types = [k for k in self.structure.keys() if isinstance(self.structure[k].primitive_type, Struct_Map)]
        map_types.sort()
        i = 0
        for key in map_types:
            pointer_order = []
            map_sub_types = []
            sub_types = self.structure[key].primitive_type.sub_types
            for sub_type in sub_types:
                pointer_order.append(sub_type.pointer_order)
                if isinstance(sub_type, Struct_Map):
                    raise Exception('Error: nested maps not implemented')
                map_sub_types.append(sub_type)
            L.append((i, key, map_sub_types, pointer_order))
            i += 1

        if len(L) != 0:
            self.update_struct(name='ITEM_KEY_t', struct_parents=map_types, struct_var_name='key')
            self.structure['ITEM_KEY_t'].primitive_type = Struct_String()

        for element in L:
            n = element[0]
            key = element[1]
            map_sub_types = element[2]
            pointer_order = element[3]

            map_item_name = 'MAP_ITEM_{}_t'.format(n)
            types = []
            dependencies = ['ITEM_KEY_t']
            for i in range(len(map_sub_types)):
                s = ''
                for j in range(pointer_order[i]):
                    s += '_'
                types.append(map_sub_types[i])
                if isinstance(types[-1], Struct_Boolean):
                    dependencies.append(s+'ITEM_VALUE_{}_bool_t'.format(i))
                    self.update_struct(name=dependencies[-1], struct_parents=[map_item_name], struct_var_name='value'+str(i))
                elif isinstance(types[-1], Struct_Float):
                    dependencies.append(s+'ITEM_VALUE_{}_double_t'.format(i))
                    self.update_struct(name=dependencies[-1], struct_parents=[map_item_name], struct_var_name='value'+str(i))
                elif isinstance(types[-1], Struct_Include):
                    dependencies.append(s+'ITEM_VALUE_{}_{}_t'.format(i, types[-1].included_type_name))
                    self.update_struct(name=dependencies[-1], struct_parents=[map_item_name], struct_dependencies=[types[-1].included_type_name+'_t'], struct_var_name='value'+str(i))
                    if not dependencies[-1] in self.structure[types[-1].included_type_name+'_t'].parents:
                        self.structure[types[-1].included_type_name+'_t'].parents.append(dependencies[-1])
                elif isinstance(types[-1], Struct_Integer):
                    dependencies.append(s+'ITEM_VALUE_{}_int_t'.format(i))
                    self.update_struct(name=dependencies[-1], struct_parents=[map_item_name], struct_var_name='value'+str(i))
                elif isinstance(types[-1], Struct_String):
                    dependencies.append(s+'ITEM_VALUE_{}_string_t'.format(i))
                    self.update_struct(name=dependencies[-1], struct_parents=[map_item_name], struct_var_name='value'+str(i))

                self.structure[dependencies[-1]].primitive_type = types[-1]
                self.structure[dependencies[-1]].primitive_type.pointer_order = pointer_order[i]+1 # The values are initialized to NULL by default --> the pointer's "order" is at least 1

            self.update_struct(name=map_item_name, struct_parents=[key], struct_dependencies=dependencies)
            self.structure[key].primitive_type = Struct_Include(name=map_item_name[:-2], pointer_order=self.structure[key].primitive_type.pointer_order)
            self.structure[key].dependencies.append(map_item_name)

        print('[Success, {} root maps were found]'.format(len(L)))


    def _merge_included_types(self):
        """Detect all included types and merge them"""

        print('Merging included types', end=' ... ')

        for key in self.structure.keys():
            if isinstance(self.structure[key].primitive_type, Struct_Include):
                if key[:-2] in self._schema.keys():
                    dependence_name = self._schema[key[:-2]].get_name()+'_t'
                else:
                    dependence_name = self.includes[key[:-2].split('.')[0]]._schema[key[:-2].split('.')[1]].get_name()+'_t'
                # print('Found one include at ' + key, end=' ... ')
                self.structure[key].dependencies.append(self.structure[key].primitive_type.included_type_name+'_t')
                self.structure[key].dependencies.sort()
                self.structure[self.structure[key].primitive_type.included_type_name+'_t'].parents.append(key)
                self.structure[self.structure[key].primitive_type.included_type_name+'_t'].parents.sort()
                # print('[Merged with {}]'.format(self.structure[key].primitive_type.included_type_name+'_t'))

        print('[Success]')


    def _update_depth(self):
        """Update the depth of all nodes and return the max depth"""
        
        print('Updating depth of all nodes', end=' ... ')
        max_depth = 0
        for key in self.structure.keys():
            if self.structure[key].is_root():
                new_depth = walk_through_struct(self.structure, key)
                if max_depth < new_depth:
                    max_depth = new_depth
        print('[Success, max depth is {}]'.format(max_depth+1))
        return max_depth


    def _make_schedule(self, max_depth):
        """Return a list storing the keys (types) in the right order"""

        print('Making the schedule', end=' ... ')
        schedule = [[] for i in range(max_depth+1)]
        for key in self.structure.keys():
            schedule[max_depth-self.structure[key].depth].append(key)
        for element in schedule:
            element.sort()

        print('[Success]')

        return schedule


    def _optimize(self, schedule):
        """Optimize the schedule by removing useless intermediate types"""

        print('Removing useless types', end=' ... ')
        
        # We remove all useless keys
        depth_of_removed_keys = [] # Will store the depth and name of included types that are removed
        for i, sub_schedule in enumerate(schedule):
            sub_schedule_list = sub_schedule[:]
            for key in sub_schedule_list:
                # If the key is defined by an include it is useless
                if isinstance(self.structure[key].primitive_type, Struct_Include) and not self.structure[key].is_root():
                    schedule[i].remove(key)
                    depth_of_removed_keys.append((self.structure[key].depth, key))
                # If the key is not a root and is not included in another one it is useless
                elif not self.is_included(key) and not self.structure[key].is_root():
                    schedule[i].remove(key)
                # # If the key is a leaf and not a root it is useless
                elif self.structure[key].is_leaf() and not self.structure[key].is_root():
                    schedule[i].remove(key)

        depth_of_removed_keys = sorted(depth_of_removed_keys, key=lambda x: x[0]) # The deepest is the last to be pulled
        removed_includes = [k[1] for k in depth_of_removed_keys]

        # We pull all dependencies due to intermediate types removal
        pull_dependencies(self.structure, schedule, removed_includes)

        # We shift the depth of all dependencies that will be impacted by the removal of each empty sub-schedule(s)
        for i, sub_schedule in enumerate(schedule[::-1]):
            if len(sub_schedule) == 0:
                # The i-th sub-schedule is empty => all keys (types) with depth >= i are raised by 1
                for key in self.structure.keys():
                    self.structure[key].depth += -(self.structure[key].depth >= i)

        # We remove empty sub-schedule(s)
        schedule = [k for k in schedule[:] if not 0==len(k)]

        print('[Success, new max depth is {}]'.format(len(schedule)))

        return schedule


    def update_struct(self, name, struct_type=[], struct_var_name='', struct_dependencies=[], struct_parents=[], struct_path=''):
        if name not in self.structure.keys():
            self.structure[name] = _Structure()

        self.structure[name].var_name = struct_var_name
        self.structure[name].dependencies.extend([k for k in struct_dependencies[:] if not k in self.structure[name].dependencies])
        self.structure[name].parents.extend([k for k in struct_parents if not k in self.structure[name].parents])
        self.structure[name].path = struct_path
        if len(struct_path) > 0:
            self.set_type_from_name(name)


    def _replace_forbidden_chars(self): # OTHER POSSIBILITY: RAISE AN ERROR IF FORBIDDEN CHARACTER FOUND
        """Modify the structure to replace all forbidden characters"""
        print('Replacing forbidden characters', end=' ... ')
        key_list = [k for k in self.structure.keys()]
        for key in key_list:
            new_key = key.translate({ord(forbidden_char):_REPLACEMENT_CHAR_ for forbidden_char in _FORBIDDEN_CHARS_})
            self.structure[new_key] = self.structure.pop(key)
            for i, dependency in enumerate(self.structure[new_key].dependencies):
                self.structure[new_key].dependencies[i] = dependency.translate({ord(forbidden_char):_REPLACEMENT_CHAR_ for forbidden_char in _FORBIDDEN_CHARS_})
            for i, parent in enumerate(self.structure[new_key].parents):
                self.structure[new_key].parents[i] = parent.translate({ord(forbidden_char):_REPLACEMENT_CHAR_ for forbidden_char in _FORBIDDEN_CHARS_})
            self.structure[new_key].var_name = self.structure[new_key].var_name.translate({ord(forbidden_char):_REPLACEMENT_CHAR_ for forbidden_char in _FORBIDDEN_CHARS_})
            self.structure[new_key].dependencies.sort()
            self.structure[new_key].parents.sort()
        print('[Success]')


    def is_included(self, type):
        """Test if a given type is included by another"""
        for parent in self.structure[type].parents:
            if isinstance(self.structure[parent].primitive_type, Struct_Include):
                return True
        return False

    def is_in_nested_struct(self, name):
        if not self.structure[name].is_root():
            for parent in self.structure[name].parents:
                if not isinstance(self.structure[parent].primitive_type,Struct_None) or self.is_included(parent):
                    return False
            return True
        else:
            return False


    def is_optional(self, path):
        if path in self._schema.keys():
            return self._schema[path].is_optional
        else:
            included_path = path.split('.')
            return self.includes[included_path[0]]._schema[included_path[1]].is_optional


    def get_type_from_key(self, key):
        path = self.structure[key].path
        if path in self._schema.keys():
            return self._schema[path]
        else:
            included_path = path.split('.')
            return self.includes[included_path[0]]._schema[included_path[1]]


    def set_type_from_name(self, name):
        if self.structure[name].is_leaf():
            if self.structure[name].path not in self._schema.keys(): # Key is included in sub-file
                included_path = self.structure[name].path.split('.')
                self.structure[name].primitive_type = Struct_Type(type=self.includes[included_path[0]]._schema[included_path[1]])
            else: # Key is defined in the main file
                self.structure[name].primitive_type = Struct_Type(type=self._schema[self.structure[name].path])
        else:
            self.structure[name].primitive_type = Struct_None()

    

class _Structure():
    def __init__(self):
        self.parents = []
        self.depth = 0
        self.var_name = None
        self.dependencies = []
        self.primitive_type = Struct_None()
        self.path = ''

    def __str__(self):
        if self.is_root():
            str_parents = 'ROOT'
        else:
            if len(self.parents) == 1:
                str_parents = 'PARENT=' + self.parents[0]
            else:
                str_parents = 'PARENTS=' + str(self.parents)
        if self.is_leaf():
            str_object = 'PRIMITIVE_TYPE=' + str(self.primitive_type)
        else:
            str_object = 'DEPENDENCIES=' + str(self.dependencies)
        str_depth = 'DEPTH=' + str(self.depth)
        str_var_name = 'VAR_NAME=' + self.var_name
        return "{}, {}, {}, {}".format(str_parents, str_object, str_var_name, str_depth)

    def is_leaf(self):
        return (len(self.dependencies) == 0)

    def is_root(self):
        return (len(self.parents) == 0)


    def declare(self, name=None):
        if name is None:
            name = self.var_name
        return self.primitive_type.declare(name)



def is_in_nested_struct(struct, name):
    """Test if the key is in a nested structure (in that case its depth will be the same as parent's struct depth)"""
    if not struct[name].is_root():
        for parent in struct[name].parents:
            if struct[parent].is_root() or not isinstance(struct[parent].primitive_type, Struct_None) or is_included(struct, parent):
                return False
        return True
    else:
        return False


    
def is_included(struct, name):
    """Test if a given type is included by another"""
    for parent in struct[name].parents:
        if isinstance(struct[parent].primitive_type, Struct_Include):
            return True
    return False



def pull_dependencies(struct, schedule, removed_includes):
    """Pull all dependencies of removed keys as much as possible"""

    # We iterate over all removed keys (types) to pull the included types
    for removed_key in removed_includes:
        # Every dependency has to be pulled as much as possible
        for sub_key in struct[removed_key].dependencies:
            # We caclulte the max parents' depth
            max_parent_depth = struct[struct[sub_key].parents[0]].depth
            for parent in struct[sub_key].parents:
                if struct[parent].depth > max_parent_depth:
                    max_parent_depth = struct[parent].depth
            depths = [struct[parent].depth for parent in struct[sub_key].parents]

            # We put the dependency in the shallowest sub-schedule possible (= under the deepest parent)
            schedule[-max_parent_depth-1].append(sub_key)
            schedule[-max_parent_depth-1].sort()
            # We remove the dependency from its original sub-schedule
            schedule[-struct[sub_key].depth-1].remove(sub_key)
            # We update the depth
            struct[sub_key].depth = max_parent_depth



def walk_through_struct(struct, key, depth=0):
    """Walk through the struct dict, update all nodes' depth and return the max depth"""

    # If an included type has several parents, its depth is calculated according to the deepest parent
    if struct[key].depth < depth:
        struct[key].depth = depth
    else:
        depth = struct[key].depth

    # We iterate recursively over the dependencies
    max_depth = depth
    for dependency in struct[key].dependencies:
        # new_depth = walk_through_struct(struct, dependency, depth=depth+1)
        new_depth = walk_through_struct(struct, dependency, depth=depth+(not is_in_nested_struct(struct, dependency)))
        if max_depth < new_depth:
            max_depth = new_depth
    return max_depth
