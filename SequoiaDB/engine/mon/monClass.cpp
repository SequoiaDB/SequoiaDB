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
   dataRead      = cb.totalDataRead ;
   dataWrite     = cb.totalDataWrite ;
   indexRead     = cb.totalIndexRead ;
   indexWrite    = cb.totalIndexWrite ;
   lobRead       = cb.totalLobRead ;
   lobWrite      = cb.totalLobWrite ;
   lobTruncate   = cb.totalLobTruncate ;
   lobAddressing = cb.totalLobAddressing ;

   return *this ;
}

void _monClassQueryTmpData::diff(_monAppCB &cb)
{
   dataRead      = MON_APP_DELTA( dataRead , cb.totalDataRead ) ;
   dataWrite     = MON_APP_DELTA( dataWrite , cb.totalDataWrite ) ;
   indexRead     = MON_APP_DELTA( indexRead , cb.totalIndexRead ) ;
   indexWrite    = MON_APP_DELTA( indexWrite , cb.totalIndexWrite ) ;
   lobRead       = MON_APP_DELTA( lobRead , cb.totalLobRead ) ;
   lobWrite      = MON_APP_DELTA( lobWrite , cb.totalLobWrite ) ;
   lobTruncate   = MON_APP_DELTA( lobTruncate , cb.totalLobTruncate ) ;
   lobAddressing = MON_APP_DELTA( lobAddressing , cb.totalLobAddressing ) ;
}

_monClassContainer::_monClassContainer ( MON_CLASS_TYPE type )
   : _archivedListMaxLen( pmdGetKRCB()->getOptionCB()->monHistEvent() ),
     _numPendingArchive( 0 ),
     _numPendingDelete( 0 ),
     _curCollectionLvl( MON_DATA_LVL_NONE ),
     _classType( type )
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
      getArchiveLatch( EXCLUSIVE ) ;
      MONCLASS_LIST::iterator it = _archivedList.begin() ;

      while ( numToDelete > 0 )
      {
         monClass &monClass = *it ;

         it = _archivedList.erase(it) ;

         SDB_OSS_DEL &monClass ;

         numToDelete-- ;
      }
      releaseArchiveLatch( EXCLUSIVE ) ;
   }
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

/*
 * return whether this container is empty for the correspond list
 * on the listType.
 * @param listType the type of list to read
 */
BOOLEAN _monClassContainer::isEmpty(MON_CLASS_LIST_TYPE listType)
{
   return ( listType == MON_CLASS_ACTIVE_LIST ) ?
          !this->getActiveListLen() : !this->getArchivedListLen() ;
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
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      if ( cb )
      {
         DMS_MON_OP_COUNT_INC( cb->getMonAppCB(), MON_GENERAL_SLOW_QUERY, 1 ) ;
      }

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

} // namespace engine
