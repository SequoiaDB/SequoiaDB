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

   Source File Name = schedTaskAdapter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "schedTaskAdapter.hpp"
#include "pmdEnv.hpp"
#include "pd.hpp"

namespace engine
{

   #define SCHED_PREPARE_POP_TIMEWAIT              ( 100 )

   /*
      _schedFIFOAdapter implement
   */
   _schedFIFOAdapter::_schedFIFOAdapter()
   {
      _type = SCHED_TYPE_FIFO ;
   }

   _schedFIFOAdapter::~_schedFIFOAdapter()
   {
      fini() ;
   }

   INT32 _schedFIFOAdapter::_onInit( SCHED_TASK_QUE_TYPE queType )
   {
      INT32 rc = SDB_OK ;

      if ( SCHED_TASK_FIFO_QUE == queType )
      {
         _pTaskQue = SDB_OSS_NEW schedFIFOTaskQue() ;
         _type = SCHED_TYPE_FIFO ;
      }
      else
      {
         _pTaskQue = SDB_OSS_NEW schedPriorityTaskQue() ;
         _type = SCHED_TYPE_PRIORITY ;
      }

      if ( !_pTaskQue )
      {
         PD_LOG( PDERROR, "Alloc task que failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _schedFIFOAdapter::_onFini()
   {
      if ( _pTaskQue )
      {
         SDB_OSS_DEL _pTaskQue ;
         _pTaskQue = NULL ;
      }
   }

   UINT32 _schedFIFOAdapter::_onPrepared( UINT32 expectNum )
   {
      UINT32 count = 0 ;
      pmdEDUEvent event ;

      while( count < expectNum )
      {
         if ( _pTaskQue->pop( event, SCHED_PREPARE_POP_TIMEWAIT ) )
         {
            /// control msg not calc count
            if ( !_isControlMsg( event ) )
            {
               ++count ;
            }
            _push2Que( event ) ;
         }
         else
         {
            break ;
         }
      }

      return count ;
   }

   INT32 _schedFIFOAdapter::_onPush( const pmdEDUEvent &event,
                                     INT64 priority,
                                     const schedInfo *pInfo )
   {
      _pTaskQue->push( event, priority ) ;
      return SDB_OK ;
   }

   /*
      _schedContainerAdapter implement
   */
   _schedContainerAdapter::_schedContainerAdapter( schedTaskContanierMgr *pMgr )
   {
      _pMgr = pMgr ;
   }

   _schedContainerAdapter::~_schedContainerAdapter()
   {
      fini() ;
   }

   INT32 _schedContainerAdapter::_onInit( SCHED_TASK_QUE_TYPE queType )
   {
      return SDB_OK ;
   }

   void _schedContainerAdapter::_onFini()
   {
   }

   UINT32 _schedContainerAdapter::_onPrepared( UINT32 expectNum )
   {
      UINT32 count = 0 ;

      FLOAT64 calcCount = 0.0 ;
      UINT32 popCount = 0 ;
      UINT32 expectPopCount = 0 ;
      schedTaskContanierPtr ptr ;
      schedTaskContanierPtr savePtr ;
      pmdEDUEvent event ;

      _pMgr->resumeIterator() ;

      while( count < expectNum && _pMgr->nextIterator( ptr ) )
      {
         savePtr = ptr ;
         calcCount = ptr->calcCount( expectNum ) ;
         expectPopCount = (UINT32)( calcCount + 0.50 ) ;
         popCount = 0 ;

         do
         {
            if ( ptr->pop( event, SCHED_PREPARE_POP_TIMEWAIT ) )
            {
               /// control msg not cal count
               if ( !_isControlMsg( event ) )
               {
                  ++popCount ;
                  ++count ;
               }
               _push2Que( event ) ;
            }
            else
            {
               break ;
            }
         } while( popCount < expectPopCount && count < expectNum ) ;

         /// update ratio
         ptr->updateAdjustRatio( expectNum, calcCount, popCount ) ;
      }

      if ( savePtr.get() )
      {
         _pMgr->pauseIterator( savePtr ) ;
      }

      return count ;
   }

   INT32 _schedContainerAdapter::_onPush( const pmdEDUEvent &event,
                                          INT64 priority,
                                          const schedInfo *pInfo )
   {
      schedTaskContanierPtr ptr ;
      string containerName = pInfo->getContianerName() ;

      /// get container
      ptr = _pMgr->getContanier( containerName, TRUE ) ;

      ptr->push( event, priority ) ;

      ptr->holdOut() ;

      return SDB_OK ;
   }

}

