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

   Source File Name = utilAccessDataHdfs.cpp

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

#include "utilAccessData.hpp"

_utilAccessDataHdfs::_utilAccessDataHdfs() : _loadModule(NULL)
{
}

_utilAccessDataHdfs::~_utilAccessDataHdfs()
{
   hdfsUnload() ;
   SAFE_OSS_FREE ( _loadModule ) ;
}

INT32 _utilAccessDataHdfs::initialize( void *pParamet )
{
   INT32 rc = SDB_OK ;
   utilAccessParametHdfs *temp = NULL ;
   if ( !pParamet )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   temp = (utilAccessParametHdfs*)pParamet ;

   _loadModule = SDB_OSS_NEW ossModuleHandle( "libhdfs.so", temp->pPath, 0 ) ;
   if ( !_loadModule )
   {
      rc = SDB_OOM ;
      PD_LOG ( PDERROR, "Failed to malloc memory" ) ;
      goto error ;
   }
   rc = _loadModule->init() ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to init load module,rc = %d", rc ) ;
      goto error ;
   }
   rc = _loadModule->resolveAddress ( "hdfsConnectAsUser", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to init load module,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
   rc = _pHdfs.ossHdfsConnect( temp->pHostName, temp->port, temp->pUser ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to connect to hdfs,rc = %d", rc ) ;
      goto error ;
   }
   rc = _loadModule->resolveAddress ( "hdfsOpenFile", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to init load module,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
   rc = _pHdfs.ossHdfsOpenFile( temp->pFileName, OSS_HDFS_RDONLY, 0, 0, 0 ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to open file,rc = %d", rc ) ;
      goto error ;
   }

   rc = _loadModule->resolveAddress ( "hdfsRead", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get read address,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
done:
   return rc ;
error:
   goto done ;
}

INT32 _utilAccessDataHdfs::readNextBuffer ( CHAR *pBuffer, UINT32 &size )
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
   INT32 iLenRead = 0 ;
   UINT32 sourceSize = 0 ;

   sourceSize = size ;
   while ( size != 0 )
   {
      rc = _pHdfs.ossHdfsRead( pBuffer, (INT32)size, iLenRead ) ;
      if ( 0 > rc )
      {
         rc = SDB_IO ;
         PD_LOG ( PDERROR, "Failed to read from file, rc = %d", rc ) ;
         goto error ;
      }
      else if ( 0 == rc )
      {
         rc = SDB_EOF ;
         goto done ;
      }
      else
      {
         rc = SDB_OK ;
         size -= iLenRead ;
      }
   }
done:
   size = sourceSize - size ;
   return rc ;
error:
   goto done ;
}

INT32 _utilAccessDataHdfs::hdfsUnload()
{
   INT32 rc = SDB_OK ;

   if ( !_loadModule )
   {
      goto done ;
   }

   rc = _loadModule->resolveAddress ( "hdfsCloseFile", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get CloseFile address,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
   _pHdfs.ossHdfsCloseFile() ;
   
   rc = _loadModule->resolveAddress ( "hdfsDisconnect", &_function ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get Disconnect address,rc = %d", rc ) ;
      goto error ;
   }
   _pHdfs.pFunctionAddress = (void*)_function ;
   _pHdfs.ossHdfsDisconnect() ;

done:
   return rc ;
error:
   goto done ;
}
