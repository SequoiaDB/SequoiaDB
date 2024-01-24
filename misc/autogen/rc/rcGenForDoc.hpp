#ifndef RC_GEN_FOR_DOC_HPP
#define RC_GEN_FOR_DOC_HPP

#include "rcGeneratorBase.hpp"

#define RC_MD_FILE_PATH    DOCUMENT_PATH"Manual/Sequoiadb_error_code.md"

class rcGenForDoc : public rcGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   rcGenForDoc() ;
   ~rcGenForDoc() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout, string &outputPath ) ;
   const char* name(){ return "rc for Doc" ; }

private:
   bool _isFinish ;
} ;

#endif