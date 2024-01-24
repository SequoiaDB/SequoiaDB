#ifndef VERSION_GEN_FOR_DOC_HPP
#define VERSION_GEN_FOR_DOC_HPP

#include "versionGeneratorBase.hpp"

#define VERSION_DOC_PATH   DOC_PATH"config/version.json"

class versionGenForDoc : public versionGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   versionGenForDoc() ;
   ~versionGenForDoc() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout,
                   string &outputPath ) ;
   const char* name() { return "version for doc" ; }

private:
   bool _isFinish ;
} ;

#endif
