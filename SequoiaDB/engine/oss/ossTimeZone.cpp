/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = ossTimeZone.cpp

   Descriptive Name = Operating System Services Utility Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains wrappers for utilities like
   memcpy, strcmp, etc...

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/31/2022  Yang QinCheng  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossTimeZone.hpp"
#include "pd.hpp"
#include "oss.h"
#include "ossUtil.hpp"
#include "ossMem.h"
#include "ossPrimitiveFileOp.hpp"
#include <string>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#if defined (_LINUX)
#include <dirent.h>
#endif // _LINUX

#if defined(_LINUX)
static const CHAR *OSS_ENV_TIMEZONE = "TZ" ;
static const CHAR *ETC_TIMEZONE_FILE = "/etc/timezone" ;
static const CHAR *ZONEINFO_DIR = "/usr/share/zoneinfo" ;
static const CHAR *DEFAULT_ZONEINFO_FILE = "/etc/localtime" ;
static const CHAR *ZONEINFO_NAME = "zoneinfo/" ;

static const CHAR popularZones[][4] = { "UTC", "GMT" } ;

#define RESTARTABLE(__cmd, __result) do {             \
   do {                                               \
      __result = __cmd ;                              \
      if ( ( -1 == __result ) && ( EINTR == errno ) ) \
      {                                               \
         ossSleepmillis( 10 ) ;                       \
      }                                               \
      else                                            \
      {                                               \
         break ;                                      \
      }                                               \
   } while(TRUE) ;                                    \
} while( 0 )

static CHAR * _isFileIdentical( CHAR *buf, size_t size, CHAR *pathName ) ;

/*
 * Remove repeated path separators ('/') in the given 'path'.
 */
static void _removeDuplicateSlashes( CHAR *path )
{
   CHAR *left = path ;
   CHAR *right = path ;
   CHAR *end = path + ossStrlen( path ) ;

   for ( ; right < end; right++ )
   {
      // Skip sequence of multiple path-separators
      while ( *right == '/' && *( right + 1 ) == '/')
      {
         right++ ;
      }

      while ( *right != '\0' && !( *right == '/' && *( right + 1 ) == '/' ) )
      {
         *left++ = *right++ ;
      }

      if ( *right == '\0' )
      {
         *left = '\0' ;
         break;
      }
   }
}

/* Check the given name sequence to see if it can be further collapsed. Return zero if not,
 * otherwise return the number of names in the sequence.
 */
static INT32 _collapsible( CHAR *names )
{
   CHAR *p    = names ;
   INT32 dots = 0 ;
   INT32 n    = 0 ;

   while ( *p )
   {
      if ( ( p[0] == '.' ) && ( ( p[1] == '\0' )
                           || ( p[1] == '/' )
                           || ( ( p[1] == '.' ) && ( ( p[2] == '\0' )
                                                || ( p[2] == '/' ) ) ) ) )
      {
         dots = 1 ;
      }
      n++ ;
      while ( *p )
      {
         if ( *p == '/' )
         {
            p++ ;
            break ;
         }
         p++ ;
      }
   }
   return ( dots ? n : 0 ) ;
}


/* Split the names in the given name sequence, replacing slashes with nulls and filling in the
 * given index array.
 */
static void _splitNames( CHAR *names, CHAR **ix )
{
   CHAR *p = names ;
   INT32 i = 0 ;

   while ( *p )
   {
      ix[i++] = p++ ;
      while ( *p )
      {
         if ( *p == '/' )
         {
            *p++ = '\0' ;
            break ;
         }
         p++ ;
      }
   }
}

/* Join the names in the given name sequence, ignoring names whose index entries have been cleared
 * and replacing nulls with slashes as needed.
 */
static void _joinNames( CHAR *names, int nc, CHAR **ix )
{
   INT32 i = 0 ;
   CHAR *p = NULL ;

   for ( i = 0, p = names ; i < nc ; i++ )
   {
      if ( !ix[i] )
      {
         continue ;
      }
      if ( i > 0 )
      {
         p[-1] = '/' ;
      }
      if ( p == ix[i] )
      {
         p += strlen( p ) + 1 ;
      }
      else
      {
         CHAR *q = ix[i] ;
         while ( ( *p++ = *q++ ) ) ;
      }
   }
   *p = '\0' ;
}

