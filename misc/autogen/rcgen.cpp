#include "rcgen.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace boost::property_tree;
using namespace std;

static string& replace_all ( string &str, const string& old_value, const string &new_value )
{
   for ( string::size_type pos(0) ; pos != string::npos; pos += new_value.length() )
   {
      if ( ( pos = str.find ( old_value, pos ) ) != string::npos )
         str.replace ( pos, old_value.length(), new_value ) ;
      else break ;
   }
   return str ;
}

static bool strStartsWith( const string& str, const string& substr )
{
   if ( str.empty() || substr.empty() )
   {
      return false ;
   }

   return str.compare( 0, substr.size(), substr ) == 0 ;
}

static inline int max(int a, int b)
{
   return (a >= b ? a : b);
}

RCGen::RCGen (const char* lang) : language (lang)
{
    maxErrorNameWidth = 0;
    loadFromXML ();
}

void RCGen::run ()
{
    genC();
    genCPP();
    genCS();
    genJava();
    genJS();
    genPython();
}

void RCGen::loadFromXML ()
{
    ptree pt;

    try
    {
        read_xml (RCXMLSRC, pt);
    }
    catch ( std::exception& )
    {
        cout<<"Can not read xml file, not exist or wrong directory!"<<endl;
        exit(0);
    }

    try
    {
        BOOST_FOREACH (ptree::value_type &v, pt.get_child (CONSLIST))
        {
            pair<string, int> constant (
                v.second.get<string> (NAME),
                v.second.get<int> (VALUE)
            );
            conslist.push_back (constant);
        }

        int i = 0 ;
        BOOST_FOREACH (ptree::value_type &v, pt.get_child (CODELIST))
        {
            ErrorCode errcode ;
            ptree vv = v.second.get_child(DESCRIPTION);
            errcode.name = v.second.get<string>(NAME);
            errcode.desc_cn = vv.get<string>("cn");
            errcode.desc_en = vv.get<string>("en");
            errcode.value = -(i+1);
            if (strStartsWith(errcode.name, RESERVED_ERROR))
            {
               errcode.reserved = true;
            }
            errcodes.push_back(errcode);
            i++;
            maxErrorNameWidth = max(maxErrorNameWidth, (int)errcode.name.length());
        }
    }
    catch ( std::exception&)
    {
        cout<<"XML format error, unknown node name or description language, please check!"<<endl;
        exit(0);
    }
}

void RCGen::genC ()
{
    ofstream fout(CPATH);
    if ( fout == NULL )
    {
        cout<<"can not open file: "<<CPATH<<endl;
        exit(0);
    }
    string comment =
        "/** \\file ossErr.h\n"
        "    \\brief The meaning of the error code.\n"
        "*/\n"
        "/*    Copyright 2012 SequoiaDB Inc.\n"
        " *\n"
        " *    Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        " *    you may not use this file except in compliance with the License.\n"
        " *    You may obtain a copy of the License at\n"
        " *\n"
        " *    http://www.apache.org/licenses/LICENSE-2.0\n"
        " *\n"
        " *    Unless required by applicable law or agreed to in writing, software\n"
        " *    distributed under the License is distributed on an \"AS IS\" BASIS,\n"
        " *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
        " *    See the License for the specific language governing permissions and\n"
        " *    limitations under the License.\n"
        " */\n"
        "/*    Copyright (C) 2011-2014 SequoiaDB Ltd.\n"
        " *    This program is free software: you can redistribute it and/or modify\n"
        " *    it under the term of the GNU Affero General Public License, version 3,\n"
        " *    as published by the Free Software Foundation.\n"
        " *\n"
        " *    This program is distributed in the hope that it will be useful,\n"
        " *    but WITHOUT ANY WARRANTY; without even the implied warrenty of\n"
        " *    MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
        " *    GNU Affero General Public License for more details.\n"
        " *\n"
        " *    You should have received a copy of the GNU Affero General Public License\n"
        " *    along with this program. If not, see <http://www.gnu.org/license/>.\n"
        " */\n";
    fout<<std::left<<comment<<endl;
    comment = "\n// This Header File is automatically generated, you MUST NOT modify this file anyway!\n"
              "// On the contrary, you can modify the xml file \"sequoiadb/misc/autogen/rclist.xml\" if necessary!\n";
    fout<<comment<<endl;

    fout<<"#ifndef OSSERR_H_"<<endl
        <<"#define OSSERR_H_"<<endl<<endl
        <<"#include \"core.h\""<<endl
        <<"#include \"ossFeat.h\""<<endl<<endl;

    for (int i = 0; i < conslist.size(); ++i)
    {
        fout<<"#define "<<setw(maxErrorNameWidth + 2)<<conslist[i].first<<conslist[i].second<<endl;
    }
    fout<<endl;

    comment =
        "/** \\fn CHAR* getErrDesp ( INT32 errCode )\n"
        "    \\brief Error Code.\n"
        "    \\param [in] errCode The number of the error code\n"
        "    \\returns The meaning of the error code\n"
        " */";
    fout<<comment<<endl;
    fout<<"const CHAR* getErrDesp ( INT32 errCode );"<<endl<<endl;

    for (int i = 0; i < errcodes.size(); ++i)
    {
        fout<<"#define "
            <<setw(maxErrorNameWidth + 2)<<errcodes[i].name
            <<setw(6)<<errcodes[i].value
            <<"/**< "<<errcodes[i].getDesc(language)<<" */"<<endl;
    }

    fout<<"#endif /* OSSERR_H_ */";

    fout.close();
}

