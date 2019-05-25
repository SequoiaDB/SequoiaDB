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

   Source File Name = ossHdfs.hpp

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
#ifndef OSSHDFS_HPP_
#define OSSHDFS_HPP_

#define OSS_HDFS_RDONLY 1
#define OSS_HDFS_WRONLY 2

class _ossHdfs
{
private:
   void* _pHdfsFile ;
   void* _pHdfsFS ;
public:
   void* pFunctionAddress ;
public:
   int ossHdfsConnect( const char *hostName,
                       unsigned short port,
                       const char *user ) ;
   int ossHdfsDisconnect() ;
   int ossHdfsOpenFile( const char *path, int flags, int bufferSize,
                        short replication, int blocksize ) ;
   int ossHdfsCloseFile() ;
   int ossHdfsRead( char *buffer, int size, int &readSize ) ;
   _ossHdfs() ;
   ~_ossHdfs() ;
} ;
typedef class _ossHdfs ossHdfs ;

#endif