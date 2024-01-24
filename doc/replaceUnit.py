
"""Module of replace the symbol in api files.

"""
import codecs
import os
import sys
import platform
import json

# symbol head. all symbol should had.
SDB_SYMBOL_HEAD = "@SDB_SYMBOL_"

# symbol, use '@SDB_SYMBOL_VERSION' in api file rather than 'SDB_SYMBOL_VERSION'.
SDB_SYMBOL_VERSION = SDB_SYMBOL_HEAD + "VERSION"   # @SDB_SYMBOL_VERSION
# add new symbol in here
# new symbol...

# symbol default value
DEFAULT_VALUE = ""

# if add new symbol,  _symbol_map had to add it and you maybe add a new function to get it's value.
_symbol_map = {
    SDB_SYMBOL_VERSION : DEFAULT_VALUE
    # new symbol...
}


class ReplaceSymbol(object):

    def __init__(self, file_path):
        """init _symbol_map

        """
        # init SDB_SYMBOL_VERSION's value.
        print ("init _symbol_map.")
        version_value = self.__get_sdb_version(file_path)
        _symbol_map[SDB_SYMBOL_VERSION] = version_value
        # if add new symbol, can init it's value in here.
        # ...

        for symbol in _symbol_map:
            print("key:", symbol," value:", _symbol_map[SDB_SYMBOL_VERSION] )


    def replace_in_directory(self, files):
        """replace in a directory.

        :param files:  A directory.
        :return:
        """
        if not os.path.exists(files):
            raise Exception("file :", files, " is not exist")

        for root, dirs,files in os.walk(files):
            for file_name in files:
                self.replace_in_file(os.path.join(root,file_name))


    def replace_in_file(self, file_path):
        """replace in a file.

        :param file_path:  A file.
        :return:
        """

        if not ( file_path.endswith(".html")):
            return
        file_data = ""
        flags = 0
        with codecs.open(file_path, mode='r', encoding='utf-8') as f:
            for line in f:
               if SDB_SYMBOL_HEAD in line:
                   flags = 1
                   line = self.__change_symbol(line)
               file_data +=line
            f.close()
        if  1 == flags :
            with codecs.open(file_path , mode='w', encoding='utf-8') as f:
                f.write(file_data)
                f.close()

    def __change_symbol(self, line):
        """change symbol with it's value.

        """
        for symbol in _symbol_map:
            if symbol in line:
                line = line.replace(symbol, _symbol_map.get(symbol))
        return line


    def __get_sdb_version(self,file_path):
        if not os.path.exists(file_path):
            raise Exception("file :", file_path, " is not exist")

        with codecs.open(file_path, mode='r', encoding='utf-8') as f:
            content = json.load(f)
            major_version = content['major']
            minor_version = content['minor']
            f.close()
        if minor_version is None or major_version is None :
            version = "0"
        elif minor_version < 10:
            version = str(major_version) + "0" + str(minor_version)
        else:
            version = str(major_version) + str(minor_version)
        return version


def __guess_os():
    plat_id = platform.system()
    if plat_id == 'Linux':
        return 'linux'
    elif plat_id == 'Windows' or plat_id == 'Microsoft':
        return 'win32'
    elif plat_id == 'AIX':
        return 'aix'
    else:
        return None

if __name__ == "__main__":
    api_dir = None
    version_file = None
    root_dir = os.path.split( os.path.realpath( sys.argv[0] ) )[0]
    if __guess_os() == 'win32':
        api_dir =  os.path.join(root_dir, 'build\\output\\api')
        version_file = os.path.join(root_dir, 'config\\version.json')
    else:
        api_dir =  os.path.join(root_dir, 'build/output/api')
        version_file = os.path.join(root_dir, 'config/version.json')
    replace = ReplaceSymbol(version_file)
    replace.replace_in_directory(api_dir)
