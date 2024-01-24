#ifndef VERSION_GEN_FOR_CRONTASK_HPP
#define VERSION_GEN_FOR_CRONTASK_HPP

#include "versionGeneratorBase.hpp"

class versionGenForTools : public versionGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   versionGenForTools() ;
   ~versionGenForTools() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout,
                   string &outputPath ) ;
   const char* name() { return "version for cron-task" ; }

private:
   void _buildVersion( char *version, char *release,
                       char *gitVersion, char *time ) ;

private:
   vector<string> _fileList ;
   int _i ;
} ;

#endif
