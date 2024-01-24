#ifndef RC_GEN_FOR_JAVA_HPP
#define RC_GEN_FOR_JAVA_HPP

#include "rcGeneratorBase.hpp"

#define RC_JAVA_FILE_PATH  DRIVER_PATH"java/src/main/java/com/sequoiadb/exception/SDBError.java"

class rcGenForJAVA : public rcGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForJAVA() ;
   ~rcGenForJAVA() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "rc for JAVA" ; }

private:
   bool _isFinish ;
} ;

#endif