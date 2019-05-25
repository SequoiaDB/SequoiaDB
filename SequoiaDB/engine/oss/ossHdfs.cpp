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

   Source File Name = ossHdfs.cpp

   Descriptive Name =

   When/how to use: parse Data util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2013  JW  Initial Draft

   Last Changed =

******************************************************************************/

#include "../util/hdfs.h"
#include "ossHdfs.hpp"


#define OSS_HDFS_CONNECT    (hdfsFS(*)(const char*,unsigned short,const char*))
#define OSS_HDFS_DISCONNECT (int(*)(hdfsFS))
#define OSS_HDFS_OPENFILE   (hdfsFile(*)(hdfsFS,const char*,int,int,short,int))
#define OSS_HDFS_CLOSEFILE  (int(*)(hdfsFS,hdfsFile))
#define OSS_HDFS_READ       (int(*)(hdfsFS,hdfsFile,void*,int))


int _ossHdfs::ossHdfsConnect( const char *hostName,
                              unsigned short port,
                              const char *user )
{
   int rc = 0 ;
   if ( !pFunctionAddress )
   {
      rc = -1 ;
      goto error ;
   }
   _pHdfsFS = (OSS_HDFS_CONNECT(pFunctionAddress))( hostName, port, user ) ;
   if ( !_pHdfsFS )
   {
      rc = -1 ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

int _ossHdfs::ossHdfsDisconnect()
{
   int rc = 0 ;
   if ( !pFunctionAddress || !_pHdfsFS )
   {
      rc = -1 ;
      goto error ;
   }
   rc = (OSS_HDFS_DISCONNECT(pFunctionAddress))( _pHdfsFS ) ;
   if ( -1 == rc )
   {
      goto error ;
   }
   _pHdfsFS = NULL ;
done:
   return rc ;
error:
   goto done ;
}

int _ossHdfs::ossHdfsOpenFile( const char *path,
                               int flags,
                               int bufferSize,
                               short replication,
                               int blocksize )
{
   int rc = 0 ;
   if ( !pFunctionAddress || !_pHdfsFS )
   {
      rc = -1 ;
      goto error ;
   }
   if ( OSS_HDFS_RDONLY == flags )
   {
      flags = O_RDONLY ;
   }
   else if ( OSS_HDFS_WRONLY == flags )
   {
      flags = O_WRONLY ;
   }
   
   _pHdfsFile = (OSS_HDFS_OPENFILE(pFunctionAddress))( _pHdfsFS,
                                                       path,
                                                       flags,
                                                       bufferSize,
                                                       replication,
                                                       blocksize ) ;
   if ( !_pHdfsFile )
   {
      rc = -1 ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

int _ossHdfs::ossHdfsCloseFile()
{
   int rc = 0 ;
   if ( !pFunctionAddress || !_pHdfsFS || !_pHdfsFile )
   {
      rc = -1 ;
      goto error ;
   }
   rc = (OSS_HDFS_CLOSEFILE(pFunctionAddress))( (hdfsFS)_pHdfsFS,
                                                (hdfsFile)_pHdfsFile ) ;
   if ( -1 == rc )
   {
      goto error ;
   }
   _pHdfsFile = NULL ;
done:
   return rc ;
error:
   goto done ;
}
   
int _ossHdfs::ossHdfsRead( char *buffer, int size, int &readSize )
{
   int rc = 0 ;
   if ( !pFunctionAddress || !_pHdfsFS || !_pHdfsFile )
   {
      rc = -1 ;
      goto error ;
   }
   readSize = (OSS_HDFS_READ(pFunctionAddress))( (hdfsFS)_pHdfsFS,
                                                 (hdfsFile)_pHdfsFile,
                                                 buffer,
                                                 size ) ;
   if ( 0 > readSize )
   {
      rc = -1 ;
      goto error ;
   }
   else if ( 0 == readSize )
   {
      rc = 0 ;
   }
   else
   {
      rc = 1 ;
   }
done:
   return rc ;
error:
   goto done ;
}

_ossHdfs::_ossHdfs() : _pHdfsFile(NULL),
                       _pHdfsFS(NULL),
                       pFunctionAddress(NULL)
{
}

_ossHdfs::~_ossHdfs()
{
}
