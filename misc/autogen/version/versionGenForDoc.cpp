#include "versionGenForDoc.hpp"
#include "ossVer.h"

#define DOC_FIELD_MAJOR    "major"
#define DOC_FIELD_MINOR    "minor"

#if defined (GENERAL_VER_DOC_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( versionGenForDoc, GENERAL_VER_DOC_FILE ) ;
#endif

versionGenForDoc::versionGenForDoc() : _isFinish( false )
{
}

versionGenForDoc::~versionGenForDoc()
{
}

bool versionGenForDoc::hasNext()
{
   return !_isFinish ;
}

int versionGenForDoc::outputFile( int id, fileOutStream &fout,
                                  string &outputPath )
{
   _isFinish = true ;

   outputPath = VERSION_DOC_PATH ;

   fout << "{"
        << endl
        << "    \"" << DOC_FIELD_MAJOR << "\": "
        << SDB_ENGINE_VERISON_CURRENT << ","
        << endl
        << "    \"" << DOC_FIELD_MINOR << "\": "
        << SDB_ENGINE_SUBVERSION_CURRENT
        << endl
        << "}"
        << endl ;

   return 0 ;
}