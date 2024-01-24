#!/usr/bin/python
import platform
import os
import sys
from os.path import join, dirname, abspath

root_dir=os.getcwd()
SYS_STR=platform.system()
SCRIPT_PATH=root_dir
SOURCE=""
TARGET=""
ISDIR=False
TOTAL_NUM=0
SUCCESS_NUM=0
FAILURE_NUM=0
# path of parsing program
PROG_PATH=join(SCRIPT_PATH, 'sundown-master')
PROG_PATH=join(PROG_PATH, 'bin')
if (SYS_STR == "Windows"):
     PROG_PATH=join(PROG_PATH, 'md2TroffTool.exe')
elif (SYS_STR == "Linux"):
     PROG_PATH=join(PROG_PATH, 'md2TroffTool')
# path of markdown file
MD_PATH=join(SCRIPT_PATH, '..')
MD_PATH=join(MD_PATH, '..')
MD_PATH=join(MD_PATH, 'src')
MD_PATH=join(MD_PATH, 'document')
MD_PATH=join(MD_PATH, 'reference')
MD_PATH=join(MD_PATH, 'Sequoiadb_command')
# path of troff file
TROFF_PATH=join(SCRIPT_PATH, '..')
TROFF_PATH=join(TROFF_PATH, '..')
TROFF_PATH=join(TROFF_PATH, 'manual')

# function define
def _error_systax(prog):
    print ("Syntax: %s -i <input file or directory> -o <output file or directory>"%(prog))
    sys.exit(1)

def _tarnsform(input_file, output_file):
    md2md_file=output_file + ".md2md"
    md2troff_file=output_file + ".md2troff"
    
    # 1) convert github's markdown to pandoc's markdown
    cmd=PROG_PATH + " -i " + input_file + " -o " + md2md_file
    ret=os.system(cmd);
    if 0 != ret:
        print("ERROR: failed to convert %s from github's markdown to pandoc's markdown, errno: %d"%(input_file, ret))
        return (-1)
    # 2) convert pandoc's markdown to troff file
    cmd="pandoc -s --column=80 --wrap=auto --from=markdown --to=man --output=" + md2troff_file + " " + md2md_file
    ret=os.system(cmd);
    if 0 != ret:
        print("ERROR: failed to use pandoc to convert %s to %s, errno: %d"%(md2md_file, md2troff_file, ret))
        return (-1)
    # 3) post-processing the troff file: reduce indent in #sample#
    cmd=PROG_PATH + " -i " + md2troff_file + " -o " + output_file + " -p "
    ret=os.system(cmd);
    if 0 != ret:
        print("ERROR: failed to convert %s to %s, errno: %d"%(md2troff_file, output_file, ret))
        return (-1)
    # 4) remvoe the tmp file
    if os.path.exists(md2md_file):
        os.remove(md2md_file)
    if os.path.exists(md2troff_file):
        os.remove(md2troff_file)
    return (0)

def _single():
    global TOTAL_NUM
    global SUCCESS_NUM
    global FAILURE_NUM
    global SOURCE
    global TARGET
    INPUT_FILE=SOURCE
    OUTPUT_FILE=TARGET
    TOTAL_NUM+=1
    ret = _tarnsform(INPUT_FILE, OUTPUT_FILE)
    if 0 != ret:
        FAILURE_NUM += 1
        print("Error: failed to convert %s to %s"%(INPUT_FILE, OUTPUT_FILE))
    else:
        SUCCESS_NUM += 1
    
def _batch():
    global TOTAL_NUM
    global SUCCESS_NUM
    global FAILURE_NUM
    global SOURCE
    global TARGET
    INPUT_DIR=SOURCE
    OUTPUT_DIR=TARGET
    INPUT_FILE=""
    OUTPUT_FILE=""
    fileName=""
    outputFileName=""
    lang=""
    
    # iterator the SOURCE directory and transform the md files
    list = os.listdir(SOURCE)
    for i in range(0, len(list)):
        path = os.path.join(SOURCE, list[i])
        if os.path.isfile(path):
            TOTAL_NUM+=1;
            fileName=list[i];
            matcher=fileName.find("_en")
            if -1 == matcher:
                lang = "cn"
            else:
                lang = "en"
            if "cn" == lang:
                outputFileName = fileName.split(".")[0] + "_cn.troff"
            else:
                outputFileName = fileName.split(".")[0] + ".troff"
            INPUT_FILE=join(INPUT_DIR, fileName)
            OUTPUT_FILE=join(OUTPUT_DIR, outputFileName)
            ret = _tarnsform(INPUT_FILE, OUTPUT_FILE)
            if 0 != ret:
                FAILURE_NUM += 1
                print("Error: failed to convert %s to %s"%(INPUT_FILE, OUTPUT_FILE))
            else:
                SUCCESS_NUM += 1
        
def _displayStatistics():
    global TOTAL_NUM
    global SUCCESS_NUM
    global FAILURE_NUM
    global SOURCE
    print("The result of converting md files in \"%s\" directory to troff files are as below:"%(SOURCE.split("\\")[-1]))
    print("TOTAL:       %d"%(TOTAL_NUM))
    print("SUCCESS_NUM: %d"%(SUCCESS_NUM))
    print("FAILURE_NUM: %d"%(FAILURE_NUM))

# init input and output directory
dir_arr=["Global", "Oma"]
source_array=[]
target_array=[]
dir_num=len(dir_arr)
for dir in dir_arr:
    source_array.append(join(MD_PATH, dir))
    target_array.append(join(TROFF_PATH, dir))

# check input arguments
argc=len(sys.argv)
if argc < 4 :
    _error_systax(sys.argv[0]) 
if sys.argv[1] == "-i":
    SOURCE=sys.argv[2]
elif sys.argv[3] == '-i':
    SOURCE=sys.argv[4]
else:
    _error_systax(sys.argv[0])
    
if sys.argv[1] == "-o":
    TARGET=sys.argv[2]
elif sys.argv[3] == '-o':
    TARGET=sys.argv[4]
else:
    _error_systax(sys.argv[0])
   
# 3) check whether the input and output file and directory exist or not   
if not os.path.exists(SOURCE):
    print ("Error: the input '%s' does not exist."%(SOURCE))
    sys.exit(1)
    
# 4) check the input and output paths are matched or not
if os.path.isdir(SOURCE):
    if not os.path.isdir(TARGET):
        print ("Error: the input is a directory, but the output is not a directory.")
        sys.exit(1)
    else:
        ISDIR=True
elif os.path.isfile(SOURCE):
    if  os.path.exists(TARGET) and not os.path.isfile(TARGET):
        print ("Error: the input is a file, but the output is not a file.")
        sys.exit(1)
    else:
        ISDIR=False
else:
    _error_systax(sys.argv[0])
    
# 5) begin to run
if ISDIR:
    _batch()
else:
    _single()

_displayStatistics();
    
Input_Global_Dir=join(MD_PATH, "Global")
Input_Oma_Dir=join(MD_PATH, "Oma")

Output_Global_Dir=join(TROFF_PATH, 'Global')
Output_Oma_Dir=join(TROFF_PATH, 'Oma')

#print "1:"+root_dir
#print "2:"+PROG_PATH
#print "3:"+MD_PATH
#print "4:"+TROFF_PATH



