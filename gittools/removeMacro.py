#!/usr/bin/python
# remove macros in source code

import os
import sys


def is_macro(line):
    if line.startswith('#'):
        return True
    return False


def is_macro_start(line):
    if not is_macro(line):
        return False
    if line.find('endif') != -1:
        return False
    if line.find('elif') != -1:
        return False
    if line.find('if') == -1:
        return False
    return True


def is_macro_elseif(line):
    if not is_macro(line):
        return False
    if line.find('elif') == -1:
        return False
    return True


def is_macro_else(line):
    if not is_macro(line):
        return False
    if line.find('else') == -1:
        return False
    return True


def is_macro_end(line):
    if not is_macro(line):
        return False
    if line.find('endif') == -1:
        return False
    return True


def is_macro_ifndef(line):
    if not is_macro(line):
        return False
    if line.find('ifndef') == -1:
        return False
    return True


def remove_macro(file_name, macro_name):
    if not os.path.exists(file_name):
        raise Exception("file not exists: " + file_name)
    f = open(file_name, 'r')
    is_macro_started = False
    in_macro_else = False
    is_ifndef = False
    depth = 0  # macro nested depth
    for rawLine in f:
        line = rawLine.lstrip()
        if not is_macro_started:
            if is_macro_start(line) and line.find(macro_name) != -1:
                is_macro_started = True
                depth += 1
                if is_macro_ifndef(line):
                    is_ifndef = True
            else:
                sys.stdout.write(rawLine)
        else:
            if is_macro_start(line):
                depth += 1
            elif is_macro_else(line):
                if depth == 1:
                    in_macro_else = True
            elif is_macro_elseif(line):
                raise Exception("do not support macro '#elif' inside macro "
                                + macro_name + " in file: " + file_name)
                pass
            elif is_macro_end(line):
                depth -= 1
                if depth == 0:
                    is_macro_started = False
                    in_macro_else = False
                    is_ifndef = False

            if is_ifndef:
                if not in_macro_else:
                    sys.stdout.write(rawLine)
            elif in_macro_else and (depth > 1 or (not is_macro_else(line))):
                sys.stdout.write(rawLine)
    f.close()


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: ' + sys.argv[0].strip() + ' <file> <macro>')
        sys.exit(1)
    fileName = sys.argv[1].strip()
    macro = sys.argv[2].strip()
    if fileName == '':
        raise Exception("the fileName is empty")
    if macro == '':
        raise Exception("the macro is empty")
    remove_macro(fileName, macro)