/* Collapse "." and ".." names in the given path wherever possible. A "." name may always be
 * eliminated; a ".." name may be eliminated if it follows a name that is neither "." nor "..".
 * This is a syntactic operation that performs no filesystem queries, so it should only be used to
 * cleanup after invoking the realpath(  ) procedure.
 */
static INT32 _collapse( CHAR *path )
{
   INT32 rc = SDB_OK ;
   INT32 nc = 0 ;
   INT32 i  = 0 ;
   INT32 j  = 0 ;
   CHAR **ix = NULL ;
   // Preserve first '/'
   CHAR *names = ( path[0] == '/' ) ? path + 1 : path ;

   nc = _collapsible( names ) ;
   if ( nc < 2 )
   {
      // Do nothing
      goto done ;
   }
   ix = (CHAR **)SDB_OSS_MALLOC( nc * sizeof( CHAR * ) ) ;
   if ( NULL == ix )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   _splitNames( names, ix ) ;

   for ( i = 0 ; i < nc ; i++ )
   {
      INT32 dots = 0 ;

      // Find next occurrence of "." or ".."
      do {
         CHAR *p = ix[i] ;
         if ( p[0] == '.' )
         {
            if ( p[1] == '\0' )
            {
               dots = 1 ;
               break ;
            }
            if ( ( p[1] == '.' ) && ( p[2] == '\0' ) )
            {
               dots = 2 ;
               break ;
            }
         }
         i++ ;
      } while ( i < nc ) ;

      if ( i >= nc )
      {
         break ;
      }

      // At this point i is the index of either a "." or a "..", so take the appropriate action and
      // then continue the outer loop
      if ( 1 == dots )
      {
         // Remove this instance of "."
         ix[i] = 0 ;
      }
      else
      {
         // If there is a preceding name, remove both that name and this instance of "..";
         // otherwise, leave the ".." as is
         for ( j = i - 1 ; j >= 0 ; j-- )
         {
            if ( ix[j] ) 
            {
               break ;
            }
         }
         if ( j < 0 )
         {
            continue ;
         }
         ix[j] = 0 ;
         ix[i] = 0 ;
      }
      // i will be incremented at the top of the loop
   }

   _joinNames( names, nc, ix ) ;

done:
   if ( NULL != ix )
   {
      SDB_OSS_FREE( ix ) ;
   }
   return rc ;
error:
   goto done ;
}

/*
 * Returns a pointer to the time zone portion of the given zoneinfo file name, or NULL if the given
 * string doesn't contain "zoneinfo/".
 */
static CHAR * _getZoneName( CHAR *str )
{
   CHAR *pos = ossStrstr( str, ZONEINFO_NAME ) ;
   if ( NULL == pos )
   {
      return NULL ;
   }
   return pos + ossStrlen( ZONEINFO_NAME ) ;
}

/*
 * Returns a path name created from the given 'dir' and 'name' under UNIX.
 */
static CHAR * _getPathName( const CHAR *dir, const CHAR *name ) {
   CHAR *path = NULL ;

   path = (CHAR *)SDB_OSS_MALLOC( ossStrlen( dir ) + ossStrlen( name ) + 2 ) ;
   if ( NULL == path )
   {
      return NULL ;
   }
   return strcat( strcat( ossStrcpy( path, dir ), "/" ), name ) ;
}

/*
 * Scans the specified directory and its subdirectories to find a zoneinfo file which has the same
 * content as /etc/localtime. Returns the time zone information if found, otherwise, NULL is
 * returned.
 */
