#ifndef OPT_GEN_FOR_CPP_HPP
#define OPT_GEN_FOR_CPP_HPP

#include "optGeneratorBase.hpp"

#define OPT_CPP_FILE_PATH  ENGINE_PATH"include/pmdOptions.hpp"

class optGenForCPP : public optGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   optGenForCPP() ;
   ~optGenForCPP() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "options for CPP" ; }

private:
   bool _isFinish ;
} ;

#endif