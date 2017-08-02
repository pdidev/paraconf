import argparse, yamale
from code_generator.c_code_generator import generate_c_structure_code, dump_c_code
from struct import Structure
from load_files import load_schema, load_data


def _run(data_path, schema_path, parser):
    _s = load_schema(schema_path, parser)
    # _d = load_data(data_path[0], parser)
    # yamale.validate(_s, _d)
    s = Structure(_s)
    IR, schedule = s.transpile() # Generate Intermediate Representation and schedule

    c_code_expressions = generate_c_structure_code(IR, schedule)
    c_code = dump_c_code(c_code_expressions)

    print('C code header has been written to file types.h')
    f = open('types.h', 'w')
    f.write(c_code)
    f.close()


def main():
    parser = argparse.ArgumentParser(description='Generate C/Fortran code from YAML data and schema', prog='yaml2struct')
    parser.add_argument('data_path', metavar='DATA', nargs='?',
                        help='file to compile')
    parser.add_argument('-s', '--schema', default='schema.yaml',
                        help='filename of schema, default is schema.yaml')
    parser.add_argument('-p', '--parser', default='ruamel',
                        help='YAML library to load files, choices are "PyYAML" or "ruamel" (default)')
    args = parser.parse_args()
    _run(args.data_path, args.schema, args.parser)


if __name__ == '__main__':
    main()
