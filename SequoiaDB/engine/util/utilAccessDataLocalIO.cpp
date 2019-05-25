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

   Source File Name = utilAccessDataLocalIO.cpp

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

_utilAccessDataLocalIO::_utilAccessDataLocalIO()
{
}

_utilAccessDataLocalIO::~_utilAccessDataLocalIO()
{
   ossClose ( _fileIO ) ;
}

INT32 _utilAccessDataLocalIO::initialize( void *pParamet )
{
   INT32 rc = SDB_OK ;
   utilAccessParametLocalIO *temp = NULL ;
   if ( !pParamet )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   temp = (utilAccessParametLocalIO*)pParamet ;

   rc = ossOpen ( temp->pFileName,
                  OSS_READONLY | OSS_SHAREREAD,
                  OSS_DEFAULTFILE,
                  _fileIO ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to open input file %s, rc = %d",
               temp->pFileName, rc ) ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 _utilAccessDataLocalIO::readNextBuffer ( CHAR *pBuffer, UINT32 &size )
{
   INT32 rc = SDB_OK ;
   SINT64 iLenRead = 0 ;
   UINT32 sourceSize = 0 ;
   SINT64 readPos = 0 ;
   sourceSize = size ;
   SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
   while ( size > 0 )
   {
      rc = ossRead ( &_fileIO, pBuffer + readPos, size, &iLenRead ) ;
      if ( rc && SDB_INTERRUPT != rc && SDB_EOF != rc )
      {
         PD_LOG ( PDERROR, "Failed to read from file, rc = %d", rc ) ;
         goto error ;
      }
      else if ( rc == SDB_EOF )
      {
         goto done ;
      }
      else
      {
         rc = SDB_OK ;
         size -= iLenRead ;
         readPos += iLenRead ;
      }
   }
done:
   size = sourceSize - size ;
   return rc ;
error:
   goto done ;
}