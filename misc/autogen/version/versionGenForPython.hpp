#ifndef VERSION_GEN_FOR_PYTHON_HPP
#define VERSION_GEN_FOR_PYTHON_HPP

#include "versionGeneratorBase.hpp"

class versionGenForPython : public versionGeneratorBase
{
DECLARE_GENERATOR_AUTO_REGISTER() ;

public:
   versionGenForPython() ;
   ~versionGenForPython() ;

   bool hasNext() ;
   int outputFile( int id, fileOutStream &fout,
                   string &outputPath ) ;
   const char* name() { return "version for python" ; }

private:
   void _buildVersion( char *version, char *release,
                       char *gitVersion, char *time ) ;

private:
   bool _versionFileDone ;    // version.py
} ;

#endif
