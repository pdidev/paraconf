#include "paraconf.h"

PC_status_t PC_as_len(yaml_document_t* document, yaml_node_t* value_node, int* len);

PC_status_t PC_as_double(yaml_document_t* document, yaml_node_t* value_node, double* value);

PC_status_t PC_as_int(yaml_document_t* document, yaml_node_t* node, int* value);

PC_status_t PC_as_string(yaml_document_t* document, yaml_node_t* value_node, char** value, int *value_len);
