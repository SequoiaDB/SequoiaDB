#ifndef RC_GEN_FOR_WEB_HPP
#define RC_GEN_FOR_WEB_HPP

#include "rcGeneratorBase.hpp"

#define RC_WEB_FILE_PATH  CLIENT_PATH"admin/admintpl/error_"

class rcGenForWEB : public rcGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForWEB() ;
   ~rcGenForWEB() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "rc for WEB" ; }

private:
   bool _isFinish ;
} ;

#endif