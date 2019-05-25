#ifndef FILENAMEGEN_H
#define FILENAMEGEN_H

#include <string>
#include "core.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

#define FILENAMEPATH "../filenames.lst"
#define SOURCEPATH "../../SequoiaDB/engine/"
#define FILENAME_CODE_PATH SOURCEPATH "include/filenames.hpp"
#define SKIPPATH ".svn"
class FileNameGen
{
public :
   static void genList ();
private :
   static void _genList ( const char *pPath, std::ofstream &fout, std::ofstream &codeout ) ;
} ;
#endif
