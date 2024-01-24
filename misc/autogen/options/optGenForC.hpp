#ifndef OPT_GEN_FOR_C_HPP
#define OPT_GEN_FOR_C_HPP

#include "optGeneratorBase.hpp"

#define OPT_C_FILE_PATH ENGINE_PATH"include/pmdOptions.h"

class optGenForC : public optGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   optGenForC() ;
   ~optGenForC() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "options for C" ; }

private:
   bool _isFinish ;
} ;

#endif