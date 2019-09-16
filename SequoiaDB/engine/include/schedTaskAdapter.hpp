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

   Source File Name = schedTaskAdapter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_TASK_ADAPTER_HPP__
#define SCHED_TASK_ADAPTER_HPP__

#include "schedTaskAdapterBase.hpp"
#include "schedTaskQue.hpp"
#include "schedTaskContainer.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "ossLatch.hpp"

namespace engine
{

   /*
      _schedFIFOAdapter define
   */
   class _schedFIFOAdapter : public _schedTaskAdapterBase
   {
      public:
         _schedFIFOAdapter() ;
         virtual ~_schedFIFOAdapter() ;

      protected:

         virtual INT32        _onInit( SCHED_TASK_QUE_TYPE queType ) ;
         virtual void         _onFini() ;

         virtual UINT32       _onPrepared( UINT32 expectNum ) ;

         virtual INT32        _onPush( const pmdEDUEvent &event,
                                       INT64 priority,
                                       const schedInfo *pInfo ) ;

         virtual SCHED_TYPE   _getType () const { return _type ; }

      private:
         schedTaskQueBase        *_pTaskQue ;
         SCHED_TYPE              _type ;

   } ;
   typedef _schedFIFOAdapter schedFIFOAdapter ;

   /*
      _schedContainerAdapter define
   */
   class _schedContainerAdapter : public _schedTaskAdapterBase
   {
      public:
         _schedContainerAdapter( schedTaskContanierMgr *pMgr ) ;
         virtual ~_schedContainerAdapter() ;

      protected:

         virtual INT32        _onInit( SCHED_TASK_QUE_TYPE queType ) ;
         virtual void         _onFini() ;

         virtual UINT32       _onPrepared( UINT32 expectNum ) ;

         virtual INT32        _onPush( const pmdEDUEvent &event,
                                       INT64 priority,
                                       const schedInfo *pInfo ) ;

         virtual SCHED_TYPE   _getType () const
         {
            return SCHED_TYPE_CONTAINER ;
         }

      private:
         schedTaskContanierMgr            *_pMgr ;

   } ;
   typedef _schedContainerAdapter schedContainerAdapter ;

}

#endif // SCHED_TASK_ADAPTER_HPP__
