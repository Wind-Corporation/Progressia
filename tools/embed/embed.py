#!/usr/bin/env python3

usage = \
'''Usage: embed.py --cpp OUT_CPP --header OUT_H [--] [INPUT as PATH]...
Generate C++ source code that includes binary contents of INPUT files.

Each file in INPUT is stored as a resource: a static array of unsigned char.
    It is identified by a PATH. If PATH is "auto", resource path is the path of
    the file relative to this script's working directory with forward slash '/'
    as separator.

Use -- to make sure the following one INPUT is not interpreted as an option.

This script generates two files:

OUT_CPP is a C++ implementation file that includes the contents of INPUT.
OUT_H is a C++ header file that declares several methods of access to the data
    in OUT_CPP. It should be located in the same directory as OUT_H at compile
    time.

OUT_H declares the following symbols:

namespace __embedded_resources {
    struct EmbeddedResource {
        const unsigned char *data;
        std::size_t length;
    };
    EmbeddedResource getEmbeddedResource(const char *path);
}

getEmbeddedResource(const char *path) returns an EmbeddedResource structure that
    contains the pointer to the beginning of the requested resource and its
    length, or {nullptr, 0} if the resource does not exist.'''

import sys
import os
import re
from types import SimpleNamespace
from json import dumps as json_dumps

def fail(*args):
    my_name = os.path.basename(sys.argv[0])
    print(my_name + ':', *args, file=sys.stderr)
    sys.exit(1)


def main():
    # Parse arguments

    out_cpp_path = None
    out_h_path = None
    inputs = []

    argi = 1
    considerOptions = True
    while argi < len(sys.argv):
        arg = sys.argv[argi]

        if considerOptions and arg.startswith('--cpp'):
            if arg == '--cpp':
                argi += 1
                if argi == len(sys.argv):
                    fail('Missing argument for --cpp')
                out_cpp_path = sys.argv[argi]
            elif arg.startswith('--impl='):
                out_cpp_path = arg.removeprefix('--cpp=')
            else:
                fail(f"Unknown option '{arg}'")

        elif considerOptions and arg.startswith('--header'):
            if arg == '--header':
                argi += 1
                if argi == len(sys.argv):
                    fail('Missing argument for --header')
                out_h_path = sys.argv[argi]
            elif arg.startswith('--header='):
                out_h_path = arg.removeprefix('--header=')
            else:
                fail(f"Unknown option '{arg}'")

        elif considerOptions and (arg == '-h' or arg == '--help'):
            sys.exit(0)

        elif considerOptions and arg == '--':
            considerOptions = False

        elif considerOptions and arg.startswith('-'):
            fail(f"Unknown option '{arg}'")

        else:
            if argi + 2 >= len(sys.argv):
                fail(f'Invalid input declaration {sys.argv[argi:]}: '
                     'expected "INPUT as PATH"')
            if sys.argv[argi + 1] != 'as':
                fail(f'Invalid input declaration {sys.argv[argi:argi+3]}: '
                     'expected "INPUT as PATH"')

            the_input = arg
            argi += 2
            name = sys.argv[argi]

            if name == 'auto':
                name = os.path.relpath(the_input).replace(os.sep, '/')

            inputs.append((the_input, name))

        argi += 1

    if out_cpp_path == None:
        fail('--impl not set')

    if out_h_path == None:
        fail('--header not set')

    if len(inputs) == 0:
        fail('No inputs')

    generate_impl(out_cpp_path, out_h_path, inputs)
    generate_header(out_h_path)


def generate_impl(out_cpp_path, out_h_path, inputs):

    try:
        with open(out_cpp_path, 'w', encoding="utf-8") as output:

            output.write(impl.start %
                {'header_name': os.path.basename(out_h_path)})

            variables = {}

            # Open each input
            for number, (input_path, resource_path) in enumerate(inputs):

                variable_name = make_variable_name(resource_path, number)

                if resource_path in variables:
                    fail('Inputs resolve to duplicate resource paths: ' +
                         resource_path)
                variables[resource_path] = variable_name

                try:
                    with open(input_path, 'rb') as input_file:
                        write_bytes(output, input_file, variable_name)

                        if number == len(inputs) - 1:
                            output.write(";\n")
                        else:
                            output.write(",\n")

                except FileNotFoundError as e:
                    fail(f"Input file '{input_path}' does not exist")
                except (PermissionError, OSError) as e:
                    fail(f"Could not read input '{input_path}': {e}")

            output.write(impl.mid)

            # Add EmbeddedResources to lookup table

            for number, (resource, variable) in enumerate(variables.items()):
                output.write(impl.mapping % {
                    'resource_path_quoted': json_dumps(resource),
                    'variable_name': variable})

                if number == len(variables) - 1:
                    output.write("\n")
                else:
                    output.write(",\n")

            output.write(impl.end)

    except (FileNotFoundError, PermissionError, OSError) as e:
        fail(f"Could not write to '{out_cpp_path}': {e}")


def make_variable_name(resource_path, number):
    max_variable_name_length = 255 # very conservative
    max_path_length = max_variable_name_length - \
        len(impl.variable_name.format(number, ''))

    return impl.variable_name % (number,
        re.sub(r'\W', '_', resource_path[-max_path_length:]).upper())


def write_bytes(out_file, in_file, variable_name):

    out_file.write(impl.declar_start % variable_name)

    max_line_length = 79
    line = impl.declar_mid_prefix

    # Process contents in chunks
    while True:
        chunk = in_file.read1(-1)
        if len(chunk) == 0:
            break

        for byte in chunk:

            byte_str = str(byte)
            if len(line) + 1 + len(byte_str) > max_line_length:
                out_file.write(line + '\n')
                line = impl.declar_mid_prefix

            line += byte_str + ','

    out_file.write(line[:-1] + '\n')
    out_file.write(impl.declar_end)


def generate_header(out_h_path):
    try:
        with open(out_h_path, 'w', encoding="utf-8") as output:
            output.write(header)
    except (FileNotFoundError, PermissionError, OSError) as e:
        fail(f"Could not write to '{out_h_path}': {e}")


# Templates

impl = SimpleNamespace(

    start=\
'''/*
 * This file is autogenerated by tools/embed/embed.py. Do not edit directly.
 * Add this file as a compilation unit.
 */

#include <unordered_map>
#include <string>

#include "%(header_name)s"

namespace {
    const unsigned char
''',

    mid=\
'''
    std::unordered_map<std::string,
                       __embedded_resources::EmbeddedResource>
    EMBEDDED_RESOURCES =
    {
''',

    end=\
'''    };
}

namespace __embedded_resources {

    EmbeddedResource getEmbeddedResource(const std::string &path) {
        auto result = EMBEDDED_RESOURCES.find(path);
        if (result == EMBEDDED_RESOURCES.end()) {
            return EmbeddedResource{nullptr, 0};
        }
        return result->second;
    }

}
''',

    mapping=\
'''        {%(resource_path_quoted)s, {
                   %(variable_name)s,
            sizeof(%(variable_name)s)
        }}''',

    declar_start=      "    %s[] = {\n",
    declar_mid_prefix= '        ',
    declar_end=        '    }',

    variable_name='EMBED_%s_%s'
)

header = '''/*
 * This file is autogenerated by tools/embed/embed.py. Do not edit directly.
 * Include this header as necessary.
 */

#pragma once

#include <string>

namespace __embedded_resources {

    struct EmbeddedResource {
        const unsigned char *data;
        std::size_t length;
    };

    EmbeddedResource getEmbeddedResource(const std::string &path);

}
'''

if __name__ == '__main__':
    main()
