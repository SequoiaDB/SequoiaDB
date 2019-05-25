/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

*******************************************************************************/

#include "core.hpp"
#include "ossIO.hpp"
#include "ossMmap.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsExtent.hpp"
#include "dmsRecord.hpp"
#include "boost/thread.hpp"
#include <stdio.h>
#include <vector>

#define DFT_FILENAME "testMmap.dat"
#define DFT_SEGSIZE  1024*1024
#define DFT_NUMSEG   50
using namespace engine ;
char fileName[1024];
SINT64 segmentSize = DFT_SEGSIZE ;
SINT64 numSeg      = DFT_NUMSEG ;
void printHelp (char *progName)
{
   printf ( "Syntax: %s [-f filename] [-s segment bytes] [-n num segments]\n",
            progName) ;
}
int main ( int argc, char **argv )
{
   INT32 rc = SDB_OK ;
   boost::posix_time::ptime t1 ;
   boost::posix_time::ptime t2 ;
   	
   if ( argc!= 1 && argc !=3 && argc !=5 && argc !=7 )
   {
      printHelp ((char*)argv[0]) ;
      return 0 ;
   }
   strcpy (fileName, DFT_FILENAME);

   for ( int i=1; i<argc; i++ )
   {
      if ( strcmp ( (char*)argv[i], "-f" ) == 0 )
      {
         i++ ;
         strcpy(fileName, (char*)argv[i]);
      }
      else if ( strcmp ((char*)argv[i],"-s")==0 )
      {
         i++;
         segmentSize = atoi ( (char*)argv[i]);
      }
      else if ( strcmp ((char*)argv[i],"-n")==0 )
      {
         i++;
         numSeg = atoi ((char*)argv[i]);
      }
   }
   printf("filename = %s\n", fileName );
   printf("numSeg   = %lld\n", numSeg ) ;
   printf("segSize  = %lld\n", segmentSize ) ;
   
   t1 = boost::posix_time::microsec_clock::local_time() ;
   	
   OSSFILE file ;
   rc = ossOpen ( fileName, OSS_CREATE|OSS_READWRITE, OSS_RU|OSS_WU|OSS_RG, file ) ;
   if ( rc )
   {
      printf("failed to open file, rc = %d\n", rc);
      printf("error = %d\n", ossGetLastError() ) ;
      return 0 ;
   }
   INT64 fileSize = 0 ;
   rc = ossGetFileSize ( &file, &fileSize ) ;
   if ( rc )
   {
      printf("failed to get file size, rc = %d\n",rc ) ;
      printf("error = %d\n", ossGetLastError() );
      return 0 ;
   }
   rc = ossExtendFile(&file, numSeg*segmentSize-fileSize) ;
   if ( rc )
   {
      printf("Failed to extend file for extra %lld bytes",
             numSeg*segmentSize-fileSize) ;
      return 0 ;
   }
   char *pBuffer = (char*)SDB_OSS_MALLOC(sizeof(char)*segmentSize) ;
   if ( !pBuffer )
   {
      printf("failed to allocate memory for %lld bytes\n", segmentSize ) ;
      printf("error = %d\n", ossGetLastError() );
      return 0;
   }
   
   SINT64   lenWritten = 0 ;
   rc = ossSeek(&file, 0, OSS_SEEK_SET ) ;
   if ( rc )
   {
      printf("failed to seek to beginning of the file\n");
      printf("error = %d\n", ossGetLastError() );
      return 0;
   }
   
   for ( int i=0; i<numSeg; i++)
   {
      memset(pBuffer, (char)i, sizeof(char)*segmentSize) ;
      rc = ossWrite(&file, pBuffer, segmentSize, &lenWritten);
      if ( rc )
      {
         printf("failed to write file, rc = %d\n", rc) ;
         printf("error = %d\n", ossGetLastError());
         return 0;
      }
      if ( (UINT32)lenWritten != sizeof(char)*segmentSize )
      {
         printf("failed to write %lld bytes, rc = %d\n",
                sizeof(char)*segmentSize, rc );
         printf("actually written %lld bytes\n", lenWritten);
         printf("error = %d\n", ossGetLastError() ) ;
         return 0 ;
      }
   }

   if ( pBuffer )
   {
      SDB_OSS_FREE(pBuffer);
      pBuffer = NULL ;
   }
   ossClose ( file ) ;
   t2 = boost::posix_time::microsec_clock::local_time() ;
   boost::posix_time::time_duration diff = t2-t1 ;
   printf ( "Takes %lld ms to perform this test \n",
            (UINT64)diff.total_milliseconds()) ;
}
