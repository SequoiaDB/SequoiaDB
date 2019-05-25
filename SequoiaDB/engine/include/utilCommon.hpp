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

   Source File Name = utilCommon.hpp

   Descriptive Name = Process MoDel Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for SequoiaDB,
   and all other process-initialization code.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILCOMMON_HPP_
#define UTILCOMMON_HPP_

#include "core.hpp"
#include "msgDef.h"
#include "pmdDef.hpp"
#include "utilStr.hpp"
#include "sdbInterface.hpp"
#include "msg.h"

#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      ROLE ENUM AND STRING TRANSFER
   */
   SDB_ROLE utilGetRoleEnum( const CHAR *role ) ;
   const CHAR* utilDBRoleStr( SDB_ROLE dbrole ) ;

   const CHAR* utilDBRoleShortStr( SDB_ROLE dbrole ) ;
   SDB_ROLE utilShortStr2DBRole( const CHAR *role ) ;

   /*
      ROLE_TYPE ENUM AND STRING TRANSFER
   */
   SDB_TYPE utilGetTypeEnum( const CHAR *type ) ;
   const CHAR* utilDBTypeStr( SDB_TYPE type ) ;

   SDB_TYPE utilRoleToType( SDB_ROLE role ) ;

   /*
      SDB_DB_STATUS AND STRING TRANSFER
   */
   const CHAR* utilDBStatusStr( SDB_DB_STATUS dbStatus ) ;
   SDB_DB_STATUS utilGetDBStatusEnum( const CHAR *status ) ;

   /*
      SDB_DATA_STATUS AND STRING TRANSFER
   */
   const CHAR* utilDataStatusStr( BOOLEAN dataIsOK, SDB_DB_STATUS dbStatus ) ;

   /*
      SDB_DB_MODE AND STRING TRANSFER
   */
   string      utilDBModeStr( UINT32 dbMode ) ;
   UINT32      utilGetDBModeFlag( const string &mode ) ;

   /*
      instance ID
    */
   BOOLEAN     utilCheckInstanceID ( UINT32 instanceID, BOOLEAN includeUnknown ) ;
   /*
      util get error bson
   */
   BSONObj        utilGetErrorBson( INT32 flags, const CHAR *detail ) ;

   /*
      util rc to shell return code
   */
   UINT32         utilRC2ShellRC( INT32 rc ) ;
   INT32          utilShellRC2RC( UINT32 src ) ;

}



#endif //UTILCOMMON_HPP_

