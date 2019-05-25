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

   Source File Name = url.h

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_URL_H__
#define UTIL_URL_H__

SDB_EXTERN_C_START

SDB_EXPORT INT32 urlDecodeSize( const CHAR *pBuffer, INT32 bufferSize ) ;

SDB_EXPORT void urlDecode( const CHAR *pBuffer, INT32 bufferSize,
                           CHAR **pOut, INT32 outSize ) ;

SDB_EXTERN_C_END

#endif