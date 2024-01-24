# -*- coding: utf-8 -*
import os
import re
def file_name(file_dir):
    allFileName = [];
    for root, dirs, files in os.walk(file_dir):
        allFileName += files
    result  = list(allFileName)
    for fileName in result :
        res = re.search("^[a-zA-Z]+[0-9]+\.cs$",fileName)
        if res is None:
            allFileName.remove(fileName)
    return allFileName

def read_file(file):
    result = []
    with open(file,'r') as f:
        for line in f:
            res = re.findall(r"[a-zA-Z]+[0-9]+\.cs", line)
            if res:
                result += res
    return result

def main():
    allFileName = file_name("../CSharp")
    result = read_file("CSharp.csproj")
    allFileName.sort()
    result.sort()
    if allFileName != result:
        if len(allFileName) > len(result):
            diff = list(set(allFileName) - set(result))
            diff.sort()
            raise Exception('The existing file is larger than the running file ',diff)
        elif len(allFileName) < len(result):
            diff = list(set(result) - set(allFileName))
            diff.sort()
            raise Exception(diff)
        else:
            raise Exception('expected is ',allFileName,' , actual is ',result)

if __name__ == "__main__":
    main()