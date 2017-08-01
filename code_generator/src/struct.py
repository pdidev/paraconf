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
        self._handle_map_types()
        max_depth = self._update_depth()
        self._replace_forbidden_chars()
        schedule = self._make_schedule(max_depth)
        schedule = self._optimize(schedule)
        return self.structure, schedule


    def _process_structure(self):
        """Extract usefull information from self._schema and put it in dict self.structure"""
        for key, value in self._schema.items():
            # We handle the case where the key has a mother-key
            parent = []
            var_name = []
            if '.' in key:
                parent_key = key.split('.')[0]
                if parent_key not in self.structure.keys():
                    # We gather the dependencies (types/variables names)
                    dependencies = []
                    for _key in self._schema.keys():
                        if parent_key in _key and parent_key+'_t' != _key:
                            dependencies.append(_key+'_t')
                            var_name = _key.split('.')[0]
                    self.update_struct(name=parent_key+'_t', struct_var_name=var_name, struct_dependencies=dependencies)
                parent = [parent_key+'_t']
            self.update_struct(name=key+'_t', struct_var_name=key.split('.')[-1], struct_type=value, struct_parents=parent)

        for sub_schema in self.includes.values():
            parent_key = sub_schema.name
            dependencies = [parent_key+'.'+k+'_t' for k in sub_schema._schema.keys()]
            self.update_struct(name=parent_key+'_t', struct_var_name=parent_key, struct_dependencies=dependencies)
            for key, value in sub_schema._schema.items():
                parent = [parent_key+'_t']
                self.update_struct(name=parent_key+'.'+key+'_t', struct_var_name=key, struct_type=value, struct_parents=parent)


    def _handle_map_types(self):
        """Create MAP_ITEM_<type>_t sub-types and merge all dependencies"""

        print('Handling Map types', end=' ... ')

        # We gather all keys corresponding to a map and the type of associated values
        L = []
        for key in self.structure.keys():
            if isinstance(self.structure[key].primitive_type, Struct_Map):
                L.append((key, self.structure[key].primitive_type.tag))

        # We iterate over each element of L
        for element in L:
            key = element[0]
            values_type_name = element[1]
            self.structure[key].dependencies.append(values_type_name)

            if 'ITEM_KEY_t' not in self.structure.keys():
                self.update_struct(name='ITEM_KEY_t', struct_parents=[values_type_name], struct_var_name='key')

            # The case where the values' type is defined by an include is treated apart
            if self.structure[key].primitive_type.values_type_name != 'include':
                self.update_struct(name='ITEM_{}_t'.format(self.structure[key].primitive_type.values_type_name), struct_parents=[values_type_name], struct_var_name='value')
                self.update_struct(name=values_type_name, struct_dependencies=['ITEM_KEY_t', 'ITEM_{}_t'.format(self.structure[key].primitive_type.values_type_name)], struct_var_name=values_type_name[:-2], struct_parents=[key])
            else:
                self.update_struct(name='ITEM_{}_t'.format(self.structure[key].primitive_type.sub_class.included_type_name), struct_parents=[values_type_name], struct_dependencies=[self.structure[key].primitive_type.sub_class.included_type_name+'_t'], struct_var_name='value')
                self.update_struct(name=values_type_name, struct_dependencies=['ITEM_KEY_t', 'ITEM_{}_t'.format(self.structure[key].primitive_type.sub_class.included_type_name)], struct_var_name=values_type_name[:-2], struct_parents=[key])
                # The included type has the map values' type as parent
                self.structure[self.structure[key].primitive_type.sub_class.included_type_name+'_t'].parents.append('ITEM_{}_t'.format(self.structure[key].primitive_type.sub_class.included_type_name))

            if self.structure[key].primitive_type.values_type_name == 'boolean':
                self.structure['ITEM_boolean_t'].primitive_type = Struct_Bool()
            if self.structure[key].primitive_type.values_type_name == 'integer':
                self.structure['ITEM_integer_t'].primitive_type = Struct_Integer()
            elif self.structure[key].primitive_type.values_type_name == 'double':
                self.structure['ITEM_double_t'].primitive_type = Struct_Float()
            elif self.structure[key].primitive_type.values_type_name == 'string':
                self.structure['ITEM_string_t'].primitive_type = Struct_String()
            elif self.structure[key].primitive_type.values_type_name == 'include':
                self.structure['ITEM_{}_t'.format(self.structure[key].primitive_type.sub_class.included_type_name)].primitive_type = Struct_Include(name=self.structure[key].primitive_type.sub_class.included_type_name+'_t')

            self.structure[key].primitive_type = Struct_Include(name=self.structure[key].dependencies[0]+'*')

        if len(L) != 0:
            self.structure['ITEM_KEY_t'].primitive_type = Struct_String()

        print('[Success, {} Maps were found]'.format(len(L)))
            

    def _merge_included_types(self):
        """Detect all included types and merge them"""
        for key in self.structure.keys():
            if isinstance(self.structure[key].primitive_type, Struct_Include):
                if key[:-2] in self._schema.keys():
                    dependence_name = self._schema[key[:-2]].get_name()+'_t'
                else:
                    dependence_name = self.includes[key[:-2].split('.')[0]]._schema[key[:-2].split('.')[1]].get_name()+'_t'
                print('Found one include at ' + key, end=' ... ')
                self.structure[key].dependencies.append(dependence_name)
                self.structure[key].dependencies.sort()
                self.structure[key].primitive_type.included_type_name = dependence_name
                self.structure[dependence_name].parents.append(key)
                self.structure[dependence_name].parents.sort()
                print('[Merged with {}]'.format(dependence_name))


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
        schedule = [[] for i in range(max_depth+1)]
        for key in self.structure.keys():
            schedule[max_depth-self.structure[key].depth].append(key)
        for element in schedule:
            element.sort()

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
                # If the key is a leaf and not a root it is useless
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


    def update_struct(self, name, struct_type=[], struct_var_name=None, struct_dependencies=[], struct_parents=[]):
        if name not in self.structure.keys():
            self.structure[name] = _Structure()

        self.structure[name].var_name = struct_var_name
        self.structure[name].dependencies = struct_dependencies[:]
        self.structure[name].parents.extend(struct_parents)

        if isinstance(struct_type, List):
            self.structure[name].primitive_type = Struct_Array(sub_class=self._schema[name[:-2]].args[0])
        elif isinstance(struct_type, Map):
            self.structure[name].primitive_type = Struct_Map(sub_class=self._schema[name[:-2]].args[0])
        elif isinstance(struct_type, Boolean):
            self.structure[name].primitive_type = Struct_Boolean()
        elif isinstance(struct_type, Integer):
            self.structure[name].primitive_type = Struct_Integer()
        elif isinstance(struct_type, Null):
            self.structure[name].primitive_type = Struct_Null()
        elif isinstance(struct_type, Number):
            self.structure[name].primitive_type = Struct_Float()
        elif isinstance(struct_type, String):
            self.structure[name].primitive_type = Struct_String()
        elif isinstance(struct_type, Include):
            self.structure[name].primitive_type = Struct_Include()


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


class _Structure():
    def __init__(self):
        self.parents = []
        self.depth = 0
        self.var_name = None
        self.dependencies = []
        self.primitive_type = Struct_None()

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
        new_depth = walk_through_struct(struct, dependency, depth=depth+1)
        if max_depth < new_depth:
            max_depth = new_depth
    return max_depth


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
