import os
import sys
from os.path import join, dirname, abspath

SOURCE_ARRAY = []
TARGET_ARRAY = []
MD_SRC_DIR_NUM = 0
MD_SRC_DIR = [ "Global", "Oma", "Sdb", "SdbCollection", "SdbCS", 
               "SdbCursor", "SdbDomain", "SdbNode", "SdbQuery", "SdbReplicaGroup" ]

SCRIPT_PATH = os.getcwd()
# name of transform script
PROG_NAME=join(SCRIPT_PATH, "md2TroffTool.py")
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

# init input and output directory
MD_SRC_DIR_NUM = len(MD_SRC_DIR)
for dir in MD_SRC_DIR:
    SOURCE_ARRAY.append(join(MD_PATH, dir))
    TARGET_ARRAY.append(join(TROFF_PATH, dir))
    
for i in range(0,MD_SRC_DIR_NUM):
    input = SOURCE_ARRAY[i]
    output = TARGET_ARRAY[i]
    cmd = " python " + PROG_NAME + " -i " + input + " -o " + output
    os.system( cmd )


   

  



