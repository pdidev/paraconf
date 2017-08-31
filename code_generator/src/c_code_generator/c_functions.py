INDENT_SPACE = 8
INDENT_STRING = ''
for i in range(INDENT_SPACE):
    INDENT_STRING += ' '


# String loader

LOAD_STRING_NB_INDENTS = 24
LOAD_STRING_FORMAT = ()
for i in range(LOAD_STRING_NB_INDENTS):
    LOAD_STRING_FORMAT += (INDENT_STRING,)
LOAD_STRING_DECLARATION = '''PC_status_t load_string(PC_tree_t tree, char** string_res);'''
LOAD_STRING_DEFINITION = '''PC_status_t load_string(PC_tree_t tree, char** string_res) {

%sPC_errhandler_t errh = PC_errhandler(PC_NULL_HANDLER);

%sint bool_res;
%sPC_status_t status = PC_bool(tree, &bool_res);
%sif (!status) {
%s%sgoto not_a_string;
%s}

%slong int_res;
%sstatus = PC_int(tree, &int_res);
%sif (!status) {
%s%sgoto not_a_string;
%s}

%sdouble double_res;
%sstatus = PC_double(tree, &double_res);
%sif (!status) {
%s%sgoto not_a_string;
%s}

%sstatus = PC_string(tree, string_res);
%sPC_errhandler(errh);
%sreturn status;

not_a_string:
%sPC_errhandler(errh);
%sreturn PC_INVALID_NODE_TYPE;
}''' % LOAD_STRING_FORMAT


# Double loader

LOAD_DOUBLE_NB_INDENTS = 12
LOAD_DOUBLE_FORMAT = ()
for i in range(LOAD_DOUBLE_NB_INDENTS):
    LOAD_DOUBLE_FORMAT += (INDENT_STRING,)
LOAD_DOUBLE_DECLARATION = '''PC_status_t load_double(PC_tree_t tree, double* double_res);'''
LOAD_DOUBLE_DEFINITION = '''PC_status_t load_double(PC_tree_t tree, double* double_res) {

%sPC_errhandler_t errh = PC_errhandler(PC_NULL_HANDLER);

%slong int_res;
%sPC_status_t status = PC_int(tree, &int_res);
%sif (!status) {
%s%sgoto not_a_double;
%s}

%sstatus = PC_double(tree, double_res);
%sPC_errhandler(errh);
%sreturn status;

not_a_double:
%sPC_errhandler(errh);
%sreturn PC_INVALID_NODE_TYPE;
}''' % LOAD_DOUBLE_FORMAT


# Integer loader

LOAD_INT_NB_INDENTS = 4
LOAD_INT_FORMAT = ()
for i in range(LOAD_INT_NB_INDENTS):
    LOAD_INT_FORMAT += (INDENT_STRING,)
LOAD_INT_DECLARATION = '''PC_status_t load_int(PC_tree_t tree, long* int_res);'''
LOAD_INT_DEFINITION = '''PC_status_t load_int(PC_tree_t tree, long* int_res) {

%sPC_errhandler_t errh = PC_errhandler(PC_NULL_HANDLER);

%sPC_status_t status = PC_int(tree, int_res);
%sPC_errhandler(errh);
%sreturn status;
}''' % LOAD_INT_FORMAT


# Boolean loader

LOAD_BOOL_NB_INDENTS = 4
LOAD_BOOL_FORMAT = ()
for i in range(LOAD_BOOL_NB_INDENTS):
    LOAD_BOOL_FORMAT += (INDENT_STRING,)
LOAD_BOOL_DECLARATION = '''PC_status_t load_bool(PC_tree_t tree, int* bool_res);'''
LOAD_BOOL_DEFINITION = '''PC_status_t load_bool(PC_tree_t tree, int* bool_res) {

%sPC_errhandler_t errh = PC_errhandler(PC_NULL_HANDLER);

%sPC_status_t status = PC_bool(tree, bool_res);
%sPC_errhandler(errh);
%sreturn status;
}''' % LOAD_BOOL_FORMAT


# Main function

MAIN_NB_INDENTS = 20
MAIN_FORMAT = ()
for i in range(MAIN_NB_INDENTS):
    MAIN_FORMAT += (INDENT_STRING,)
MAIN_FUNCTION = '''#include <stdio.h>
#include <paraconf.h>
#include "pcgen_init.h"


int main(int argc, char* argv[]) {

%sif (2 > argc) {
%s%sprintf("Error: the path to the config file should be specified\\n\\n");
%s%sreturn EXIT_FAILURE;
%s}

%sPC_tree_t conf = PC_parse_path(argv[1]);

%sroot_t* root = NULL;
%sroot = calloc(1, sizeof(root_t));
%sif (NULL == root) {
%s%sprintf("Error: impossible to allocate memory for the root node\\n\\n");
%s%sreturn EXIT_FAILURE;
%s}

%sload_root(conf, root);

%s/* INSERT YOUR CODE HERE */

        printf("%%g\\n", *(root->node1));
        printf("%%ld\\n", root->node2);

        printf("%%d\\n", *(root->node3.n1));
        printf("%%s\\n", root->node3.n2);
        printf("%%lf\\n", root->node3.n3.sub_node_1);
        printf("%%ld\\n", root->node3.n4->item.long_value);

        printf("%%s\\n", root->node4->tab[2].item.str_value);
        printf("%%s: %%lf\\n", root->node5.map[1].key, root->node5.map[1].item.double_value);

        if (NULL != root->node6) {
            printf("%%s\\n", root->node6->tab[0].item.str_value);
            printf("%%lf\\n", root->node6->tab[1].item.list1_value.tab[1].item.double_value);
        }

        printf("%%s: %%lf\\n", root->node7.item.list0_value.tab[0].item.map0_value.map[1].key, root->node7.item.list0_value.tab[0].item.map0_value.map[1].item.double_value);

        printf("%%s\\n", root->node8->n);

        printf("%%s: {n: %%s}\\n", root->node9.map[0].key, root->node9.map[0].item.XYZ_value->n);

        printf("%%d\\n", root->node10.item.int_value);

        printf("{%%s: %%ld}\\n", root->node11.item.mynode1_value.a.map[0].key, root->node11.item.mynode1_value.a.map[0].item.long_value);
        printf("{b: %%lf}\\n", root->node11.item.mynode1_value.b.item.double_value);

%sfree(root);
%sPC_tree_destroy(&conf);

%sreturn EXIT_SUCCESS;
}
''' % (MAIN_FORMAT)
