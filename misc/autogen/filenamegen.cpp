#include "core.hpp"
#include "ossUtil.h"
#include "filenamegen.h"
#include <stdio.h>
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
namespace fs = boost::filesystem ;
using namespace std ;
const CHAR *fileNameSuffix[] = {
".cpp",
".c",
".h",
".hpp",
".C"
} ;

static BOOLEAN isSourceFile ( const CHAR *pFile )
{
   INT32 fileNameLen = sizeof(fileNameSuffix) / sizeof(CHAR*) ;
   for ( INT32 i = 0 ; i < fileNameLen; ++i )
   {
      if ( ossStrlen(pFile) > ossStrlen(fileNameSuffix[i]) &&
           ossStrncasecmp ( &pFile[strlen(pFile)-strlen(fileNameSuffix[i])],
                            fileNameSuffix[i], strlen(fileNameSuffix[i]) ) == 0)
         return TRUE ;
   }
   return FALSE ;
}

void FileNameGen::genList ()
{
   try
   {
      ofstream fout ( FILENAMEPATH ) ;
      if ( fout == NULL )
      {
         cout << "can not open file: " << FILENAMEPATH << endl ;
         exit(0) ;
      }
      
      ofstream codeout ( FILENAME_CODE_PATH );

      string comment =
         "/* This list file is automatically generated, you MUST NOT \
modify this file anyway! */" ;

      fout << comment << endl ;
      codeout << comment << endl ;

      string codeHeader = "\
#pragma once\n\
\n\
#include \"oss.hpp\"\n\
#include <map> \n\
#include <string> \n\
\n\
using namespace std ; \n\
\n\
const static pair<UINT32, string> filenamesArray[] =\n\
{\n\
" ;
      codeout << codeHeader ;

      _genList ( SOURCEPATH, fout, codeout ) ;

      string codeTail = "\n\
} ;\n\
\n\
const UINT32 filenameSize = sizeof(filenamesArray) / sizeof(filenamesArray[0]) ;\n\
\n\
const static map<UINT32, string> filenamesMap( filenamesArray, filenamesArray + filenameSize ) ;\n\
" ;

      codeout << codeTail << endl ;

      fout.close() ;
      codeout.close() ;
   }
   catch ( std::exception &e )
   {
      ossPrintf ( "Failed to gen list: %s"OSS_NEWLINE,
                  e.what() ) ;
   }
}

static BOOLEAN isFirstFile = TRUE ;

void FileNameGen::_genList ( const CHAR *pPath, std::ofstream &fout, std::ofstream &codeout )
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
            const std::string fileName = dir_iter->path().filename().string() ;
            const CHAR *pFileName = fileName.c_str() ;
            if ( isSourceFile ( pFileName ) )
            {
               UINT32 hashCode = ossHashFileName ( pFileName ) ;
               fout << hashCode << " : " << pFileName << endl ;
               if ( !isFirstFile )
               {
                  codeout << "," << endl ;
               }
               else
               {
                   isFirstFile = FALSE ;
               }
               codeout << "   make_pair(" << hashCode << ", \"" << fileName << "\")" ;
            }
         }
         else if ( fs::is_directory ( dir_iter->status() ) )
         {
            if ( ossStrncmp ( dir_iter->path().filename().string().c_str(),
                              SKIPPATH, ossStrlen(SKIPPATH)) != 0 )
               _genList ( dir_iter->path().string().c_str(),
                          fout, codeout ) ;
         }
      }
   }
}

