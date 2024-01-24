#ifndef RC_GEN_FOR_C_HPP
#define RC_GEN_FOR_C_HPP

#include "rcGeneratorBase.hpp"

#define RC_C_FILE_PATH  ENGINE_PATH"include/ossErr.h"

class rcGenForC : public rcGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForC() ;
   ~rcGenForC() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "rc for C" ; }

private:
   bool _isFinish ;
} ;

#endif