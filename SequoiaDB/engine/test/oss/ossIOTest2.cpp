/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

using namespace engine ;
char dirName[1024]="";
char pathName[1024]="";
char fileName[1024]="";
char fsTypeName[1024]="";
char fileSZName[1024] = "";

void printHelp (char *progName)
{
   printf ( "Syntax: %s [-d dir] [-f file] [-p path] [-t file or dir name] [-s file]\n \
                     -d  create/delete directory\n \
                     -f  create/delete file\n \
                     -p  check file type\n \
                     -t  check file system type\n \
                     -s  get file size \n",
            progName) ;
}
int main ( int argc, char **argv )
{
   INT32 rc = SDB_OK ;
   
   if ( argc !=3 && argc !=5 && argc != 7 && argc != 9 && argc != 11)
   {
      printHelp ((char*)argv[0]) ;
      return 0 ;
   }

   for ( int i=1; i<argc; i++ )
   {
      if ( strcmp ( (char*)argv[i], "-d" ) == 0 )
      {
         i++ ;
         strcpy(dirName, (char*)argv[i]);
      }
      else if ( strcmp ((char*)argv[i],"-f")==0 )
      {
         i++;
          strcpy(fileName, (char*)argv[i]); 
      }
      else if ( strcmp ((char*)argv[i],"-p")==0 )
      {
         i++ ;
         strcpy(pathName, (char*)argv[i]);
      }
      else if ( strcmp ((char*)argv[i],"-t")==0 )
      {
         i++ ;
         strcpy(fsTypeName, (char*)argv[i]);
      }
      else if ( strcmp ((char*)argv[i],"-s")==0 )
      {
         i++ ;
         strcpy(fileSZName, (char*)argv[i]);
      }
   }

   OSSFILE file ;

   if (0 != strlen(dirName))
   {    	
       rc = ossMkdir ( dirName ) ;
       if ( rc )
       {
          printf("failed to mkdir %s, rc = %d\n", dirName, rc);
          printf("error = %d\n", ossGetLastError() ) ;
          return 0 ;
       }
       else
       {
          printf("%s is created !\n",dirName);
       }
       
       rc = ossDelete ( dirName );
       if ( rc )
       {
          printf("failed to delete dir %s, rc = %d\n", dirName, rc);
          printf("error = %d\n", ossGetLastError() ) ;
          return 0 ;
       }
       else
       {
          printf("%s is deleted !\n",dirName);
       }
   }
   
   if (0 != strlen(fileName))
   {
      rc = ossOpen ( fileName, OSS_CREATE|OSS_READWRITE, OSS_RU|OSS_WU|OSS_RG, file ) ;
      if ( rc )
      {
         printf("failed to open file, rc = %d\n", rc);
         printf("error = %d\n", ossGetLastError() ) ;
         return 0 ;
      }
      else
      {
         printf("%s is created !\n",fileName);
      }
      
      ossClose( file );
      
      rc = ossDelete ( fileName );
      if ( rc )
      {
         printf("failed to delete file %s, rc = %d\n", fileName, rc);
         printf("error = %d\n", ossGetLastError() ) ;
         return 0 ;
      }
      else
      {
         printf("%s is deleted !\n",fileName);
      }
   }
   
   if (0 != strlen(pathName))
   {    	 
      SDB_OSS_FILETYPE   FileType = SDB_OSS_UNK ;
      rc = ossGetPathType ( pathName, &FileType ) ;
      if ( rc )
      {
         printf( "failed to retrieve path type - %s\n", pathName ) ;
         printf( "error = %d\n", rc );
         return 0 ;
      }
      else
      {
         switch (FileType)
         {
         case  SDB_OSS_FIL:      // file type
            printf("File Type - regular file\n");
            break;
         case  SDB_OSS_DIR:      // directory type
            printf("File Type - directory\n");
            break; 
         case  SDB_OSS_SYM:      // symbolic link
            printf("File Type - symbolic link\n");
            break;
         case  SDB_OSS_DEV:      // device
            printf("File Type - device\n");
            break;
         case  SDB_OSS_PIP:      // pipe
            printf("File Type - pipe\n");
            break;
         case  SDB_OSS_SCK:       // socket
            printf("FileType - socket\n");
            break;
         case  SDB_OSS_UNK:
         default:
            printf("File Type - unknown\n");
            break;
         }
      }
   }

   if (0 != strlen(fsTypeName))
   {
   	  INT32             rc = SDB_OK;
      OSS_FS_TYPE   fsType = OSS_FS_TYPE_UNKNOWN ;
      
      rc = ossGetFSType ( fsTypeName, &fsType ) ;
      if ( !rc )
      {	
         switch (fsType)
         {
         case OSS_FS_EXT2_OLD:
         	    printf("file system type is EXT2 OLD\n");
         	    break;
         case OSS_FS_EXT2:
         	    printf("file system type is EXT2 and EXT3 \n");
         	    break;
         case OSS_FS_NTFS:
         	    printf("file system type is NTFS \n");
         	    break;
         default:
         	    printf("file system type is UNKNOWN \n");
         	    break;
         }
      }
      else
      {
         printf("ossGetFSType failed, rc = %d\n", rc) ;
      }
   }
   
   
    if (0 != strlen(fileSZName))
   {
   	  INT32   rc = SDB_OK;
      INT64   fileSZ = 0 ;
      
      rc = ossGetFileSizeByName ( fileSZName, &fileSZ ) ;
      if ( !rc )
      {	
         printf(" %s  size :  %lld bytes\n", fileSZName, fileSZ); 
      }
      else
      {
         printf("ossGetFileSizeByName failed, rc = %d\n", rc) ;
      }
   }      
}
