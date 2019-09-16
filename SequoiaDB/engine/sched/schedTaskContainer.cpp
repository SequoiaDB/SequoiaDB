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

   Source File Name = schedTaskContainer.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "schedTaskContainer.hpp"
#include "pmdEnv.hpp"
#include "pd.hpp"

namespace engine
{

   #define SCHED_MAX_ADJUST_RATIO               ( 0.15 )
   #define SCHED_ADJUST_RATIO_FACTOR            ( 0.36 )
   #define SCHED_ADJUST_RATIO_THRESHOLD         ( 0.30 )

   #define SCHED_FULL_RATIO_THRESHOLD           ( 0.9999 )

   #define SCHED_WEIGHT_BASE                    ( 50 )
   #define SCHED_WEIGHT_STEP                    ( 6 )
   #define SCHED_WEIGHT_FACTOR                  ( 2 )

   /*
      _schedTaskContanier implement
   */
   _schedTaskContanier::_schedTaskContanier()
   :_holdNum( 0 )
   {
      _pTaskQue = NULL ;
      _nice = SCHED_NICE_DFT ;
      _weightValue = _nice2Weight( _nice ) ;

      _fixedRatio = 1.0 ;
      _adjustRatio = 0.0 ;
   }

   _schedTaskContanier::~_schedTaskContanier()
   {
      fini() ;
   }

   void _schedTaskContanier::holdIn()
   {
      _holdNum.inc() ;
   }

   void _schedTaskContanier::holdOut()
   {
      _holdNum.dec() ;
   }

   BOOLEAN _schedTaskContanier::hasHold()
   {
      return _holdNum.compare( 0 ) ;
   }

   schedTaskQueBase* _schedTaskContanier::getTaskQue()
   {
      return _pTaskQue ;
   }

