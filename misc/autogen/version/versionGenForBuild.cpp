#include "versionGenForBuild.hpp"
#include "../ver_conf.h"
#include <time.h>

#define TIME_BUFFER_SIZE 64

#if defined (GENERAL_VER_BUILD_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( versionGenForBuild, GENERAL_VER_BUILD_FILE ) ;
#endif

versionGenForBuild::versionGenForBuild() : _isFinish( false )
{
}

versionGenForBuild::~versionGenForBuild()
{
}

bool versionGenForBuild::hasNext()
{
   return !_isFinish ;
}

int versionGenForBuild::outputFile( int id, fileOutStream &fout,
                                    string &outputPath )
{
   int rc = 0 ;
   size_t pos = -1 ;
   string content ;
   struct tm *otm ;
   struct timeval tv ;
   struct timezone tz ;
   time_t tt ;
   char timeInfo[TIME_BUFFER_SIZE] = {0} ;

   _isFinish = true ;

   outputPath = utilGetRealPath2( VERSION_BUILD_FILE_PATH ) ;

   rc = utilGetFileContent( VERSION_BUILD_LOCAL_PATH, content ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "Failed to get file content: path = "
                           << outputPath << endl ;
      goto error ;
   }

   gettimeofday(&tv, &tz);
   tt = tv.tv_sec ;

   otm = localtime ( &tt ) ;
   utilSnprintf( timeInfo, TIME_BUFFER_SIZE, SDB_ENGINE_BUILD_FORMAT,
                 otm->tm_year + 1900,
                 otm->tm_mon + 1,
                 otm->tm_mday,
                 otm->tm_hour,
                 otm->tm_min,
                 otm->tm_sec ) ;

   pos = content.find( REPLACE_TIMESTAMP_STRING, 0 ) ;
   if ( pos == string::npos )
   {
      printLog( PD_WARNING ) << "Warning: Failed to locate " REPLACE_TIMESTAMP_STRING
                             << endl ;
   }

   if ( strlen ( REPLACE_TIMESTAMP_STRING ) != strlen ( timeInfo ) )
   {
      printLog( PD_WARNING ) << "Warning: Length of timestamp is not correct"
                             << ", expected: " << strlen ( REPLACE_TIMESTAMP_STRING )
                             << ", actual: "   << strlen ( timeInfo ) << endl ;
      goto done ;
   }

   fout << utilReplaceAll( content, REPLACE_TIMESTAMP_STRING, timeInfo ) ;

done:
   return rc ;
error:
   goto done ;
}