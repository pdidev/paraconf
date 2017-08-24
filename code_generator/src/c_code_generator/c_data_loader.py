from yamale.validators import *
# from yamale.util import get_value
from c_code_generator.tools import replace_chars

INDENT_SPACE = 8
INIT_HEADER = 'PARACONF_DATA_LOADER_H__'


class C_DataLoader():

    def __init__(self, schema, init_name='pcgen_init', main_name='main', type_name='types'):
        """Initialize the data loader"""

        # Code lines will be represented as tuples:
        #     1st element -> indent level
        #     2nd element -> string corresponding to the line of code
        
        self.schema = schema
        self.init_name = init_name  # Name of the init functions file
        self.main_name = main_name  # Name of the main function file
        self.type_name = type_name  # Name of the types' definition file
        
        self.main_code = []         # Main function
        self.init_code = []         # Initialization functions
        self.init_header = []       # Initialization functions' header


    def gen_init_code(self):
        """Generate the initialization code/header"""

        self.gen_init_header()

        nodes_to_initialize = self.gen_init_root_code()
        for node in nodes_to_initialize:
            self.gen_init_node_code(node, 'root->'+node)

        self.init_header.append((0, ''))
        self.init_header.append((0, '#endif'))


    def gen_init_header(self):
        """Generate the beginning of the initialization header"""
        
        self.init_header.append((0, '#ifndef %s' % (INIT_HEADER)))
        self.init_header.append((0, '#define %s' % (INIT_HEADER)))
        self.init_header.append((0, ''))
        self.init_header.append((0, '#include "%s.h"' % (self.type_name)))
        self.init_header.append((0, ''))
        self.init_header.append((0, ''))
        self.init_header.append((0, 'PC_status_t load_root(PC_tree_t conf, root_t* root);'))
        self.init_header.append((0, ''))


    def gen_init_root_code(self):
        """Generate the load_root() function"""

        root_keys = []
        for key in self.schema._schema.keys():
            key = key.split('.')[0]
            if not key in root_keys:
                root_keys.append(key)

        indent_level = 0

        self.init_code.append((indent_level, '#include <paraconf.h>'))
        self.init_code.append((indent_level, '#include "%s.h"' % (self.init_name)))

        self._insert_space_init(n=2)

        self.init_code.append((indent_level, 'PC_status_t load_root(PC_tree_t conf, root_t* root) {'))

        self._insert_space_init()

        self.init_code.append((indent_level+1, 'int root_len;'))
        self.init_code.append((indent_level+1, 'PC_len(conf, &root_len);'))

        self._insert_space_init()

        self.init_code.append((indent_level+1, 'for(int i = 0 ; i < root_len ; ++i) {'))

        self.init_code.append((indent_level+2, 'char* node_name = NULL;'))
        self.init_code.append((indent_level+2, 'PC_string(PC_get(conf, "{%d}", i), &node_name);'))

        key = root_keys[0]
        self.init_code.append((indent_level+2, 'if (!strcmp(node_name, "%s")) {' % (key)))
        self.init_code.append((indent_level+3, 'load_root_%s(conf, root);' % (replace_chars(key))))
        self.init_code.append((indent_level+2, '}'))
        for key in root_keys[1:]:
            self.init_code.append((indent_level+2, 'else if (!strcmp(node_name, "%s")) {' % (key)))
            self.init_code.append((indent_level+3, 'load_root_%s(conf, root);' % (replace_chars(key))))
            self.init_code.append((indent_level+2, '}'))
        
        self.init_code.append((indent_level+2, 'free(node_name);'))
        self.init_code.append((indent_level+1, '}'))

        self._insert_space_init()
        self.init_code.append((indent_level+1, 'return PC_OK;'))
        self.init_code.append((indent_level, '}'))

        return root_keys


    def gen_init_node_code(self, position, c_variable):
        """Generate the load_root_<node>() function"""

        indent_level = 0

        self.init_header.append((0, 'PC_status_t load_root_%s(PC_tree_t conf, root_t* root);' % (replace_chars(position))))
        self.init_header.append((0, ''))

        self._insert_space_init(n=2)
        self.init_code.append((indent_level, 'PC_status_t load_root_%s(PC_tree_t conf, root_t* root) {' % (replace_chars(position))))
        self.init_code.append((indent_level+1, 'printf("%s\\n");' % (position)))
        self.init_code.append((indent_level+1, 'return PC_OK;'))
        self.init_code.append((indent_level, '}'))



    def _insert_space_init(self, n=1):
        """Insert n space(s) at the end of the code list"""

        while n > 0:
            self.init_code.append((0, ''))
            n += -1



    def dump_init_code(self):
        """Dump the initialization functions code in <self.init_name>.c"""

        f = open(self.init_name+'.c', "w")

        for line in self.init_code:
            indent = ''
            for i in range(INDENT_SPACE * line[0]):
                indent += ' '
            f.write(indent + line[1] + '\n')

        f.close()


    def dump_init_header(self):
        """Dump the initialization functions header in <self.init_name>.h"""

        f = open(self.init_name+'.h', "w")

        for line in self.init_header:
            indent = ''
            for i in range(INDENT_SPACE * line[0]):
                indent += ' '
            f.write(indent + line[1] + '\n')

        f.close()