void RCGen::genCPP ()
{
    ofstream fout(CPPPATH);
    if ( fout == NULL )
    {
        cout<<"can not open file: "<<CPPPATH<<endl;
        exit(0);
    }

    string comment =
        "/*    Copyright 2012 SequoiaDB Inc.\n"
        " *\n"
        " *    Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        " *    you may not use this file except in compliance with the License.\n"
        " *    You may obtain a copy of the License at\n"
        " *\n"
        " *    http://www.apache.org/licenses/LICENSE-2.0\n"
        " *\n"
        " *    Unless required by applicable law or agreed to in writing, software\n"
        " *    distributed under the License is distributed on an \"AS IS\" BASIS,\n"
        " *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
        " *    See the License for the specific language governing permissions and\n"
        " *    limitations under the License.\n"
        " */\n"
        "/*    Copyright (C) 2011-2014 SequoiaDB Ltd.\n"
        " *    This program is free software: you can redistribute it and/or modify\n"
        " *    it under the term of the GNU Affero General Public License, version 3,\n"
        " *    as published by the Free Software Foundation.\n"
        " *\n"
        " *    This program is distributed in the hope that it will be useful,\n"
        " *    but WITHOUT ANY WARRANTY; without even the implied warrenty of\n"
        " *    MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
        " *    GNU Affero General Public License for more details.\n"
        " *\n"
        " *    You should have received a copy of the GNU Affero General Public License\n"
        " *    along with this program. If not, see <http://www.gnu.org/license/>.\n"
        " */\n";
    fout<<comment<<endl;
    comment = "\n// This Header File is automatically generated, you MUST NOT modify this file anyway!\n"
              "// On the contrary, you can modify the xml file \"sequoiadb/misc/rcgen/rclist.xml\" if necessary!\n";
    fout<<comment<<endl;

    fout<<"#include \"ossErr.h\""<<endl<<endl;
    fout<<"const CHAR* getErrDesp ( INT32 errCode )"<<endl
        <<"{"<<endl
        <<"   INT32 code = -errCode;"<<endl
        <<"   const static CHAR* errDesp[] ="<<endl
        <<"   {"<<endl
        <<"      \"Succeed\","<<endl;

    int size = (int)errcodes.size() - 1;
    for (int i = 0; i < size; ++i)
    {
        fout<<"      "
            <<"\""<<errcodes[i].getDesc(language)<<"\""
            <<","<<endl;
    }
    fout<<"      "
        <<"\""<<errcodes[size].getDesc(language)<<"\""<<endl
        <<"   };"<<endl
        <<"   if ( code < 0 || (UINT32)code >= (sizeof ( errDesp ) / "
        <<"sizeof ( CHAR* )) )"<<endl
        <<"   {"<<endl
        <<"      return \"unknown error\";"<<endl
        <<"   }"<<endl
        <<"   return errDesp[code];"<<endl
        <<"}"<<endl;

    fout.close();
}

