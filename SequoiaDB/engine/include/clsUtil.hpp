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

   Source File Name = clsUitl.hpp

   Descriptive Name = Replication Control Block Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSUTIL_HPP_
#define CLSUTIL_HPP_

#include <map>

#include "clsDef.hpp"
#include "netDef.hpp"
#include "clsBase.hpp"

using namespace std ;

namespace engine
{

   CLS_SYNC_STATUS clsSyncWindow( const DPS_LSN &remoteLsn,
                                  const DPS_LSN &fileBeginLsn,
                                  const DPS_LSN &memBeginLSn,
                                  const DPS_LSN &endLsn ) ;

   INT32 clsString2Strategy( const CHAR *str, INT32 &sty ) ;

   INT32 clsStrategy2String( INT32 sty, CHAR *str, UINT32 len ) ;


}

#endif //CLSUTIL_HPP_

