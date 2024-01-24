#include "versionGenForPython.hpp"
#include "../ver_conf.h"
#include "ossVer.h"
#include <time.h>

#define VERSION_BUFFER_SIZE 64

#define REPLACE_VERSION     "DB_VERSION="
#define REPLACE_RELEASE     "RELEASE="
#define REPLACE_GIT_VER     "GIT_VERSION="
#define REPLACE_BUILD_TIME  "BUILD_TIME="

#define VERSION_FILE_PATH   "version.py"

#if defined (GENERAL_VER_PYTHON_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( versionGenForPython, GENERAL_VER_PYTHON_FILE ) ;
#endif

versionGenForPython::versionGenForPython()
: _versionFileDone( false )
{
}

versionGenForPython::~versionGenForPython()
{
}

bool versionGenForPython::hasNext()
{
   if ( _versionFileDone )
   {
      return false ;
   }
   return true ;
}

int versionGenForPython::outputFile( int id, fileOutStream &fout,
                                     string &outputPath )
{
   int rc = 0 ;
   string content, newContent ;
   vector<string> lines ;
   char version[ VERSION_BUFFER_SIZE ] = { 0 } ;
   char release[ VERSION_BUFFER_SIZE ] = { 0 } ;
   char gitVersion[ VERSION_BUFFER_SIZE ] = { 0 } ;
   char time[ VERSION_BUFFER_SIZE ] = { 0 } ;
   const char* fileName = NULL ;

   // we need to process file: version.py
   if ( !_versionFileDone )
   {
      fileName = VERSION_FILE_PATH ;
      _versionFileDone = true ;
   }

   outputPath = utilGetRealPath2( fileName ) ;

   // read content from file
   rc = utilGetFileContent( fileName, content ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "Failed to get file content: path = "
                           << outputPath << endl ;
      goto error ;
   }

   // build version info
   _buildVersion( version, release, gitVersion, time ) ;

   // replace version info
   lines = stringSplit( content, "\n" ) ;
   for( int i = 0; i < lines.size() ; ++i )
   {
      const char* oneLine = lines.at( i ).c_str() ;
      if ( 0 == strncmp( oneLine,
                         REPLACE_VERSION,
                         strlen( REPLACE_VERSION ) ) )
      {
         newContent += version ;
         newContent += "\n" ;
      }
      else if ( 0 == strncmp( oneLine,
                              REPLACE_RELEASE,
                              strlen( REPLACE_RELEASE ) ) )
      {
         newContent += release ;
         newContent += "\n" ;
      }
      else if ( 0 == strncmp( oneLine,
                              REPLACE_GIT_VER,
                              strlen( REPLACE_GIT_VER ) ) )
      {
         newContent += gitVersion ;
         newContent += "\n" ;
      }
      else if ( 0 == strncmp( oneLine,
                              REPLACE_BUILD_TIME,
                              strlen( REPLACE_BUILD_TIME ) ) )
      {
         newContent += time ;
         newContent += "\n" ;
      }
      else
      {
         newContent += oneLine ;
         newContent += "\n" ;
      }
   }

   fout << newContent ;

done:
   return rc ;
error:
   goto done ;
}

void versionGenForPython::_buildVersion( char *version, char *release,
                                         char *gitVersion, char *time )
{
   // VERSION="3.4.1"
#ifdef SDB_ENGINE_FIXVERSION_CURRENT
   utilSnprintf( version, VERSION_BUFFER_SIZE - 1, "%s\"%d.%d.%d\"",
                 REPLACE_VERSION,
                 SDB_ENGINE_VERISON_CURRENT,
                 SDB_ENGINE_SUBVERSION_CURRENT,
                 SDB_ENGINE_FIXVERSION_CURRENT ) ;
#else
   utilSnprintf( version, VERSION_BUFFER_SIZE - 1, "%s\"%d.%d\"",
                 REPLACE_VERSION,
                 SDB_ENGINE_VERISON_CURRENT,
                 SDB_ENGINE_SUBVERSION_CURRENT ) ;
#endif

   // RELEASE="32581"
   utilSnprintf( release, VERSION_BUFFER_SIZE - 1, "%s\"%d\"",
                 REPLACE_RELEASE,
                 SDB_ENGINE_RELEASE_CURRENT ) ;

   // GIT_VERSION="f1aa74c8c48c74f3afd1f97015460bc38b15cee4"
   utilSnprintf( gitVersion, VERSION_BUFFER_SIZE - 1, "%s\"%s\"",
                 REPLACE_GIT_VER,
                 SDB_ENGINE_GIT_VERSION ) ;

   // BUILD_TIME="2021-01-19-11.08.05"
   struct tm *otm ;
   struct timeval tv ;
   struct timezone tz ;
   time_t tt ;

   gettimeofday(&tv, &tz);
   tt = tv.tv_sec ;
   otm = localtime ( &tt ) ;

   utilSnprintf( time, VERSION_BUFFER_SIZE - 1,
                 "%s\"" SDB_ENGINE_BUILD_FORMAT "\"",
                 REPLACE_BUILD_TIME,
                 otm->tm_year + 1900,
                 otm->tm_mon + 1,
                 otm->tm_mday,
                 otm->tm_hour,
                 otm->tm_min,
                 otm->tm_sec ) ;
}