void RCGen::genCS ()
{
    ofstream fout(CSPATH);
    if ( fout == NULL )
    {
        cout<<"can not open file: "<<CSPATH<<endl;
        exit(0);
    }

    fout<<std::left
        <<"namespace SequoiaDB"<<endl
        <<"{"<<endl
        <<"    class Errors"<<endl
        <<"    {"<<endl
        <<"        public enum errors : int"<<endl
        <<"        {"<<endl;

    int size = (int)errcodes.size() - 1;
    for (int i = 0; i < size; ++i)
    {
        fout<<"            "
            <<setw(maxErrorNameWidth + 2)<<errcodes[i].name
            <<" = "
            <<errcodes[i].value
            <<","<<endl;
    }
    fout<<"            "
        <<setw(maxErrorNameWidth + 2)<<errcodes[size].name
        <<" = "
        <<errcodes[size].value
        <<endl;

    fout<<"        };"<<endl
        <<endl
        <<"        public static readonly string[] descriptions = {"<<endl;

    for (int i = 0; i < size; ++i)
    {
        fout<<"            "
            <<"\""<<errcodes[i].getDesc(language)<<"\""
            <<","<<endl;
    }
    fout<<"            "
        <<"\""<<errcodes[size].getDesc(language)<<"\""<<endl;

    fout<<"        };"<<endl
        <<"    }"<<endl
        <<"}";

    fout.close();
}

void RCGen::genJava()
{
    ofstream fout(JAVAPATH);

    string license =
        "/*    Copyright (C) 2011-2017 SequoiaDB Inc.\n"
        " *\n"
        " *    Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        " *    you may not use this file except in compliance with the License.\n"
        " *    You may obtain a copy of the License at\n"
        " *\n"
        " *    http://www.apache.org/licenses/LICENSE-2.0\n"
        " *\n"
        " *    Unless required by applicable law or agreed to in writing, software\n"
        " *    distributed under the License is distributed on an \"AS IS\" BASIS,\n"
        " *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
        " *    See the License for the specific language governing permissions and\n"
        " *    limitations under the License.\n"
        " */\n";
    fout << license << endl;

    string comment =
        "// This Header File is automatically generated, you MUST NOT modify this file anyway!\n"
        "// On the contrary, you can modify the xml file \"sequoiadb/misc/autogen/rclist.xml\" if necessary!\n";
    fout << comment << endl;

    fout << "package com.sequoiadb.exception;" << endl;
    fout << endl;
    fout << "public enum SDBError {" << endl;

    fout << left;

    for ( int i = 0; i < errcodes.size(); i++ )
    {
        const ErrorCode& errcode = errcodes[i];
        fout << "    "
             << left << setfill(' ') << setw(maxErrorNameWidth + 4) << errcode.name + "("
             << right << setfill(' ') << setw(5) << errcode.value << ",    "
             << "\"" << errcode.getDesc(language) << "\"" << "    )";
        if (i < errcodes.size() - 1)
        {
            fout << ",";
        }
        else
        {
            fout << ";";
        }
        fout << endl;
    }

    fout << endl;

    string code =
      "    private int code;\n"
      "    private String desc;\n"
      "\n"
      "    private SDBError(int code, String desc) {\n"
      "        this.code = code;\n"
      "        this.desc = desc;\n"
      "    }\n"
      "\n"
      "    @Override\n"
      "    public String toString() {\n"
      "          return this.name() + \"(\" + this.code + \")\" + \": \" + this.desc;\n"
      "    }\n"
      "\n"
      "    public int getErrorCode() {\n"
      "          return this.code;\n"
      "    }\n"
      "\n"
      "    public String getErrorDescription() {\n"
      "          return this.desc;\n"
      "    }\n"
      "\n"
      "    public String getErrorType() {\n"
      "          return this.name();\n"
      "    }";

    fout << code << endl;
    fout << endl;
    fout << "    public static SDBError getSDBError(int errorCode) {" << endl;
    fout << "        switch(errorCode) {" << endl;
    for ( int i = 0; i < errcodes.size(); i++ )
    {
        const ErrorCode& errcode = errcodes[i];
        fout << "        case "
             << right << setfill(' ') << setw(5) << errcode.value
             << ": " << "return " << errcode.name
             << ";" << endl;
    }
    fout << "        default:    return null;" << endl;
    fout << "        }" << endl;
    fout << "    }" << endl;

    fout << "}" << endl;
}

