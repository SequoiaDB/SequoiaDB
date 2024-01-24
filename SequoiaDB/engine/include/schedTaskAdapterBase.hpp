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

   Source File Name = schedTaskAdapterBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_TASK_ADAPTER_BASE_HPP__
#define SCHED_TASK_ADAPTER_BASE_HPP__

#include "netDef.hpp"
#include "schedDef.hpp"
#include "pmdDef.hpp"
#include "schedTaskQue.hpp"

using namespace bson ;

namespace engine
{

   /*
      _schedTaskAdapterBase define
   */
   class _schedTaskAdapterBase : public SDBObject
   {
      public:
         _schedTaskAdapterBase() ;
         virtual ~_schedTaskAdapterBase() ;

         INT32                init( schedTaskInfo *pInfo,
                                    SCHED_TASK_QUE_TYPE queType ) ;

         void                 fini() ;

         BOOLEAN              pop( INT64 millisec,
                                   MsgHeader **pHeader,
                                   NET_HANDLE &handle,
                                   pmdEDUMemTypes &memType ) ;

         INT32                push( const NET_HANDLE &handle,
                                    const _MsgHeader *header,
                                    const schedInfo *pInfo ) ;

         UINT32               prepare( INT64 millisec ) ;

         void                 dump( BSONObjBuilder &builder ) ;
         void                 resetDump() ;

      protected:

         virtual INT32        _onInit( SCHED_TASK_QUE_TYPE queType ) = 0 ;
         virtual void         _onFini() = 0 ;

         virtual UINT32       _onPrepared( UINT32 expectNum ) = 0 ;

         virtual INT32        _onPush( const pmdEDUEvent &event,
                                       INT64 priority,
                                       const schedInfo *pInfo ) = 0 ;

         virtual SCHED_TYPE   _getType () const = 0 ;

      protected:

         void                 _push2Que( const pmdEDUEvent &event ) ;
         BOOLEAN              _isControlMsg( const pmdEDUEvent &event ) ;

      protected:
         schedFIFOTaskQue              _queue ;
         schedTaskInfo                 *_pTaskInfo ;

         ossAtomic32                   _doNotify ;
         ossAutoEvent                  _notifyEvent ;

         ossAtomic64                   _hardNum ;
         ossAtomic64                   _eventNum ;
         ossAtomic64                   _cacheNum ;

   } ;
   typedef _schedTaskAdapterBase schedTaskAdapterBase ;

}

#endif // SCHED_TASK_ADAPTER_BASE_HPP__
