#include "fileListGenerator.hpp"
#include "../util.h"

static const char *_suffix[] = {
   FILENAME_SUFFIX_LIST
} ;

static const char *_filterDir[] = {
   FILENAME_FILTER_DIR
} ;

#define SUFFIX_SIZE (sizeof( _suffix ) / sizeof( const char * ))
#define FILTER_SIZE (sizeof( _filterDir ) / sizeof( const char * ))

#if defined (GENERAL_FILE_LIST_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( fileListGenerator, GENERAL_FILE_LIST_FILE ) ;
#endif

fileListGenerator::fileListGenerator() : _isFinish( false )
{
}

fileListGenerator::~fileListGenerator()
{
}

int fileListGenerator::init()
{
   string enginePath = utilGetRealPath2( ENGINE_PATH ) ;

   utilGetFileNameListBySuffix( enginePath.c_str(),
                                _suffix, SUFFIX_SIZE,
                                _filterDir, FILTER_SIZE,
                                _fileNameList ) ;

   return 0 ;
}

bool fileListGenerator::hasNext()
{
   return !_isFinish ;
}

int fileListGenerator::outputFile( int id, fileOutStream &fout,
                                   string &outputPath )
{
   int rc = 0 ;

   if ( id == 0 )
   {
      rc = _genFileNameLst( fout, outputPath ) ;
   }
   else
   {
      rc = _genFileNameHPP( fout, outputPath ) ;
      _isFinish = true ;
   }

   return rc ;
}

int fileListGenerator::_genFileNameLst( fileOutStream &fout,
                                        string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int size = (int)_fileNameList.size() ;

   outputPath = FILENAMES_LST_PATH ;

   fout << FILENAME_LST_STATEMENT"\n" ;

   for( i = 0; i < size; ++i )
   {
      string fileName = _fileNameList.at( i ) ;
      unsigned int hashCode = utilHashFileName( fileName.c_str() ) ;

      fout << hashCode << " : " << fileName << "\n" ;
   }

   return rc ;
}

int fileListGenerator::_genFileNameHPP( fileOutStream &fout,
                                        string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int size = (int)_fileNameList.size() ;

   outputPath = FILENAMES_HPP_PATH ;

   fout << FILENAME_LST_STATEMENT"\n"
           "#pragma once\n"
           "\n"
           "#include \"oss.hpp\"\n"
           "#include <map> \n"
           "#include <string> \n"
           "\n"
           "using namespace std ; \n"
           "\n"
           "const static pair<UINT32, string> autoFilenamesArray[] =\n"
           "{\n" ;

   for( i = 0; i < size; ++i )
   {
      string fileName = _fileNameList.at( i ) ;
      unsigned int hashCode = utilHashFileName( fileName.c_str() ) ;

      if ( i > 0 )
      {
         fout << ",\n" ;
      }

      fout << "   make_pair(" << hashCode << ", \"" << fileName << "\")" ;
   }

   fout << "\n"
           "} ;\n"
           "\n"
           "const UINT32 autoFilenameSize = sizeof(autoFilenamesArray) /\n"
           "                                sizeof(autoFilenamesArray[0]) ;\n"
           "\n"
           "const static map<UINT32, string> autoFilenamesMap( autoFilenamesArray, \n"
           "                                                   autoFilenamesArray +\n"
           "                                                   autoFilenameSize ) ;\n"
           "\n"
           "inline string autoGetFileName( UINT32 fileCode )\n"
           "{\n"
           "   map<UINT32, string>::const_iterator cit ;\n"
           "   cit = autoFilenamesMap.find( fileCode ) ;\n"
           "   if ( cit != autoFilenamesMap.end() )\n"
           "   {\n"
           "      return cit->second ;\n"
           "   }\n"
           "   else\n"
           "   {\n"
           "      return \"\" ;\n"
           "   }\n"
           "}\n"
           "\n";

   return rc ;
}

