def generate_main(main_path, data_path):

    code_string = '''#include <stdio.h>
#include <paraconf.h>
#include "init.h"


int main(int argc, char* argv[]) {

    PC_tree_t conf = PC_parse_path("%s");
    root_t* root = load_data(conf);

    /* INSERT YOUR CODE HERE */

    printf("%%ld\\n", root->k1);
    printf("%%lf\\n", root->k2);
    printf("%%d\\n", root->k3);
    printf("%%s\\n", root->k4);

    free(root->k4);
    free(root);
    PC_tree_destroy(&conf);

    return EXIT_SUCCESS;
}
''' % (data_path)

    f = open(main_path, "w")
    f.write(code_string)
    f.close()
