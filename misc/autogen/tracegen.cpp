#include "core.hpp"
#include "ossUtil.h"
#include "tracegen.h"
#include <stdio.h>
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
namespace fs = boost::filesystem ;
using namespace std ;
const CHAR *traceNameSuffix[] = {
".cpp",
".c",
".h",
".hpp",
".C"
} ;

static BOOLEAN isTraceSourceFile ( const CHAR *pFile )
{
   INT32 fileNameLen = sizeof(traceNameSuffix) / sizeof(CHAR*) ;
   for ( INT32 i = 0 ; i < fileNameLen; ++i )
   {
      if ( ossStrlen(pFile) > ossStrlen(traceNameSuffix[i]) &&
           ossStrncasecmp ( &pFile[strlen(pFile)-strlen(traceNameSuffix[i])],
                            traceNameSuffix[i], strlen(traceNameSuffix[i]) ) == 0)
         return TRUE ;
   }
   return FALSE ;
}

static UINT32 seqNum = 0 ;
void TraceGen::genList ()
{
   try
   {
      ofstream fout1 ( TRACEFILENAMEPATH1 ) ;
      if ( fout1 == NULL )
      {
         cout << "can not open file: " << TRACEFILENAMEPATH1 << endl ;
         exit(0) ;
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
      fout1<<std::left<<comment<<endl;
      fout1 << "/* This list file is automatically generated," <<
               "you shoud NOT modify this file anyway! */" << endl ;
      fout1 << "#include \"core.hpp\"" << endl ;
      fout1 << "static const CHAR *_pTraceFunctionList[] = {" << endl ;

      _genList ( SOURCEPATH, NULL, fout1, -1 ) ;

      fout1 << "} ;" << endl ;
      fout1 << "const UINT32 _pTraceFunctionListNum = " <<
               seqNum << " ;" << endl ;
      fout1 << "const UINT32 pdGetTraceFunctionListNum()" << endl;
      fout1 << "{" << endl;
      fout1 << "  return " << seqNum << ";" << endl;
      fout1 << "}" << endl;
      fout1 << "const CHAR *pdGetTraceFunction ( UINT64 id )" << endl ;
      fout1 << "{" << endl ;
      fout1 << "   UINT32 funcID = (UINT32)(id & 0xFFFFFFFF) ;" << endl ;
      fout1 << "   if ( funcID >= _pTraceFunctionListNum )" << endl ;
      fout1 << "      return NULL ;" << endl ;
      fout1 << "   return _pTraceFunctionList[funcID] ;" << endl ;
      fout1 << "}" << endl ;

      fout1.close () ;
   }
   catch ( std::exception &e )
   {
      ossPrintf ( "Failed to gen list: %s"OSS_NEWLINE,
                  e.what() ) ;
   }
}

void TraceGen::_extractFromFile ( std::ofstream *fout,
                                  std::ofstream &fout1,
                                  const CHAR *pFileName,
                                  INT32 compid )
{
#if defined (_LINUX) || defined (_AIX)
   FILE *pFile = fopen ( pFileName, "r" ) ;
#elif defined (_WINDOWS)
   FILE *pFile = NULL ;
   fopen_s ( &pFile, pFileName, "r" ) ;
#endif
   CHAR lineBuffer[512] = {0} ;
   CHAR *p = NULL ;
   if ( !pFile )
      return ;
   CHAR *pAlias = NULL ;
   CHAR *pValue = NULL ;
   UINT32 one = 1 ;
   while ( ( p = fgets ( lineBuffer, sizeof(lineBuffer), pFile ) ) != NULL )
   {
      while ( *p != 0 && ( *p == ' ' || *p == '\t' || *p == '/' ) )
         ++p ;
      if ( ossStrncmp ( p, TRACEEYECATCHER, TRACEEYECATCHERLEN ) == 0 )
      {
         // it's PD_TRACE_DECLARE_FUNCTION macro
         while ( *p != 0 && *p != '(' )
            ++p ;
         ++p ;
         while ( *p != 0 && ( *p == ' ' || *p == '\t' ) )
            ++p ;
         pAlias = p ;
         while ( *p != 0 && *p != ' ' && *p != '\t' && *p != ',' )
            ++p ;
         *p = '\0' ;
         ++p ;
         while ( *p != 0 && *p != '"' )
            ++p ;
         ++p ;
         pValue = p ;
         while ( *p != 0 && *p != '"' )
            ++p ;
         *p = '\0' ;

         // PD_TRACE_DECLARE_FUNCTION ( SDB_PDLOG, "pdLog" )
         UINT32 componentID = one<<compid ;
         UINT64 uniqueID = ((UINT64)componentID)<<32 | seqNum ;
         CHAR hexBuffer[512] = {0} ;
         ossSnprintf ( hexBuffer, sizeof(hexBuffer), "#define %-50s 0x%llxL", pAlias, uniqueID ) ;
         *fout << hexBuffer << endl ;
         if ( 0 != seqNum )
         {
            fout1 << ",   " ;
         }
         else
         {
            fout1 << "    " ;
         }
         fout1 << "\"" << pValue << "\"" << endl ;
         seqNum++ ;
      }
   }
   fclose ( pFile ) ;
}

void TraceGen::_genList ( const CHAR *pPath,
                          std::ofstream *fout,
                          std::ofstream &fout1,
                          INT32 compid )
{
   const CHAR *pathSep = OSS_FILE_SEP ;
   fs::path directory ( pPath ) ;
   fs::directory_iterator end_iter ;
   if ( fs::exists(directory) && fs::is_directory(directory) )
   {
      for ( fs::directory_iterator dir_iter ( directory );
            dir_iter != end_iter; ++dir_iter )
      {
         if ( fs::is_regular_file ( dir_iter->status() ) )
         {
            if ( -1 == compid )
               continue ;
            const std::string filePath = dir_iter->path().string() ;
            const std::string fileName = dir_iter->path().filename().string() ;
            const CHAR *pFileName = fileName.c_str() ;
            if ( isTraceSourceFile ( pFileName ) )
            {
               _extractFromFile ( fout, fout1,
                                  filePath.c_str(), compid ) ;
            }
         }
         else if ( fs::is_directory ( dir_iter->status() ) )
         {
            const std::string dirName = dir_iter->path().filename().string() ;
            if ( -1 == compid )
            {
               const CHAR *pathSep = OSS_FILE_SEP ;
               const CHAR *pFileName = ossStrrchr ( dirName.c_str(),
                                                    pathSep[0] ) ;
               if ( !pFileName )
                  pFileName = dirName.c_str() ;
               else
                  pFileName++ ;
               for ( INT32 i = 0; i < _pdTraceComponentNum; i++ )
               {
                  if ( ossStrcmp ( pFileName,
                                   pdGetTraceComponent(i) ) == 0 )
                  {
                     if ( ossStrncmp ( dir_iter->path().filename().string().c_str(),
                                       SKIPPATH, ossStrlen(SKIPPATH)) != 0 )
                     {
                        CHAR buffer [ OSS_MAX_PATHSIZE ] = {0} ;
                        ossSnprintf ( buffer, OSS_MAX_PATHSIZE, "%s%s%s",
                                      TRACEINCLUDEPATH,
                                      pdGetTraceComponent(i),
                                      TRACEINCLUDESUFFIX ) ;
                        ofstream foutNew ( buffer ) ;
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
                          " */\n" ;
                        foutNew << std::left<<comment << endl ;
                        foutNew << "/* This list file is automatically generated," <<
                                   "you shoud NOT modify this file anyway! test comment*/" << endl ;
                        foutNew << "#ifndef " << pdGetTraceComponent(i) <<
                                   "TRACE_H__" << endl ;
                        foutNew << "#define " << pdGetTraceComponent(i) <<
                                   "TRACE_H__" << endl ;
                        foutNew << "// Component: " << pdGetTraceComponent(i)
                                << endl ;
                        _genList ( dir_iter->path().string().c_str(),
                                   &foutNew, fout1, i ) ;
                        foutNew << "#endif" << endl ;
                        foutNew.close () ;
                     }
                     break ;
                  } // if ( ossStrcmp ( dirName.c_str(),
               } // for ( INT32 i = 0; i < _pdTraceComponentNum; i++ )
            } // if ( -1 == compid )
            else
            {
               if ( ossStrncmp ( dir_iter->path().filename().string().c_str(),
                                 SKIPPATH, ossStrlen(SKIPPATH)) != 0 )
                  _genList ( dir_iter->path().string().c_str(),
                             fout, fout1, compid ) ;
            }
         } // else if ( fs::is_directory ( dir_iter->status() ) )
      }
   }
}

