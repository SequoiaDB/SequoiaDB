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

   Source File Name = schedTaskQue.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "schedTaskQue.hpp"

namespace engine
{

   /*
      _schedFIFOTaskQue implement
   */
   _schedFIFOTaskQue::_schedFIFOTaskQue()
   {
   }

   _schedFIFOTaskQue::~_schedFIFOTaskQue()
   {
   }

   UINT32 _schedFIFOTaskQue::size()
   {
      return _queue.size() ;
   }

   BOOLEAN _schedFIFOTaskQue::isEmpty()
   {
      return _queue.empty() ;
   }

   void _schedFIFOTaskQue::push( const pmdEDUEvent &event, INT64 userData )
   {
      _queue.push( event ) ;
   }

   BOOLEAN _schedFIFOTaskQue::pop( pmdEDUEvent &event, INT64 millisec )
   {
      if ( millisec < 0 )
      {
         _queue.wait_and_pop( event ) ;
         return TRUE ;
      }
      else if ( 0 == millisec )
      {
         return _queue.try_pop( event ) ;
      }
      else
      {
         return _queue.timed_wait_and_pop( event, millisec ) ;
      }
   }

   /*
      _schedPriorityTaskQue implement
   */
   _schedPriorityTaskQue::_schedPriorityTaskQue()
   {
   }

   _schedPriorityTaskQue::~_schedPriorityTaskQue()
   {
   }

   UINT32 _schedPriorityTaskQue::size()
   {
      return _queue.size() ;
   }

   BOOLEAN _schedPriorityTaskQue::isEmpty()
   {
      return _queue.empty() ;
   }

   void _schedPriorityTaskQue::push( const pmdEDUEvent &event, INT64 userData )
   {
      _queue.push( priorityEvent( event, userData ) ) ;
   }

   BOOLEAN _schedPriorityTaskQue::pop( pmdEDUEvent &event, INT64 millisec )
   {
      BOOLEAN ret = FALSE ;
      priorityEvent tmpEvent ;

      if ( millisec < 0 )
      {
         _queue.wait_and_pop( tmpEvent ) ;
         ret = TRUE ;
      }
      else if ( 0 == millisec )
      {
         ret = _queue.try_pop( tmpEvent ) ;
      }
      else
      {
         ret = _queue.timed_wait_and_pop( tmpEvent, millisec ) ;
      }

      if ( ret )
      {
         event = tmpEvent._event ;
      }
      return ret ;
   }

}