   INT32 _schedTaskContanier::init( const string &name,
                                    SCHED_TASK_QUE_TYPE queType )
   {
      INT32 rc = SDB_OK ;

      _name = name ;

      if ( SCHED_TASK_FIFO_QUE == queType )
      {
         _pTaskQue = SDB_OSS_NEW schedFIFOTaskQue() ;
      }
      else
      {
         _pTaskQue = SDB_OSS_NEW schedPriorityTaskQue() ;
      }

      if ( !_pTaskQue )
      {
         PD_LOG( PDERROR, "Allocate task queue failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _schedTaskContanier::fini()
   {
      if ( _pTaskQue )
      {
         SDB_OSS_DEL _pTaskQue ;
         _pTaskQue = NULL ;
      }
   }

   void _schedTaskContanier::push( const pmdEDUEvent &event, INT64 userData )
   {
      if ( _pTaskQue )
      {
         _pTaskQue->push( event, userData ) ;
      }
   }

   BOOLEAN _schedTaskContanier::pop( pmdEDUEvent &event, INT64 millisec )
   {
      if ( _pTaskQue )
      {
         return _pTaskQue->pop( event, millisec ) ;
      }
      return FALSE ;
   }

   void _schedTaskContanier::setNice( INT32 nice )
   {
      if ( nice > SCHED_NICE_MAX )
      {
         _nice = SCHED_NICE_MAX ;
      }
      else if ( nice < SCHED_NICE_MIN )
      {
         _nice = SCHED_NICE_MIN ;
      }
      else
      {
         _nice = nice ;
      }

      _weightValue = _nice2Weight( _nice ) ;
   }

   void _schedTaskContanier::updateFixedRatio( UINT64 totalWeight )
   {
      if ( totalWeight > 0 )
      {
         _fixedRatio = ( FLOAT64 )_weightValue / totalWeight ;
      }
      else
      {
         _fixedRatio = 1.0 ;
      }
   }

   void _schedTaskContanier::updateAdjustRatio( UINT32 total,
                                                FLOAT64 needProcessNum,
                                                UINT32 hasProcessedNum )
   {
      if ( 0 == total ||
           hasProcessedNum >= total ||
           (FLOAT64)hasProcessedNum >= needProcessNum )
      {
         _adjustRatio = 0.0 ;
      }
      else
      {
         _adjustRatio = ( needProcessNum - hasProcessedNum ) / (FLOAT64)total ;

         if ( _adjustRatio > SCHED_ADJUST_RATIO_THRESHOLD )
         {
            _adjustRatio *= SCHED_ADJUST_RATIO_FACTOR ;
         }

         if ( _adjustRatio > SCHED_MAX_ADJUST_RATIO )
         {
            _adjustRatio = SCHED_MAX_ADJUST_RATIO ;
         }
      }
   }

   UINT64 _schedTaskContanier::getWeightValue() const
   {
      return _weightValue ;
   }

   FLOAT64 _schedTaskContanier::calcCount( UINT32 total )
   {
      FLOAT64 totalRatio = _fixedRatio + _adjustRatio ;

      if ( totalRatio > SCHED_FULL_RATIO_THRESHOLD )
      {
         return ( FLOAT64 ) total ;
      }
      return total * totalRatio ;
   }

   UINT64 _schedTaskContanier::_nice2Weight( INT32 nice ) const
   {
      return SCHED_WEIGHT_BASE + ( SCHED_NICE_MAX - nice ) *
             ( SCHED_NICE_MAX - nice ) * SCHED_WEIGHT_STEP /
             SCHED_WEIGHT_FACTOR ;
   }

   /*
      _schedTaskContanierMgr implement
   */
   _schedTaskContanierMgr::_schedTaskContanierMgr()
   {
      _queType = SCHED_TASK_PIRORITY_QUE ;
   }

   _schedTaskContanierMgr::~_schedTaskContanierMgr()
   {
   }

   INT32 _schedTaskContanierMgr::init( SCHED_TASK_QUE_TYPE queType )
   {
      INT32 rc = SDB_OK ;
      schedTaskContanier *pContainer = NULL ;

      _queType = queType ;

      pContainer = SDB_OSS_NEW schedTaskContanier() ;
      if ( !pContainer )
      {
         PD_LOG( PDERROR, "Allocate container failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = pContainer->init( SCHED_SYS_CONTAINER_NAME, _queType ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init container failed, rc: %d", rc ) ;
         goto error ;
      }

      _defaultPtr = schedTaskContanierPtr( pContainer ) ;
      pContainer = NULL ;

      _mapContanier[ _defaultPtr->getName() ] = _defaultPtr ;
      _lastPtr = _mapContanier.begin()->second ;

   done:
      if ( pContainer )
      {
         SDB_OSS_DEL pContainer ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _schedTaskContanierMgr::_flushWeight()
   {
      UINT64 totalWeight = 0 ;

      MAP_CONTAINERPTR::iterator it = _mapContanier.begin() ;
      while( it != _mapContanier.end() )
      {
         totalWeight += it->second->getWeightValue() ;
         ++it ;
      }

      it = _mapContanier.begin() ;
      while( it != _mapContanier.end() )
      {
         it->second->updateFixedRatio( totalWeight ) ;
         ++it ;
      }
   }

   INT32 _schedTaskContanierMgr::addContanier( const string &name,
                                               INT32 nice,
                                               schedTaskContanierPtr &ptr )
   {
      INT32 rc = SDB_OK ;
      schedTaskContanier *pContainer = NULL ;

      pContainer = SDB_OSS_NEW schedTaskContanier() ;
      if ( !pContainer )
      {
         PD_LOG( PDERROR, "Allocate container failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = pContainer->init( name, _queType ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init container failed, rc: %d", rc ) ;
         goto error ;
      }
      pContainer->setNice( nice ) ;

      ptr = schedTaskContanierPtr( pContainer ) ;
      pContainer = NULL ;

      {
         ossScopedLock lock( &_latch ) ;

         MAP_CONTAINERPTR::iterator it = _mapContanier.find( name ) ;
         if ( it != _mapContanier.end() )
         {
            it->second->setNice( nice ) ;
         }
         else
         {
            _mapContanier[ name ] = ptr ;
         }

         _flushWeight() ;
      }

   done:
      if ( pContainer )
      {
         SDB_OSS_DEL pContainer ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _schedTaskContanierMgr::isContainerExist( const string &name )
   {
      MAP_CONTAINERPTR::iterator it ;

      ossScopedLock lock( &_latch ) ;

      it = _mapContanier.find( name ) ;
      if ( it != _mapContanier.end() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   INT32 _schedTaskContanierMgr::delContanier( const string &name )
   {
      schedTaskContanierPtr ptr ;
      BOOLEAN bFound = FALSE ;

      if ( name != _defaultPtr->getName() )
      {
         MAP_CONTAINERPTR::iterator it ;
         ossScopedLock lock( &_latch ) ;

         it = _mapContanier.find( name ) ;
         if ( it != _mapContanier.end() )
         {
            ptr = it->second ;
            bFound = TRUE ;
            _mapContanier.erase( it ) ;

            _updateIterator( ptr ) ;
            _flushWeight() ;
         }
      }

      if ( bFound )
      {
         pmdEDUEvent event ;

         while( ptr->hasHold() )
         {
            ossSleep( 100 ) ;
         }

         while( ptr->pop( event, 0 ) )
         {
            _defaultPtr->push( event, (INT64)pmdGetDBTick() ) ;
         }
      }

      return SDB_OK ;
   }

   schedTaskContanierPtr _schedTaskContanierMgr::getContanier( const string &name,
                                                               BOOLEAN withHold )
   {
      schedTaskContanierPtr ptr = _defaultPtr ;
      MAP_CONTAINERPTR::iterator it ;

      ossScopedLock lock( &_latch ) ;
      it = _mapContanier.find( name ) ;
      if ( it != _mapContanier.end() )
      {
         ptr = it->second ;
      }

      if ( withHold )
      {
         ptr->holdIn() ;
      }

      return ptr ;
   }

   void _schedTaskContanierMgr::_updateIterator( schedTaskContanierPtr &ptr )
   {
      MAP_CONTAINERPTR::iterator it ;

      if ( ptr.get() == _lastPtr.get() )
      {
         it = _mapContanier.upper_bound( _lastPtr->getName() ) ;

         if ( it == _mapContanier.end() )
         {
            _lastPtr = _mapContanier.begin()->second ;
         }
         else
         {
            _lastPtr = it->second ;
         }
      }

      if ( ptr.get() == _curPtr.get() )
      {
         it = _mapContanier.upper_bound( _curPtr->getName() ) ;

         if ( it == _mapContanier.end() )
         {
            _curPtr = _mapContanier.begin()->second ;
         }
         else
         {
            _curPtr = it->second ;
         }
      }
   }

   void _schedTaskContanierMgr::resumeIterator()
   {
      ossScopedLock lock( &_latch ) ;
      _curPtr = schedTaskContanierPtr() ;
   }

   void _schedTaskContanierMgr::pauseIterator( schedTaskContanierPtr ptr )
   {
      MAP_CONTAINERPTR::iterator it ;

      ossScopedLock lock( &_latch ) ;
      it = _mapContanier.upper_bound( ptr->getName() ) ;
      if ( it == _mapContanier.end() )
      {
         _lastPtr = _mapContanier.begin()->second ;
      }
      else
      {
         _lastPtr = it->second ;
      }
   }

   BOOLEAN _schedTaskContanierMgr::nextIterator( schedTaskContanierPtr &ptr )
   {
      BOOLEAN found = FALSE ;
      MAP_CONTAINERPTR::iterator it ;

      ossScopedLock lock( &_latch ) ;

      if ( !_curPtr.get() )
      {
         _curPtr = _lastPtr ;
         ptr = _curPtr ;
         found = TRUE ;
      }
      else
      {
         it = _mapContanier.upper_bound( _curPtr->getName() ) ;
         if ( it == _mapContanier.end() )
         {
            _curPtr = _mapContanier.begin()->second ;
         }
         else
         {
            _curPtr = it->second ;
         }

         if ( _curPtr.get() != _lastPtr.get() )
         {
            ptr = _curPtr ;
            found = TRUE ;
         }
      }

      return found ;
   }

}

