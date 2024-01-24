#ifndef FILELIST_GENERATOR_HPP
#define FILELIST_GENERATOR_HPP

#include "../generateInterface.hpp"

/* =============== statement =============== */
#define FILENAME_LST_STATEMENT "/* This list file is automatically generated, you MUST NOT modify this file anyway! */"

#define FILENAME_SUFFIX_LIST \
   "hpp",   \
   "cpp",   \
   "h",     \
   "c",     \

#define FILENAME_FILTER_DIR   ".svn"

#define FILENAMES_LST_PATH    MISC_PATH"filenames.lst"
#define FILENAMES_HPP_PATH    ENGINE_PATH"include/filenames.hpp"

class fileListGenerator : public generateBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   fileListGenerator() ;
   ~fileListGenerator() ;
   int init() ;
   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout,
                   string &outputPath ) ;
   const char* name() { return "file list" ; }

private:
   int _genFileNameLst( fileOutStream &fout, string &outputPath ) ;
   int _genFileNameHPP( fileOutStream &fout, string &outputPath ) ;

private:
   bool _isFinish ;
   vector<string> _fileNameList ;
} ;

#endif
