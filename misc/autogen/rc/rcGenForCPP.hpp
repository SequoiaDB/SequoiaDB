#ifndef RC_GEN_FOR_CPP_HPP
#define RC_GEN_FOR_CPP_HPP

#include "rcGeneratorBase.hpp"

#define RC_CPP_FILE_PATH   ENGINE_PATH"oss/ossErr.cpp"

class rcGenForCPP : public rcGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForCPP() ;
   ~rcGenForCPP() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "rc for CPP" ; }

private:
   bool _isFinish ;
} ;

#endif