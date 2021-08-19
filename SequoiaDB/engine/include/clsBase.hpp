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

   Source File Name = clsBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_BASE_HPP_
#define CLS_BASE_HPP_

#include "core.hpp"
#include "msg.hpp"
#include "oss.hpp"
#include <vector>
#include <string>

namespace engine
{

   enum CLS_MEMBER_TYPE
   {
      CLS_SHARD   = 1,           //shard
      CLS_REPL    = 2            //repl
   };

   enum CLS_INNER_SESSION_TID
   {
      CLS_TID_REPL_SYC     = 1,  //repl active sync
      CLS_TID_REPL_FS_SYC  = 2,  //repl active full sync

      CLS_TID_END          = 10  //end
   };

   #define CLS_INVALID_TIMERID         (0)

   enum CLS_SYNC_STRATEGY
   {
      CLS_SYNC_NONE        = 0,
      CLS_SYNC_KEEPNORMAL  = 1,
      CLS_SYNC_KEEPALL     = 2
   } ;
   #define CLS_SYNC_DTF_STRATEGY    CLS_SYNC_KEEPNORMAL

   typedef MsgRouteID   NodeID ;
   #define INVALID_NODE_ID       (MSG_INVALID_ROUTEID)

   #define SAFE_DELETE(p) \
      do { \
         if ( p ) \
         { \
            SDB_OSS_DEL p ; \
            p = NULL ; \
         } \
      } while (0)

   #define SAFE_NEW_GOTO_ERROR(p, className) \
      do { \
         p = SDB_OSS_NEW className() ; \
         if ( !p ) \
         { \
            PD_LOG ( PDERROR, "Failed to allocate memory to #className" ) ; \
            rc = SDB_OOM ; \
            goto error ; \
         } \
      } while (0)

   #define SAFE_NEW_GOTO_ERROR1(p, className, arg1) \
      do { \
         p = SDB_OSS_NEW className( arg1 ) ; \
         if ( !p ) \
         { \
            PD_LOG ( PDERROR, "Failed to allocate memory to #className" ) ; \
            rc = SDB_OOM ; \
            goto error ; \
         } \
      } while (0)

   #define SAFE_NEW_GOTO_ERROR2(p, className, arg1, arg2) \
      do { \
         p = SDB_OSS_NEW className( arg1, arg2 ) ; \
         if ( !p ) \
         { \
            PD_LOG ( PDERROR, "Failed to allocate memory to #className" ) ; \
            rc = SDB_OOM ; \
            goto error ; \
         } \
      } while (0)

   #define INIT_OBJ_GOTO_ERROR(p) \
      do { \
         if ( SDB_OK != ( rc = p->initialize () ) ) \
         { \
            PD_LOG ( PDERROR, "init failed" ) ; \
            goto error ; \
         } \
      } while (0)

}

#endif //CLS_BASE_HPP_



