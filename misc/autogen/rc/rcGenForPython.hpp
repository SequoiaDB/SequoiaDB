#ifndef RC_GEN_FOR_PYTHON_HPP
#define RC_GEN_FOR_PYTHON_HPP

#include "rcGeneratorBase.hpp"

#define RC_PYTHON_FILE_PATH  DRIVER_PATH"python/pysequoiadb/errcode.py"

class rcGenForPython : public rcGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForPython() ;
   ~rcGenForPython() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "rc for Python" ; }

private:
   bool _isFinish ;
} ;

#endif