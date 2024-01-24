#ifndef VERSION_GEN_FOR_BUILD_HPP
#define VERSION_GEN_FOR_BUILD_HPP

#include "versionGeneratorBase.hpp"

#define VERSION_BUILD_LOCAL_PATH "./ver_conf.h"
#define VERSION_BUILD_FILE_PATH  ENGINE_PATH"include/ossVer_Autogen.h"

#define REPLACE_TIMESTAMP_STRING "0000-00-00-00.00.00"

class versionGenForBuild : public versionGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   versionGenForBuild() ;
   ~versionGenForBuild() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout,
                   string &outputPath ) ;
   const char* name() { return "version for build" ; }

private:
   bool _isFinish ;
} ;

#endif
