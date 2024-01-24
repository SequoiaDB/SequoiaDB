#ifndef RC_GEN_FOR_CS_HPP
#define RC_GEN_FOR_CS_HPP

#include "rcGeneratorBase.hpp"

#define RC_CS_FILE_PATH DRIVER_PATH"C#.Net/Driver/exception/Errors.cs"

class rcGenForCS : public rcGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForCS() ;
   ~rcGenForCS() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "rc for C#" ; }

private:
   bool _isFinish ;
} ;

#endif