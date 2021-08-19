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

   Source File Name = monClass.cpp

   Descriptive Name = monitor Class source

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for OSS operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/24/2019  CW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "monClass.hpp"
#include "monCB.hpp"
#include "pmd.hpp"

namespace engine
{

// microsecond
#define MON_LATCH_ARCHIVE_THRESHOLD 1000

// microsecond
#define MON_LOCK_ARCHIVE_THRESHOLD 1000

archiveFunc monClassArchiveFP[MON_CLASS_MAX] = {
   monArchiveQuery,  // MON_CLASS_QUERY
   monArchiveLatch,  // MON_CLASS_LATCH
   monArchiveLock    // MON_CLASS_LOCK
};

MON_DATA_LEVEL monClassCreateCB[MON_CLASS_MAX] = {
   MON_DATA_LVL_BASIC,   // MON_CLASS_QUERY
   MON_DATA_LVL_DETAIL,  // MON_CLASS_LATCH
   MON_DATA_LVL_DETAIL   // MON_CLASS_LOCK
};

UINT32 monGetTID()
{
   pmdEDUCB *cb = pmdGetThreadEDUCB() ;
   if ( cb )
   {
      return cb->getTID() ;
   }

   return ossGetCurrentThreadID() ;
}

/**
 * _monClass constructor
 */
_monClass::_monClass()
   : _status(MON_CLASS_STATUS_NORMAL),
     _type(MON_CLASS_MAX),
     dataLvl( MON_DATA_LVL_NONE )
{
   _createTSTick.sample() ;
}

_monClass::~_monClass() {}

_monClassQueryTmpData& _monClassQueryTmpData::operator=(const _monAppCB& cb)
{
   dataRead  = cb.totalDataRead ;
   dataWrite = cb.totalDataWrite ;
   indexRead = cb.totalIndexRead ;
   indexWrite = cb.totalIndexWrite ;
   return *this ;
}

void _monClassQueryTmpData::diff(_monAppCB &cb)
{
   dataRead  = cb.totalDataRead - dataRead ;
   dataWrite = cb.totalDataWrite - dataWrite ;
   indexRead = cb.totalIndexRead - indexRead ;
   indexWrite = cb.totalIndexWrite - indexWrite ;
}


_monClassContainer::_monClassContainer ( MON_CLASS_TYPE type )
   : _archivedListMaxLen( pmdGetKRCB()->getOptionCB()->monHistEvent() ),
     _numPendingArchive( 0 ),
     _numPendingDelete( 0 ),
     _curCollectionLvl( MON_DATA_LVL_NONE )
{
   _doArchive = monClassArchiveFP[(INT32)type] ;
   _minOperationalLvl = monClassCreateCB[(INT32)type] ;
}

/**
 * Async remove an object from the active list. The
 * object is either marked as pending archive or pending delete
 * depending on the container's archive rule.
 *
 * @param obj object to be removed.
 */
void _monClassContainer::remove ( monClass *obj )
{
   ossGetCurrentTime( obj->getEndTS() ) ;

   if ( (*_doArchive)( obj ) )
   {
      obj->setPendingArchive() ;
      _numPendingArchive.inc() ;
   }
   else
   {
      obj->setPendingDelete() ;
      _numPendingDelete.inc() ;
   }
}

/**
 * Remove archived obj back to capacity
 * The archived list is structured as a MRU list with the tail being most
 * recent used. So elements are removed from the head which are the oldest
 */
void _monClassContainer::_removeArchivedObj()
{
   getArchiveLatch( EXCLUSIVE ) ;
   SINT32 numToDelete = 0 ;

   UINT32 archivedListSize = _archivedList.size() ;

   if ( !isOperational() )
   {
      numToDelete = archivedListSize ;
   }
   else
   {
      numToDelete = archivedListSize - _archivedListMaxLen ;
   }

   if ( numToDelete > 0 )
   {
      MONCLASS_LIST::iterator it = _archivedList.begin() ;

      while ( numToDelete > 0 )
      {
         monClass &monClass = *it ;

         it = _archivedList.erase(it) ;

         SDB_OSS_DEL &monClass ;

         numToDelete-- ;
      }
   }
   releaseArchiveLatch( EXCLUSIVE ) ;
}

/**
 * Process pending delete or pending archive object
 */
void _monClassContainer::_processPendingObj()
{
   getArchiveLatch( EXCLUSIVE ) ;

   MON_PARTITION_LIST::iterator it = _activeList.begin() ;

   while ( it != _activeList.end() )
   {
      if ( TRUE == it->isPendingArchive() )
      {
         monClass &obj = *it ;
         it = _activeList.erase(it) ;
         _numPendingArchive.dec() ;
         _archivedList.push_back(obj) ;
      }
      else if ( TRUE == it->isPendingDelete() )
      {
         monClass &monClass = *it ;
         it = _activeList.erase(it) ;
         SDB_OSS_DEL &monClass ;
         _numPendingDelete.dec() ;
      }
      else
      {
         it++ ;
      }
   }

   releaseArchiveLatch( EXCLUSIVE ) ;
}

/**
 * archive query based on response time
 */
BOOLEAN monArchiveQuery ( monClass *obj )
{
   monClassQuery *monQuery = (monClassQuery*)obj ;
   // in microsecond
   UINT64 responseTime = monQuery->responseTime.toUINT64() ;

   if ((responseTime/1000) >= pmdGetKRCB()->getOptionCB()->slowQueryThreshold())
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

/**
 * archive latch when there is latch wait
 */
BOOLEAN monArchiveLatch ( monClass *obj )
{
   monClassLatch *monLatch = (monClassLatch*)obj ;
   // in microsecond
   UINT64 waitTime = monLatch->waitTime.toUINT64() ;;

   if ( waitTime > MON_LATCH_ARCHIVE_THRESHOLD )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

/**
 * archive lock when there is lock wait
 */
BOOLEAN monArchiveLock ( monClass *obj )
{
   monClassLock *monLock = (monClassLock*)obj ;
   // in microsecond
   UINT64 waitTime = monLock->waitTime.toUINT64() ;

   if ( waitTime > MON_LOCK_ARCHIVE_THRESHOLD )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

BOOLEAN monNoArchive ( monClass *obj )
{
   return FALSE ;
}

// monClassScanner implements

void _monClassReadScanner::initScan()
{
   if ( ( _listType == MON_CLASS_ACTIVE_LIST ) &&
        ( _container->getActiveListLen() ) )
   {
      _itr = _container->begin( _listType ) ;
      _endReached = ( _itr == _container->end( _listType ) ) ? TRUE : FALSE ;
   }
   else if ( _listType == MON_CLASS_ARCHIVED_LIST )
   {
      if ( _container->getArchivedListLen() ||
           _container->getNumPendingArchive() )
      {
         _container->getArchiveLatch( SHARED ) ;
         _hasArchiveLatch = TRUE ;
         _itr = _container->begin( _listType ) ;
      }
      else
      {
         _endReached = TRUE ;
      }
   }
   else
   {
      _endReached = TRUE ;
   }
   _initCalled = TRUE ;
}

monClass* _monClassReadScanner::getNext()
{
   monClass *ret = NULL ;
   BOOLEAN found = FALSE ;
   if ( !_initCalled )
   {
      initScan() ;
   }

   // If we have not reached the end of the lists
   while (!_endReached && !found)
   {
      // 1. We skip any pending deletes
      // 2. Skip if this is a pending archive and we are only interested in
      //    the active list
      // 3. Skip if this is active and we are only interested in the
      //    archived list
      if ( _itr->isPendingDelete() ||
           (_itr->isPendingArchive() && _listType == MON_CLASS_ACTIVE_LIST ) ||
           (!_itr->isPendingArchive() && _listType == MON_CLASS_ARCHIVED_LIST ) )
      {
         _itr++ ;
      }
      else
      {
         // Found a match, return
         ret = &( *_itr ) ;
         ++_itr ;
         found = TRUE ;
      }

      // Reached the end
      if ( _itr == _container->end(_listType) )
      {
         _endReached = TRUE ;
      }
   }

   return ret ;
}
} // namespace engine
