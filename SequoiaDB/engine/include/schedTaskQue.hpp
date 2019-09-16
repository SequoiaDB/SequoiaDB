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

   Source File Name = schedTaskQue.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_TASK_QUE_HPP__
#define SCHED_TASK_QUE_HPP__

#include "pmdDef.hpp"
#include "ossQueue.hpp"
#include "ossPriorityQueue.hpp"
#include "schedDef.hpp"

namespace engine
{

   /*
      SCHED_TASK_QUE_TYPE define
   */
   enum SCHED_TASK_QUE_TYPE
   {
      SCHED_TASK_FIFO_QUE  = 1,
      SCHED_TASK_PIRORITY_QUE
   } ;

   /*
      _schedTaskQueBase define
   */
   class _schedTaskQueBase : public SDBObject
   {
      public:
         _schedTaskQueBase() {}
         virtual ~_schedTaskQueBase() {}

      public:
         virtual UINT32    size() = 0 ;
         virtual BOOLEAN   isEmpty() = 0 ;
         virtual void      push( const pmdEDUEvent &event, INT64 userData ) = 0 ;
         virtual BOOLEAN   pop( pmdEDUEvent &event,INT64 millisec ) = 0 ;

   } ;
   typedef _schedTaskQueBase schedTaskQueBase ;

   /*
      _schedFIFOTaskQue define
   */
   class _schedFIFOTaskQue : public _schedTaskQueBase
   {
      public:
         _schedFIFOTaskQue() ;
         virtual ~_schedFIFOTaskQue() ;

      public:
         virtual UINT32    size() ;
         virtual BOOLEAN   isEmpty() ;
         virtual void      push( const pmdEDUEvent &event, INT64 userData ) ;
         virtual BOOLEAN   pop( pmdEDUEvent &event, INT64 millisec ) ;

      private:
         ossQueue<pmdEDUEvent>      _queue ;
   } ;
   typedef _schedFIFOTaskQue schedFIFOTaskQue ;

   /*
      _schedPriorityTaskQue define
   */
   class _schedPriorityTaskQue : public _schedTaskQueBase
   {
      struct _priorityEvent
      {
         public:
            pmdEDUEvent    _event ;

         private:
            INT64          _priority ;

         public:
            _priorityEvent( const pmdEDUEvent &event,
                            INT64 priority = 0 )
            {
               _event = event ;
               _priority = 0 - priority ;
            }
            _priorityEvent()
            {
               _priority = 0 ;
            }

            bool operator< ( const _priorityEvent &right ) const
            {
               if ( _priority < right._priority )
               {
                  return true ;
               }
               return false ;
            }
      } ;
      typedef _priorityEvent priorityEvent ;

      public:
         _schedPriorityTaskQue() ;
         virtual ~_schedPriorityTaskQue() ;

      public:
         virtual UINT32    size() ;
         virtual BOOLEAN   isEmpty() ;
         virtual void      push( const pmdEDUEvent &event, INT64 userData ) ;
         virtual BOOLEAN   pop( pmdEDUEvent &event, INT64 millisec ) ;

      private:
         ossPriorityQueue<priorityEvent>        _queue ;
   } ;
   typedef _schedPriorityTaskQue schedPriorityTaskQue ;

}

#endif // SCHED_TASK_QUE_HPP__