void RCGen::genPython ()
{
    ofstream fout(PYTHONPATH);
   
    string license =
        "#   Copyright (C) 2011-2017 SequoiaDB Inc.\n"
        "#\n"
        "#   Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        "#   you may not use this file except in compliance with the License.\n"
        "#   You may obtain a copy of the License at\n"
        "#\n"
        "#   http://www.apache.org/licenses/LICENSE-2.0\n"
        "#\n"
        "#   Unless required by applicable law or agreed to in writing, software\n"
        "#   distributed under the License is distributed on an \"AS IS\" BASIS,\n"
        "#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
        "#   See the License for the specific language governing permissions and\n"
        "#   limitations under the License.\n"
        "#\n";
    fout << license << endl;

    string comment =
        "# This Header File is automatically generated, you MUST NOT modify this file anyway!\n"
        "# On the contrary, you can modify the xml file \"sequoiadb/misc/autogen/rclist.xml\" if necessary!\n";
    fout << comment << endl << endl;

    string errcodeDef =
        "class Errcode(object):\n"
        "    \"\"\"Errcode of SequoiaDB.\n"
        "    \"\"\"\n"
        "\n"
        "    def __init__(self, name, code, desc):\n"
        "        self.__name = name\n"
        "        self.__code = code\n"
        "        self.__desc = desc\n"
        "\n"
        "    @property\n"
        "    def name(self):\n"
        "        \"\"\"Name of this error code.\n"
        "        \"\"\"\n"
        "        return self.__name\n"
        "\n"
        "    @property\n"
        "    def code(self):\n"
        "        \"\"\"Code of this error code.\n"
        "        \"\"\"\n"
        "        return self.__code\n"
        "\n"
        "    @property\n"
        "    def desc(self):\n"
        "        \"\"\"Description of this error code.\n"
        "        \"\"\"\n"
        "        return self.__desc\n"
        "\n"
        "    def __eq__(self, other):\n"
        "        \"\"\"Errcode can equals to Errcode and int.\n"
        "        \"\"\"\n"
        "        if isinstance(other, Errcode):\n"
        "            return self.code == other.code\n"
        "        elif isinstance(other, int):\n"
        "            return self.code == other\n"
        "        else:\n"
        "            return False\n"
        "\n"
        "    def __ne__(self, other):\n"
        "        \"\"\"Errcode can not equals to Errcode and int.\n"
        "        \"\"\"\n"
        "        return not self.__eq__(other)\n"
        "\n"
        "    def __repr__(self):\n"
        "        return \"Errcode('%s', %d, '%s')\" % (self.name, self.code, self.desc)\n"
        "\n"
        "    def __str__(self):\n"
        "        return \"Errcode('%s', %d, '%s')\" % (self.name, self.code, self.desc)\n"
        "\n";
    fout << errcodeDef << endl;

    fout << "SDB_OK = Errcode(\"SDB_OK\", 0, \"OK\")" << endl;

    for ( int i = 0; i < errcodes.size(); i++ )
    {
        const ErrorCode& errcode = errcodes[i];
        fout << errcode.name << " = Errcode("
             << "\"" << errcode.name << "\", "
             << errcode.value << ", "
             << "\"" << errcode.getDesc(language) << "\"" << ")"
             << endl;
    }

    fout << endl;
    fout << "_errcode_map = {\n";
    for ( int i = 0; i < errcodes.size(); i++ )
    {
        const ErrorCode& errcode = errcodes[i];
        fout << "    "
             << errcode.value << ": " << errcode.name;
        if (i < errcodes.size() - 1)
        {
            fout << ",\n";
        }
        else
        {
            fout << "\n";
        }
    }
    fout << "}" << endl;

    fout << endl << endl;
    fout << "def get_errcode(code):\n"
         << "    return _errcode_map.get(code)"
         << endl;

    fout.close();
}

