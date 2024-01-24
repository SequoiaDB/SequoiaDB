#include "versionGenForTools.hpp"
#include "../ver_conf.h"
#include "ossVer.h"
#include <time.h>
#include <fstream>

#define VERSION_BUFFER_SIZE 64

#define REPLACE_VERSION     "DB_VERSION="
#define REPLACE_RELEASE     "RELEASE="
#define REPLACE_GIT_VER     "GIT_VERSION="
#define REPLACE_BUILD_TIME  "BUILD_TIME="

#define CRONTASK_PATH       TOOLS_PATH"crontask/"
#define TASK_CTL_FILE       CRONTASK_PATH"sdbtaskctl"
#define TASK_DAEMON_FILE    CRONTASK_PATH"sdbtaskdaemon"

#define UPGRADE_PATH        TOOLS_PATH"upgrade/"
#define UPGRADE_IDX_FILE    UPGRADE_PATH"sdbupgradeidx"

#if defined (GENERAL_VER_TOOLS_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( versionGenForTools, GENERAL_VER_TOOLS_FILE ) ;
#endif

versionGenForTools::versionGenForTools()
: _i( 0 )
{
   _fileList.push_back( TASK_CTL_FILE ) ;
   _fileList.push_back( TASK_DAEMON_FILE ) ;
   _fileList.push_back( UPGRADE_IDX_FILE ) ;
}

versionGenForTools::~versionGenForTools()
{
}

bool versionGenForTools::hasNext()
{
   return _i >= _fileList.size() ? false : true ;
}

int versionGenForTools::outputFile( int id, fileOutStream &fout,
                                    string &outputPath )
{
   int rc = 0 ;
   string content, newContent ;
   char version[ VERSION_BUFFER_SIZE ] = { 0 } ;
   char release[ VERSION_BUFFER_SIZE ] = { 0 } ;
   char gitVersion[ VERSION_BUFFER_SIZE ] = { 0 } ;
   char time[ VERSION_BUFFER_SIZE ] = { 0 } ;
   const char* fileName = NULL ;

   fileName = _fileList[ _i ].c_str() ;
   _i++ ;

   outputPath = utilGetRealPath2( fileName ) ;

   // build version info
   _buildVersion( version, release, gitVersion, time ) ;

   // read content from file, replace version info
   ifstream fin( fileName, ifstream::in ) ;
   while( fin.good() )
   {
      string oneLine ;
      getline( fin, oneLine ) ; // getline can read empty line
      if ( 0 == strncmp( oneLine.c_str(),
                         REPLACE_VERSION,
                         strlen( REPLACE_VERSION ) ) )
      {
         newContent += version ;
      }
      else if ( 0 == strncmp( oneLine.c_str(),
                              REPLACE_RELEASE,
                              strlen( REPLACE_RELEASE ) ) )
      {
         newContent += release ;
      }
      else if ( 0 == strncmp( oneLine.c_str(),
                              REPLACE_GIT_VER,
                              strlen( REPLACE_GIT_VER ) ) )
      {
         newContent += gitVersion ;
      }
      else if ( 0 == strncmp( oneLine.c_str(),
                              REPLACE_BUILD_TIME,
                              strlen( REPLACE_BUILD_TIME ) ) )
      {
         newContent += time ;
      }
      else
      {
         newContent += oneLine ;
      }
      // add newline if it is not last line
      if ( fin.good() )
      {
         newContent += "\n" ;
      }
   }
   fin.close() ;

   fout << newContent ;

done:
   return rc ;
error:
   goto done ;
}

void versionGenForTools::_buildVersion( char *version, char *release,
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