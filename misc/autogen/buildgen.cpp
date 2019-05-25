#include "buildgen.h"
#include "core.hpp"
#include <stdio.h>
#include "ossUtil.h"
#include "ossMem.hpp"
#define TIMEBUFFERSIZE 64
void BuildGen::run ()
{
   FILE *pFile = fopen ( SDB_ENGINE_OSSVER_FILE_RELATIVE_PATH, "rb+" ) ;
   INT64 fileSize = 0 ;
   CHAR *pBuffer = NULL ;
   size_t readSize  = 0 ;
   size_t writeSize = 0 ;
   struct tm *otm ;
   struct timeval tv ;
   struct timezone tz ;
   time_t tt ;
   CHAR *pPos = NULL ;
   CHAR timeInfo[TIMEBUFFERSIZE] = {0} ;

   if ( !pFile )
   {
      printf ( "Failed to open file %s\n",
               SDB_ENGINE_OSSVER_FILE_RELATIVE_PATH ) ;
      goto error ;
   }
   if ( 0 != fseek ( pFile, 0, SEEK_END ) )
   {
      printf ( "Failed to seek to end of file\n" ) ;
      goto error ;
   }
   if ( ( fileSize = ftell ( pFile ) ) < 0 )
   {
      printf ( "Failed to tell the current position\n" ) ;
      goto error ;
   }
   pBuffer = (CHAR*)SDB_OSS_MALLOC(sizeof(CHAR) * fileSize) ;
   if ( !pBuffer )
   {
      printf ( "Failed to allocate buffer memory for %lld bytes\n",
               fileSize ) ;
      goto error ;
   }
   if ( 0 != fseek ( pFile, 0, SEEK_SET ) )
   {
      printf ( "Failed to seek to begin of file\n" ) ;
      goto error ;
   }
   readSize = fread ( pBuffer, 1, fileSize, pFile ) ;
   if ( readSize != fileSize )
   {
      printf ( "Failed to read %lld bytes from file, actual read %lld\n",
               fileSize, (INT64)readSize ) ;
      goto error ;
   }
   gettimeofday(&tv, &tz);
   tt = tv.tv_sec ;
   otm = localtime ( &tt ) ;
   ossSnprintf ( timeInfo, TIMEBUFFERSIZE, SDB_ENGINE_BUILD_FORMAT,
                 otm->tm_year+1900,
                 otm->tm_mon+1,
                 otm->tm_mday,
                 otm->tm_hour,
                 otm->tm_min,
                 otm->tm_sec ) ;
   pPos = ossStrstr ( pBuffer, SDB_ENGINE_BUILD_CURRENT ) ;
   if ( !pPos )
   {
      printf ( "Failed to locate %s\n", SDB_ENGINE_BUILD_CURRENT ) ;
      goto error ;
   }
   if ( ossStrlen ( SDB_ENGINE_BUILD_CURRENT ) !=
        ossStrlen ( timeInfo ) )
   {
      printf ( "Length of timeinfo is not correct, expected: %d, actual: %d\n",
               (INT32)ossStrlen ( SDB_ENGINE_BUILD_CURRENT ),
               (INT32)ossStrlen ( timeInfo ) ) ;
      goto error ;
   }
   ossMemcpy ( pPos, timeInfo, ossStrlen ( timeInfo ) ) ;
   if ( 0 != fseek ( pFile, 0, SEEK_SET ) )
   {
      printf ( "Failed to seek to begin of file\n" ) ;
      goto error ;
   }
   writeSize = fwrite ( pBuffer, 1, fileSize, pFile ) ;
   if ( writeSize != fileSize )
   {
      printf ( "Failed to write %lld bytes from file, actual write %lld, errno = %d\n",
               fileSize, (INT64)writeSize, errno ) ;
      goto error ;
   }
done :
   if ( pFile )
   {
      fclose ( pFile ) ;
   }
   if ( pBuffer )
   {
      SDB_OSS_FREE ( pBuffer ) ;
   }
   return ;
error :
   goto done ;
}
