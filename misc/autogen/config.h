#ifndef CONFIG_H
#define CONFIG_H

/* =============== Compilation options and priority =============== */
#define GENERAL_RC_C_FILE           50
#define GENERAL_RC_CPP_FILE         30
#define GENERAL_RC_JS_FILE          10
#define GENERAL_RC_CS_FILE          10
#define GENERAL_RC_JAVA_FILE        10
#define GENERAL_RC_PYTHON_FILE      10
#define GENERAL_RC_DOC_FILE         10
#define GENERAL_RC_WEB_FILE         10

#define GENERAL_OPT_C_FILE          10
#define GENERAL_OPT_CPP_FILE        10
#define GENERAL_OPT_SAMPLE_FILE     10

#define GENERAL_OPT_SAC_FILE        10

#define GENERAL_OPT_DOC_FILE        10

#define GENERAL_FILE_LIST_FILE      10

#define GENERAL_TRACE_FILE          10

#define GENERAL_VER_DOC_FILE        10
#define GENERAL_VER_PYTHON_FILE     10
#define GENERAL_VER_BUILD_FILE      10
#define GENERAL_VER_TOOLS_FILE      10

#define GENERAL_JS_CODE_FILE        20

/* =============== License =============== */
#define GLOBAL_LICENSE "\
   Copyright (C) 2011-%d SequoiaDB Ltd.\n\
\n\
   This program is free software: you can redistribute it and/or modify\n\
   it under the terms of the GNU Affero General Public License as published by\n\
   the Free Software Foundation, either version 3 of the License, or\n\
   (at your option) any later version.\n\
\n\
   This program is distributed in the hope that it will be useful,\n\
   but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
   GNU Affero General Public License for more details.\n\
\n\
   You should have received a copy of the GNU Affero General Public License\n\
   along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"

#define GLOBAL_LICENSE2 "\
/******************************************************************************\n\
\n\
\n\
" GLOBAL_LICENSE "\
\n\
\n\
******************************************************************************/"

/* =============== default statement =============== */
#define DEFAULT_FILE_STATEMENT "\
 This Header File is automatically generated, you MUST NOT modify this file anyway!\n\
 On the contrary, you can modify the file \"%s\" if necessary!\n"

/* =============== language list =============== */
#define LANG_CN   "cn"
#define LANG_EN   "en"
#define LANG_LIST \
   LANG_CN,       \
   LANG_EN,       \

//file path
#define MISC_PATH       "../"
#define ENGINE_PATH     "../../SequoiaDB/engine/"
#define SAC_PATH        "../../SequoiaDB/web/"
#define DRIVER_PATH     "../../driver/"
#define DOC_PATH        "../../doc/"
#define DOCUMENT_PATH   DOC_PATH"src/manual/"
#define CLIENT_PATH     "../../client/"
#define SAMPLES_PATH    "../../conf/samples/"
#define TOOLS_PATH      "../../tools/"

#endif
