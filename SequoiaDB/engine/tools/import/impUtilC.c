/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = impUtilC.c

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2023/02/10  TZB Initial Draft

   Last Changed =

*******************************************************************************/
#include "impUtilC.h"
#include "../client/client.h"

INT32 checkConnInfo( const CHAR *pHostname, const CHAR *pSvcname,
                     const CHAR *pUser, const CHAR *pPassword, BOOLEAN useSSL )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;

   if (useSSL)
   {
      rc = sdbSecureConnect( pHostname, pSvcname, pUser, pPassword, &conn ) ;
   }
   else
   {
      rc = sdbConnect( pHostname, pSvcname, pUser, pPassword, &conn ) ;
   }
   if ( SDB_INVALID_HANDLE != conn )
   {
      sdbDisconnect( conn ) ;
      sdbReleaseConnection( conn ) ;
   }
   return rc ;
}
