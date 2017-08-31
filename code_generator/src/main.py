import argparse, yamale
from c_code_generator.c_data_loader import C_DataLoader
from c_code_generator.c_free_memory import c_free_root
from c_code_generator.c_types_generator import C_TypesGenerator
from c_code_generator.c_functions import MAIN_FUNCTION


def _run(schema_path, output_path, parser):
    schema = yamale.make_schema(schema_path, parser)

    c_types_header = C_TypesGenerator(schema)
    c_types_header.define_types()
    c_types_header.dump_types_definition(output_path)

    c_loader = C_DataLoader(schema, init_name='pcgen_loader', main_name='main', type_name='types')
    c_loader.gen_init_code()

    c_free_code, c_free_header = c_free_root(schema)
    c_loader.init_code.extend(c_free_code)
    c_loader.init_header.extend(c_free_header)
    c_loader.dump_code()
    c_loader.dump_header()

    f = open('main.c', 'w')
    f.write(MAIN_FUNCTION)
    f.close()


def main():
    parser = argparse.ArgumentParser(description='Generate C/Fortran code from YAML data and schema', prog='pcgen')
    parser.add_argument('schema', metavar='SCHEMA', default='schema.yaml', nargs='?',
                        help='filename of schema, default is ./schema.yaml')
    parser.add_argument('-o', '--output', default='types.h',
                        help='output name for the types definition file, default is "types.h"')
    parser.add_argument('-p', '--parser', default='ruamel',
                        help='YAML library to load files, choices are "PyYAML" or "ruamel" (default)')
    args = parser.parse_args()
    _run(args.schema, args.output, args.parser)


if __name__ == '__main__':
    main()
