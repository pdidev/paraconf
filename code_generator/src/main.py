import argparse, yamale
from c_code_generator.c_types_generator import C_TypesGenerator
from c_code_generator.c_data_loader import C_DataLoader, c_generate_header
from c_code_generator.c_main_generator import generate_main


def _run(schema_path, data_path, output_path, parser):

    schema = yamale.make_schema(schema_path, parser)

    c_types_header = C_TypesGenerator(schema)
    c_types_header.define_types()
    c_types_header.dump_types_definition(output_path)

    if not data_path is None:
        data = yamale.make_data(data_path[0], parser)
        yamale.validate(schema, data)

        c_init = C_DataLoader(schema, data)
        c_init.define_init_function()
        c_init.dump_init('init.c')

        c_generate_header('init.h')
        generate_main('main.c', data_path[0])



def main():
    parser = argparse.ArgumentParser(description='Generate C/Fortran code from YAML data and schema', prog='pcgen')
    parser.add_argument('schema', metavar='SCHEMA', default='schema.yaml', nargs='?',
                        help='filename of schema, default is ./schema.yaml')
    parser.add_argument('-d', '--data', nargs=1, default=None,
                        help='data to load through ParaConf')
    parser.add_argument('-o', '--output', default='types.h',
                        help='output name for the types definition file, default is "types.h"')
    parser.add_argument('-p', '--parser', default='ruamel',
                        help='YAML library to load files, choices are "PyYAML" or "ruamel" (default)')
    args = parser.parse_args()
    _run(args.schema, args.data, args.output, args.parser)



if __name__ == '__main__':
    main()
