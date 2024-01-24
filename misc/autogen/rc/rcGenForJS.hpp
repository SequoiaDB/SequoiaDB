#ifndef RC_GEN_FOR_JS_HPP
#define RC_GEN_FOR_JS_HPP

#include "rcGeneratorBase.hpp"

#define RC_JS_FILE_PATH    ENGINE_PATH"spt/error.js"

class rcGenForJS : public rcGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForJS() ;
   ~rcGenForJS() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "rc for JS" ; }

private:
   bool _isFinish ;
} ;

#endif