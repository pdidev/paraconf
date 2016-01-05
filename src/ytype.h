#include "paraconf.h"

PC_status_t PC_as_len(PC_tree_t tree, int* len);

PC_status_t PC_as_double(PC_tree_t tree, double* value);

PC_status_t PC_as_int(PC_tree_t tree, int* value);

PC_status_t PC_as_string(PC_tree_t tree, char** value, int *value_len);
