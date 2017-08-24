def generate_main(main_path):

    code_string = '''#include <stdio.h>
#include <paraconf.h>
#include "pcgen_init.h"


int main(int argc, char* argv[]) {

        if (2 > argc) {
                printf("Error: the path to the config file should be specified\\n\\n");
                return EXIT_FAILURE;
        }

        PC_tree_t conf = PC_parse_path(argv[1]);

        root_t* root = NULL;
        root = calloc(1, sizeof(root_t));
        if (NULL == root) {
                printf("Error: impossible to allocate memory for the root node\\n\\n");
                return EXIT_FAILURE;
        }

        load_root(conf, root);

        /* INSERT YOUR CODE HERE */

        free(root);
        PC_tree_destroy(&conf);

        return EXIT_SUCCESS;
}
''' % ()

    f = open(main_path, "w")
    f.write(code_string)
    f.close()