static CHAR * _findZoneinfoFile( CHAR *buf, size_t size, const CHAR *dir )
{
   DIR *dirp         = NULL ;
   struct dirent *dp = NULL ;
   CHAR *tz          = NULL ;
   CHAR *pathName    = NULL ;

   if ( 0 == ossStrcmp( dir, ZONEINFO_DIR ) )
   {
      // Fast path for 1st iteration
      for ( UINT32 i = 0 ; i < sizeof ( popularZones ) / sizeof ( popularZones[0] ) ; i++ )
      {
         pathName = _getPathName( dir, popularZones[i] ) ;
         if ( NULL == pathName )
         {
            continue ;
         }

         tz = _isFileIdentical( buf, size, pathName ) ;
         SDB_OSS_FREE( pathName ) ;
         pathName = NULL ;
         if ( NULL != tz )
         {
            return tz ;
         }
      }
   }

   dirp = opendir( dir ) ;
   if ( NULL == dirp )
   {
      return NULL ;
   }

   while ( NULL != ( dp = readdir( dirp ) ) )
   {
      // Skip '.' and '..' ( and possibly other .* files ), "ROC", "posixrules", and "localtime"
      if ( ( '.' == dp->d_name[0] )
         || ( 0 == ossStrcmp( dp->d_name, "ROC" ) )
         || ( 0 == ossStrcmp( dp->d_name, "posixrules" ) )
         || ( 0 == ossStrcmp( dp->d_name, "localtime" ) ) )
      {
         continue ;
      }

      pathName = _getPathName( dir, dp->d_name ) ;
      if ( NULL == pathName )
      {
         break ;
      }

      tz = _isFileIdentical( buf, size, pathName ) ;
      SDB_OSS_FREE( pathName ) ;
      pathName = NULL ;
      if ( tz != NULL )
      {
         break ;
      }
   }

   if ( NULL != dirp )
   {
      closedir( dirp ) ;
   }
   return tz ;
}

/*
 * Checks if the file pointed to by pathname matches the data contents in buf. Returns a
 * representation of the time zone file name if file match is found, otherwise NULL.
 */
static CHAR * _isFileIdentical( CHAR *buf, size_t size, CHAR *pathName )
{
   CHAR *possibleMatch     = NULL ;
   CHAR *dbuf              = NULL ;
   INT32 fd                = -1 ;
   INT32 res               = 0 ;
   struct oss_stat statbuf = { 0 } ;

   RESTARTABLE( oss_stat( pathName, &statbuf ), res ) ;
   if ( -1 == res )
   {
      PD_LOG( PDERROR, "Failed to get file status of %s, errno = %d ", pathName,
              ossGetLastError() ) ;
      return NULL ;
   }

   if ( S_ISDIR( statbuf.st_mode ) )
   {
      possibleMatch  = _findZoneinfoFile( buf, size, pathName ) ;
   }
   else if ( S_ISREG( statbuf.st_mode ) && (size_t)statbuf.st_size == size )
   {
      dbuf = (CHAR *)SDB_OSS_MALLOC( size ) ;
      if ( NULL == dbuf )
      {
         return NULL ;
      }
      RESTARTABLE( open( pathName, O_RDONLY ), fd ) ;
      if ( -1 == fd )
      {
         PD_LOG( PDERROR, "Failed to open file %s with O_RDONLY mode, errno = %d ", pathName,
                 ossGetLastError() ) ;
         goto freedata ;
      }
      RESTARTABLE( read( fd, dbuf, size ), res ) ;
      if ( res != ( ssize_t ) size )
      {
         PD_LOG( PDERROR, "Failed to read file %s, res = %d, size = %u, errno = %d ", pathName,
                 res, size, ossGetLastError() ) ;
         goto freedata ;
      }
      if ( 0 == ossMemcmp( buf, dbuf, size ) )
      {
         possibleMatch = _getZoneName( pathName ) ;
         if ( NULL != possibleMatch )
         {
            possibleMatch = ossStrdup( possibleMatch ) ;
         }
      }

   freedata:
      if ( NULL != dbuf )
      {
         SDB_OSS_FREE( dbuf ) ;
      }
      if ( -1 != fd )
      {
         close( fd ) ;
      }
   }
   return possibleMatch ;
}
#endif // _LINUX

/*
 * Get time zone info from system file. Refer to the openJDK for inernal implementation.
 */