void RCGen::genWeb ()
{
   string docpath = string ( WEBPATH ) + string ( language ) + string ( WEBPATHSUFFIX ) ;
   ofstream fout ( docpath.c_str() ) ;
   if ( fout == NULL )
   {
      cout << "can't open file: " << docpath << endl ;
      exit ( -1 ) ;
   }

   fout << std::left ;
   fout << "<?php" << endl ;
   fout << "$errno_" << language << " = array(" << endl ;
   for ( int i = 0; i < errcodes.size(); ++i )
   {
      string first = errcodes[i].name ;
      string second = errcodes[i].getDesc(language);
      first = replace_all ( first, "$", "\\$" ) ;
      second = replace_all ( second, "$", "\\$" ) ;
      fout << setw(6) << errcodes[i].value << " => \"" << first << ": " << second << "\"" ;
      if ( i < errcodes.size()-1 )
      {
         fout << "," ;
      }
      fout << endl ;
   }
   fout << ") ;" << endl ;
   fout << "?>" << endl ;
   fout.close () ;
}

void RCGen::genJS ()
{
   ofstream fout ( JSPATH ) ;
   if ( fout == NULL )
   {
      cout << "can't open file: " << JSPATH << endl ;
      exit (-1) ;
   }

   fout << std::left ;

   fout << "/* Error Constants */" << endl ;
   for ( int i = 0 ; i < conslist.size() ; i++ )
   {
      fout << "const " << setw(maxErrorNameWidth + 2) << conslist[i].first << " = "
         << setw(6) << conslist[i].second << ";" << endl ;
   }
   fout << endl ;

   fout << "/* Error Codes */" << endl ;
   for ( int i = 0 ; i < errcodes.size() ; i++ )
   {
      fout << "const " << setw(maxErrorNameWidth + 2) << errcodes[i].name << " = "
         << setw(6) << -(i + 1) << "; // "
         << errcodes[i].getDesc(language) << ";" << endl ;
   }
   fout << endl ;

   fout << "function _getErr (errCode) {" << endl ;
   fout << "   var errDesp = [ " << endl ;
   fout << "      \"Succeed\"," << endl ;
   for ( int i = 0 ; i < errcodes.size() ; i++ )
   {
      fout << "      \"" << errcodes[i].getDesc(language)
         << ((i == errcodes.size() - 1) ? "\"" : "\",") << endl ;
   }
   fout << "   ]; " << endl ;
   fout << "   var index = -errCode ;" << endl ;
   fout << "   if ( index < 0 || index >= errDesp.length ) {" << endl ;
   fout << "      return \"unknown error\"" << endl ;
   fout << "   }" << endl ;
   fout << "   return errDesp[index] ;" << endl ;
   fout << "}" << endl ;
   fout << "function getErr (errCode) {" << endl ;
   fout << "   return _getErr ( errCode ) ;" << endl ;
   fout << "}" << endl ;

   fout.close() ;
}

void RCGen::genDoc()
{
   string docpath = string ( RC_MDPATH ) ;
   ofstream fout ( docpath.c_str() ) ;

   fout << std::left ;

   fout << "| Name | Error Code | Description |" << endl ;
   fout << "| --- | --- | --- |" << endl ;

   std::vector<ErrorCode>::const_iterator it;
   for ( it = errcodes.begin(); it != errcodes.end(); it++ )
   {
      const ErrorCode& errcode = *it;
      if (errcode.reserved)
      {
         continue;
      }

      fout << "| " << errcode.name
           << " | " << errcode.value
           << " | " << errcode.getDesc(language)
           << " |" << endl ;
   }
}