INT32 ossGetSysTimeZone( CHAR *pTimeZone, INT32 len )
{
   INT32 rc    = SDB_OK ;
   CHAR *tz    = NULL ;
   INT32 fd    = -1 ;
   INT32 res   = 0 ;
   CHAR *buf   = NULL ;
   FILE *fp    = NULL ;
   size_t size = 0 ;
   INT32 tzLen = 0 ;
   oss_struct_stat statbuf = { 0 } ;

#if defined(_LINUX)
   if ( NULL == pTimeZone || 0 >= len )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   /*
    * Try reading the /etc/timezone file for Debian distros. There's no spec of the file format
    * available. This parsing assumes that there's one line of an Olson tzid followed by a '\n', no
    * leading or trailing spaces, no comments.
    */
   fp = fopen( ETC_TIMEZONE_FILE, "r" ) ;
   if ( NULL != fp )
   {
      CHAR line[OSS_TIMEZONE_MAX_LEN] = { 0 } ;
      if ( NULL != fgets( line, sizeof( line ), fp ) )
      {
         CHAR *p = ossStrchr( line, '\n' ) ;
         if ( NULL != p )
         {
            *p = '\0' ;
         }
         if ( ossStrlen( line ) > 0 )
         {
            tz = ossStrdup( line ) ;
         }
      }
      fclose( fp ) ;
      if ( NULL != tz )
      {
         PD_LOG( PDDEBUG, "Read time zone information %s from %s", tz, ETC_TIMEZONE_FILE ) ;
         goto done ;
      }
   }
   else
   {
      PD_LOG( PDDEBUG, "Failed to read time zone information from %s, errno = %d ",
              ETC_TIMEZONE_FILE, ossGetLastError() ) ;
   }

   /*
    * Next, try to find the time zone information from /etc/localtime.
    */
   RESTARTABLE( oss_lstat( DEFAULT_ZONEINFO_FILE, &statbuf ), res ) ;
   if ( -1 == res )
   {
      PD_LOG( PDERROR, "Failed to get file information of %s, errno = %d ", DEFAULT_ZONEINFO_FILE,
              ossGetLastError() ) ;
      rc = SDB_IO ;
      goto error ;
   }

   /*
    * If the /etc/localtime file is a symlink, get the link name and its zone ID part. (The older
    * versions of timeconfig created a symlink as described in the Red Hat man page. It was changed
    * in 1999 to create a copy of a zoneinfo file. It's no longer possible to get the zone ID from
    * /etc/localtime.)
    */
   if ( S_ISLNK( statbuf.st_mode ) )
   {
      CHAR linkbuf[PATH_MAX + 1] = { 0 } ;
      rc = ossReadlink( DEFAULT_ZONEINFO_FILE, linkbuf, sizeof( linkbuf ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get a symlink of %s, rc = %d", DEFAULT_ZONEINFO_FILE, rc ) ;
         goto error ;
      }

      _removeDuplicateSlashes( linkbuf ) ;
      rc = _collapse( linkbuf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to collapse \".\" and \"..\" names in the given path: %s, rc = %d", linkbuf, rc ) ;
         goto error ;
      }

      tz = _getZoneName( linkbuf ) ;
      if ( NULL != tz )
      {
         tz = ossStrdup( tz ) ;
         goto done ;
      }
   }

   /*
    * If the /etc/localtime file is a regular file, we need to find out the same zoneinfo file
    * that has been copied as /etc/localtime.
    */
   RESTARTABLE( open( DEFAULT_ZONEINFO_FILE, O_RDONLY ), fd ) ;
   if ( -1 == fd )
   {
      PD_LOG( PDERROR, "Failed to open file %s with O_RDONLY mode, errno = %d ",
              DEFAULT_ZONEINFO_FILE, ossGetLastError() ) ;
      rc = SDB_IO ;
      goto error ;
   }

   RESTARTABLE( oss_fstat( fd, &statbuf ), res ) ;
   if ( -1 == res )
   {
      PD_LOG( PDERROR, "Failed to get file status from %s, errno = %d ", DEFAULT_ZONEINFO_FILE,
              ossGetLastError() ) ;
      rc = SDB_IO ;
      goto error ;
   }

   size = (size_t) statbuf.st_size ;
   buf = (CHAR *)SDB_OSS_MALLOC( size ) ;
   if ( NULL == buf )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   RESTARTABLE( read( fd, buf, size ), res ) ;
   if ( res != (ssize_t) size )
   {
      PD_LOG( PDERROR, "Failed to read file %s, res = %d, size = %u, errno = %d ",
              DEFAULT_ZONEINFO_FILE, res, size, ossGetLastError() ) ;
      rc = SDB_IO ;
      goto error ;
   }
   close( fd ) ;

   tz = _findZoneinfoFile( buf, size, ZONEINFO_DIR ) ;
   if ( NULL == tz )
   {
      PD_LOG( PDERROR, "Failed to find the same file as %s in %s", DEFAULT_ZONEINFO_FILE,
              ZONEINFO_DIR ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done:
   if ( -1 != fd )
   {
      close( fd ) ;
   }
   if ( NULL != buf )
   {
      SDB_OSS_FREE( buf ) ;
   }
   if ( NULL != tz )
   {
      tzLen = ossStrlen( tz ) ;
      if ( len < ( tzLen + 1 ) )
      {
         PD_LOG( PDERROR, "The given length[%d] is less than the actual length[%d].", len,
                 tzLen ) ;
         rc = SDB_INVALIDSIZE ;
      }
      else
      {
         ossStrncpy( pTimeZone, tz, tzLen ) ;
         pTimeZone[tzLen] = 0 ;
      }
      SDB_OSS_FREE( tz ) ;
   }
   return rc ;
error:
   goto done ;

#else
   return rc ;
#endif  // _LINUX
}

INT32 ossGetTZEnv ( CHAR *pTimeZone, INT32 len )
{
   INT32 rc        = SDB_OK ;
   INT32 valueLen  = 0 ;
   CHAR *pEnvValue = NULL ;

#if defined (_LINUX)
   if ( NULL == pTimeZone || 0 >= len )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pEnvValue = getenv( OSS_ENV_TIMEZONE ) ;
   if ( NULL != pEnvValue )
   {
      valueLen = ossStrlen( pEnvValue );
   }

   if ( 0 < valueLen )
   {
      if ( len < ( valueLen + 1 ) )
      {
         PD_LOG( PDERROR, "The given length[%d] is less than the actual length[%d].", len,
                 valueLen ) ;
         rc = SDB_INVALIDSIZE ;
         goto error ;
      }
      ossStrncpy( pTimeZone, pEnvValue, valueLen ) ;
      pTimeZone[valueLen] = 0 ;
   }
   else
   {
      PD_LOG( PDDEBUG, "Environment variable %s value is empty", OSS_ENV_TIMEZONE ) ;
   }

done:
   return rc ;
error:
   goto done ;

#else
   return rc ;
#endif  // _LINUX
}

INT32 ossInitTZEnv ()
{
   INT32 rc           = SDB_OK ;
   const INT32 len    = OSS_TIMEZONE_MAX_LEN + 1 ;
   CHAR timeZone[len] = { 0 } ;

#if defined (_LINUX)
   // check TZ
   rc = ossGetTZEnv( timeZone, len ) ;
   if ( SDB_OK == rc && 0 < ossStrlen( timeZone ) )
   {
      PD_LOG( PDEVENT, "The environment variable %s exists, vlaue: %s", OSS_ENV_TIMEZONE,
              timeZone ) ;
      goto done ;
   }
   else
   {
      PD_LOG( PDEVENT, "The environment variable %s does not exists", OSS_ENV_TIMEZONE ) ;
   }

   // TZ not exsits, get the time zone information of the current system
   rc = ossGetSysTimeZone( timeZone, len ) ;
   if ( SDB_OK == rc && 0 < ossStrlen( timeZone ) )
   {
      if ( 0 == setenv( OSS_ENV_TIMEZONE, timeZone, 0 ) )
      {
         PD_LOG( PDEVENT, "Set the environment variable %s to %s", OSS_ENV_TIMEZONE, timeZone ) ;
         tzset() ;
         goto done ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to set the %s environment variable to %s. erron = %d",
                 OSS_ENV_TIMEZONE, timeZone, ossGetLastError() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   }
done:
   return rc ;
error:
   goto done ;

#else
   return rc ;
#endif // _LINUX
}
