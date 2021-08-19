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

   Source File Name = catDCLogMgr.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     XJH Opt

*******************************************************************************/


#include "catCommon.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "catDCLogMgr.hpp"
#include "dmsCB.hpp"
#include "rtnCB.hpp"
#include "dpsLogWrapper.hpp"
#include "catalogueCB.hpp"
#include "dpsOp2Record.hpp"
#include "authDef.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _catDCLogItem implement
   */
   _catDCLogItem::_catDCLogItem( UINT32 pos, const string &clname )
   {
      pmdKRCB *krcb     = pmdGetKRCB() ;
      _pDmsCB           = krcb->getDMSCB() ;
      _pDpsCB           = krcb->getDPSCB() ;
      _pRtnCB           = krcb->getRTNCB() ;
      _pCatCB           = krcb->getCATLOGUECB() ;

      _pos              = pos ;
      _clName           = clname ;
      _clLID            = DMS_INVALID_CLID ;

      _reset() ;
   }

   _catDCLogItem::~_catDCLogItem()
   {
   }

   string _catDCLogItem::toString() const
   {
      // name + count + first lsn + last lsn
      stringstream ss ;
      ss << "Name:" << _clName << ", Count:" << getCount()
         << ", First lsn:" << _first.version << "." << (INT64)_first.offset
         << ", Last lsn:" << _last.version << "." << (INT64)_last.offset
         << ", Coming lsn:" << _coming.version << "." << (INT64)_coming.offset ;
      return ss.str() ;
   }

   void _catDCLogItem::_reset()
   {
      _first = DPS_LSN() ;
      _last  = DPS_LSN() ;
      _coming= DPS_LSN() ;
      _count = 0 ;
   }

   INT32 _catDCLogItem::_parseMeta( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      const CHAR *pCLShortName = NULL ;
      dmsMBContext *mbContext = NULL ;

      // get logical id and count
      rc = rtnResolveCollectionNameAndLock( _clName.c_str(),
                                            _pDmsCB,
                                            &su,
                                            &pCLShortName,
                                            suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Reslove collection name[%s] failed, rc: %d",
                   toString().c_str(), rc ) ;

      rc = su->data()->getMBContext( &mbContext, pCLShortName, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "Get mb context failed, rc: %d", rc ) ;

      _clLID = mbContext->clLID() ;
      _count = mbContext->mbStat()->_totalRecords ;

   done:
      if ( mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( suID != DMS_INVALID_SUID )
      {
         _pDmsCB->suUnlock( suID ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogItem::restore( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj orderByFirst = BSON( FIELD_NAME_LSN_OFFSET << 1 ) ;
      BSONObj orderByLast = BSON( FIELD_NAME_LSN_OFFSET << -1 ) ;

      // first reset
      _reset() ;

      rc = _parseMeta( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse system log[%s] meta failed, rc: %d",
                   toString().c_str(), rc ) ;

      // get the first lsn
      rc = _parseLsn( orderByFirst, cb, _first ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse first lsn failed, rc: %d", rc ) ;

      // get the last lsn
      rc = _parseLsn( orderByLast, cb, _last ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse last lsn failed, rc: %d", rc ) ;

      // if the lsn is not continuous, need to drop the collection
      if ( ( _first.invalid() && ( !_last.invalid() || 0 != _count ) ) ||
           ( _last.invalid() && ( !_first.invalid() || 0 != _count ) ) ||
           ( !_first.invalid() && !_last.invalid() &&
             _count != _last.offset - _first.offset + 1 ) )
      {
         PD_LOG( PDWARNING, "System log[%s] data corrupted, trucanted",
                 toString().c_str() ) ;
         rc = truncate( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Truncate system log[%s] failed, rc: %d",
                      toString().c_str(), rc ) ;
      }

      if ( !_last.invalid() )
      {
         _coming.version = _last.version ;
         _coming.offset  = _last.offset + 1 ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogItem::_parseLsn( const BSONObj &orderby,
                                   _pmdEDUCB *cb,
                                   DPS_LSN &lsn )
   {
      INT32 rc = SDB_OK ;
      BSONObj hint ;
      INT64 contextID = -1 ;
      rtnContextBuf buffObj ;

      rc = rtnQuery( _clName.c_str(), hint, hint, orderby, hint, 0, cb,
                     0, 1, _pDmsCB, _pRtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, rc: %d",
                   toString().c_str(), rc ) ;
      rc = rtnGetMore( contextID, -1, buffObj, cb, _pRtnCB ) ;
      if( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Get more collection[%s] failed, rc: %d",
                 toString().c_str(), rc ) ;
         goto error ;
      }

      try
      {
         BSONObj obj( buffObj.data() ) ;
         BSONElement eVer = obj.getField( FIELD_NAME_LSN_VERSION ) ;
         BSONElement eOff = obj.getField( FIELD_NAME_LSN_OFFSET ) ;
         if ( NumberInt != eVer.type() || NumberLong != eOff.type() )
         {
            PD_LOG( PDERROR, "Version or Offset is invalid in obj[%s]",
                    obj.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         lsn.version = ( DPS_LSN_VER )eVer.numberInt() ;
         lsn.offset = ( DPS_LSN_OFFSET )eOff.numberLong() ;         
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse lsn occur expection: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // kill context
      _pRtnCB->contextDelete( contextID, cb ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogItem::truncate( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      rc = rtnTruncCollectionCommand( _clName.c_str(), cb, _pDmsCB, _pDpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate system log[%s] failed, rc: %d",
                   toString().c_str(), rc ) ;

      _reset() ;

      // re-parse meta
      rc = _parseMeta( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse system log[%s] meta info failed, "
                   "rc: %d", toString().c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogItem::writeData( BSONObj &obj, const DPS_LSN &lsn,
                                   _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( !_coming.invalid() )
      {
         SDB_ASSERT( _coming.offset == lsn.offset, "Write lsn is not the "
                     "same with coming lsn" ) ;
         if ( _coming.offset != lsn.offset )
         {
            PD_LOG( PDERROR, "Write lsn[%d.%lld] is invalid in system log[%s]",
                    lsn.version, lsn.offset, toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      rc = rtnInsert( _clName.c_str(), obj, 1, 0, cb, _pDmsCB, _pDpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed insert obj[%s] to system log[%s], "
                   "rc: %d", obj.toString().c_str(), toString().c_str(),
                   rc ) ;

      ++_count ;

      if ( _first.invalid() )
      {
         _first = lsn ;
         _last = lsn ;
      }
      else
      {
         _last = lsn ;
      }
      _coming.version = lsn.version ;
      _coming.offset = _last.offset + 1 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogItem::readData( const BSONObj &match,
                                  _dpsMessageBlock *mb,
                                  _pmdEDUCB *cb,
                                  const BSONObj &orderby,
                                  INT64 limit,
                                  INT32 maxTime,
                                  INT32 maxSize )
   {
      INT32 rc = SDB_OK ;
      BSONObj hint ;
      INT64 contextID = -1 ;
      rtnContextBuf buffObj ;
      UINT64 bTime = 0 ;

      if ( maxTime > 0 )
      {
         bTime = ( UINT64 )time( NULL ) ;
      }

      rc = rtnQuery( _clName.c_str(), hint, match, orderby, hint, 0, cb,
                     0, limit, _pDmsCB, _pRtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query system log[%s] by matcher[%s] failed, "
                   "rc: %d", toString().c_str(), match.toString().c_str(),
                   rc ) ;

      while( TRUE )
      {
         rc = rtnGetMore( contextID, 1, buffObj, cb, _pRtnCB ) ;
         if( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            contextID = -1 ;
            PD_LOG( PDERROR, "Get more from system log[%s] failed, rc: %d",
                    toString().c_str(), rc ) ;
            goto error ;
         }

         // add objs to mb block
         if( mb->idleSize() < (UINT32)buffObj.size() )
         {
            rc = mb->extend( buffObj.size() - mb->idleSize() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to extend mb, rc: %d", rc ) ;
         }

         // copy
         ossMemcpy( mb->writePtr(), buffObj.data(), buffObj.size() ) ;
         mb->writePtr( mb->length() + buffObj.size() ) ;

         if ( maxSize > 0 )
         {
            maxSize = maxSize > buffObj.size() ? maxSize - buffObj.size() : 0 ;
         }

         /// max size check
         if ( 0 == maxSize )
         {
            break ;
         }
         /// max time check
         if ( maxTime > 0 && time( NULL ) - bTime >= (UINT64)maxTime )
         {
            break ;
         }
      }

      if( 0 == mb->length() )
      {
         PD_LOG( PDERROR, "Can't find obj[%s] in system log[%s]",
                 match.toString().c_str(), toString().c_str() ) ;
         rc = SDB_DPS_LOG_NOT_IN_FILE ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         _pRtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogItem::removeDataToLow( DPS_LSN_OFFSET lowOffset,
                                         _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj hint ;
      utilDeleteResult delResult ;
      BSONObj matcher = BSON( FIELD_NAME_LSN_OFFSET << BSON(
                              "$gte" << (INT64)lowOffset ) ) ;
      BSONObj orderByFirst = BSON( FIELD_NAME_LSN_OFFSET << 1 ) ;
      BSONObj orderByLast = BSON( FIELD_NAME_LSN_OFFSET << -1 ) ;

      rc = rtnDelete( _clName.c_str(), matcher, hint, 0, cb, _pDmsCB,
                      _pDpsCB, 1, &delResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Delete objs[%s] from system log[%s] failed, "
                   "del num: %llu, rc: %d", matcher.toString().c_str(),
                   _clName.c_str(), delResult.deletedNum(), rc ) ;

      _reset() ;

      // parse meta
      rc = _parseMeta( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse system log[%s] meta failed, rc: %d",
                   toString().c_str(), rc ) ;

      // get the first lsn
      rc = _parseLsn( orderByFirst, cb, _first ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse first lsn failed, rc: %d", rc ) ;

      // get the last lsn
      rc = _parseLsn( orderByLast, cb, _last ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse last lsn failed, rc: %d", rc ) ;

      if ( !_last.invalid() )
      {
         _coming.version = _last.version ;
         _coming.offset  = _last.offset + 1 ;

         SDB_ASSERT( _coming.offset == lowOffset, "Coming offset invalid" ) ;
         SDB_ASSERT( _count == _last.offset - _first.offset + 1,
                     "last - first + 1 is not same with count" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT64 _catDCLogItem::getCount() const
   {
      return _count ;
   }

   UINT32 _catDCLogItem::getLID() const
   {
      return _clLID ;
   }

   BOOLEAN _catDCLogItem::isFull() const
   {
      if ( 0 == _coming.offset % CAT_SYSLOG_CL_MAX_COUNT &&
           ( _coming.offset / CAT_SYSLOG_CL_MAX_COUNT ) %
           CAT_SYSLOG_CL_NUM != _pos )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _catDCLogItem::isEmpty() const
   {
      if ( _coming.invalid() ||
           ( 0 == _coming.offset % CAT_SYSLOG_CL_MAX_COUNT &&
            ( _coming.offset / CAT_SYSLOG_CL_MAX_COUNT ) %
              CAT_SYSLOG_CL_NUM == _pos ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   DPS_LSN _catDCLogItem::getFirstLSN() const
   {
      return _first ;
   }

   DPS_LSN _catDCLogItem::getLastLSN() const
   {
      return _last ;
   }

   DPS_LSN _catDCLogItem::getComingLSN() const
   {
      return _coming ;
   }

   /*
      _catDCLogMgr implement
   */
   _catDCLogMgr::_catDCLogMgr()
   {
      _pEduCB     = NULL ;
      _begin      = 0 ;
      _work       = 0 ;
      _expectLsn.version = 0 ;
      _expectLsn.offset = 0 ;
   }

   _catDCLogMgr::~_catDCLogMgr()
   {
      for ( UINT32 i = 0 ; i < _vecLogCL.size() ; ++i )
      {
         SDB_OSS_DEL _vecLogCL[ i ] ;
      }
      _vecLogCL.clear() ;
   }

   void _catDCLogMgr::attachCB( pmdEDUCB * cb )
   {
      _pEduCB = cb ;
   }

   void _catDCLogMgr::detachCB( pmdEDUCB * cb )
   {
      _pEduCB = NULL ;
   }

   INT32 _catDCLogMgr::init()
   {
      INT32 rc          = SDB_OK ;
      catDCLogItem *pLog= NULL ;
      CHAR clName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;

      for ( UINT32 i = 0 ; i < CAT_SYSLOG_CL_NUM ; ++i )
      {
         ossSnprintf( clName, sizeof( clName ), "%s%d",
                      CAT_SYSLOG_COLLECTION_NAME, i ) ;
         pLog = SDB_OSS_NEW catDCLogItem( i, clName ) ;
         if ( !pLog )
         {
            PD_LOG( PDERROR, "Alloc dc log item[%s] failed", clName ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _vecLogCL.push_back( pLog ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogMgr::restore()
   {
      return SDB_OK ;
/*
TODO:XUJIANHUI
Begin Fobidden DC
      INT32 rc = SDB_OK ;
      catDCLogItem *pLog = NULL ;
      UINT32 i = 0 ;
      UINT32 tmpWork = 0 ;
      DPS_LSN minLsn ;
      DPS_LSN firstLsn ;
      DPS_LSN comingLsn ;

      for ( i = 0 ; i < _vecLogCL.size() ; ++i )
      {
         pLog = _vecLogCL[ i ] ;
         rc = pLog->restore( _pEduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Analysis collection[%s] failed, rc: %d",
                    pLog->getCLName(), rc ) ;
            goto error ;
         }
      }

      // analyse the current work position
      // 1. find the begin
      for ( i = 0 ; i < _vecLogCL.size() ; ++i )
      {
         pLog = _vecLogCL[ i ] ;
         firstLsn = pLog->getFirstLSN() ;
         if ( !firstLsn.invalid() && ( minLsn.invalid() ||
              firstLsn.compareOffset( minLsn.offset ) < 0 ) )
         {
            _begin = i ;
            minLsn = firstLsn ;
         }
      }
      // 2. find the work
      tmpWork = _begin ;
      comingLsn = minLsn ;
      for ( i = 0 ; i < _vecLogCL.size() ; ++i )
      {
         pLog = _vecLogCL[ tmpWork ] ;
         firstLsn = pLog->getFirstLSN() ;
         if ( 0 != firstLsn.compareOffset( comingLsn.offset ) ||
              !pLog->isFull() )
         {
            ++i ;
            break ;
         }
         comingLsn = pLog->getComingLSN() ;
         tmpWork = _incFileID( tmpWork ) ;
      }
      _work = tmpWork ;
      pLog = _vecLogCL[ _work ] ;
      if ( !pLog->getLastLSN().invalid() )
      {
         _curLsn = pLog->getLastLSN() ;
         _expectLsn = pLog->getComingLSN() ;
      }

      // 3. reset others
      for ( ; i < _vecLogCL.size() ; ++i )
      {
         tmpWork = _incFileID( tmpWork ) ;
         pLog = _vecLogCL[ tmpWork ] ;
         firstLsn = pLog->getFirstLSN() ;
         if ( !firstLsn.invalid() )
         {
            PD_LOG( PDWARNING, "Truncate system log[%s]",
                    pLog->toString().c_str() ) ;
            rc = pLog->truncate( _pEduCB ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Truncate system log[%s] failed, rc: %d",
                       pLog->toString().c_str(), rc ) ;
               goto error ;
            }
         }
      }

      PD_LOG( PDEVENT, "Analysis system log, begin:[%s], work:[%s],"
              "curLsn: %d.%lld, expectLsn: %d.%lld",
              _vecLogCL[ _begin ]->toString().c_str(),
              _vecLogCL[ _work ]->toString().c_str(),
              _curLsn.version, _curLsn.offset,
              _expectLsn.version, _expectLsn.offset ) ;

   done:
      return rc ;
   error:
      goto done ;
END
*/
   }

   INT32 _catDCLogMgr::search( const DPS_LSN &minLsn,
                               _dpsMessageBlock *mb,
                               UINT8 type,
                               INT32 maxNum,
                               INT32 maxTime,
                               INT32 maxSize )
   {
      INT32 rc = SDB_OK ;
      UINT32 pos = 0 ;
      BOOLEAN hasLock = FALSE ;
      DPS_LSN begin ;
      BSONObj matcher = BSON( FIELD_NAME_LSN_OFFSET <<
                              BSON( "$gte" << (INT64)minLsn.offset )
                            ) ;

      if ( DPS_INVALID_LSN_OFFSET == minLsn.offset )
      {
         rc = SDB_DPS_LOG_NOT_IN_FILE ;
         goto error ;
      }

      pos = minLsn.offset % CAT_SYSLOG_CL_MAX_COUNT ;

      _latch.get_shared() ;
      hasLock = TRUE ;

      begin = _getStartLsn() ;
      if ( begin.invalid() )
      {
         PD_LOG( PDERROR, "begin lsn invalid [offset:%lld] [version:%d]",
                 begin.offset, begin.version ) ;
         rc = SDB_DPS_LOG_NOT_IN_FILE ;
         goto error ;
      }
      if ( minLsn.compareOffset( begin.offset ) < 0 )
      {
         PD_LOG( PDDEBUG, "lsn %lld is smaller than membegin %lld",
                 minLsn.offset, begin.offset ) ;
         rc = SDB_DPS_LOG_NOT_IN_FILE ;
         goto error ;
      }
      if ( minLsn.compareOffset( _expectLsn.offset ) >= 0 )
      {
         rc = SDB_DPS_LSN_OUTOFRANGE ;
         goto error ;
      }

      rc = _readData( matcher, _vecLogCL[ pos ], mb, BSONObj(),
                      (INT64)maxNum, maxTime, maxSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Read data by matcher[%s] failed, rc: %d",
                   matcher.toString().c_str(), rc ) ;

   done:
      if ( hasLock )
      {
         _latch.release_shared() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogMgr::searchHeader( const DPS_LSN &lsn,
                                     _dpsMessageBlock *mb,
                                     UINT8 type )
   {
      return search( lsn, mb, type ) ;
   }

   DPS_LSN _catDCLogMgr::getStartLsn( BOOLEAN logBufOnly )
   {
      ossScopedLock lock( &_latch, SHARED ) ;
      return _getStartLsn() ;
   }

   DPS_LSN _catDCLogMgr::getCurrentLsn()
   {
      ossScopedLock lock( &_latch, SHARED ) ;
      return _curLsn ;
   }

   DPS_LSN _catDCLogMgr::expectLsn()
   {
      ossScopedLock lock( &_latch, SHARED ) ;
      return _expectLsn ;
   }

   DPS_LSN _catDCLogMgr::commitLsn()
   {
      return DPS_LSN() ;
   }

   void _catDCLogMgr::getLsnWindow( DPS_LSN &beginLsn,
                                    DPS_LSN &endLsn,
                                    DPS_LSN *expected,
                                    DPS_LSN *committed )
   {
      ossScopedLock lock( &_latch, SHARED ) ;
      beginLsn = _getStartLsn() ;
      endLsn = _curLsn ;
      if ( NULL != expected )
      {
         *expected = _expectLsn ;
      }

      /// no committed
      return ;
   }

   void _catDCLogMgr::getLsnWindow( DPS_LSN &fileBeginLsn,
                                    DPS_LSN &memBeginLsn,
                                    DPS_LSN &endLsn,
                                    DPS_LSN *pExpectLsn,
                                    DPS_LSN *committed )
   {
      ossScopedLock lock( &_latch, SHARED ) ;
      fileBeginLsn = _getStartLsn() ;
      memBeginLsn = fileBeginLsn ;
      endLsn = _curLsn ;
      if ( pExpectLsn )
      {
         *pExpectLsn = _expectLsn ;
      }
      /// no committed
      return ;
   }

   INT32 _catDCLogMgr::move( const DPS_LSN_OFFSET &offset,
                             const DPS_LSN_VER &version )
   {
      INT32 rc = SDB_OK ;
      DPS_LSN begin ;
      BOOLEAN hasLock = FALSE ;
      UINT32 pos = ( offset / CAT_SYSLOG_CL_MAX_COUNT ) % CAT_SYSLOG_CL_NUM ;

      if ( DPS_INVALID_LSN_OFFSET == offset )
      {
         rc = SDB_DPS_MOVE_FAILED ;
         PD_LOG( PDERROR, "can not move to a invalid lsn" ) ;
         goto error ;
      }

      _latch.get() ;
      hasLock = TRUE ;

      begin = _getStartLsn() ;

      // out of all range
      if ( _curLsn.invalid() || offset < begin.offset ||
           offset > _expectLsn.offset )
      {
         for ( UINT32 i = 0 ; i < _vecLogCL.size() ; ++i )
         {
            rc = _vecLogCL[ i ]->truncate( _pEduCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Truncate system log[%s] failed, "
                         "rc: %d", _vecLogCL[ i ]->toString().c_str(),
                         rc ) ;
         }
         _work = pos ;
         _begin = _work ;
         _curLsn.offset = DPS_INVALID_LSN_OFFSET ;
         _curLsn.version = DPS_INVALID_LSN_VERSION ;
      }
      else if ( offset < _expectLsn.offset )
      {
         while ( _work != pos )
         {
            rc = _vecLogCL[ _work ]->truncate( _pEduCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Truncate system log[%s] failed, "
                         "rc: %d", _vecLogCL[ _work ]->toString().c_str(),
                         rc ) ;
            _work = _decFileID( _work ) ;
         }
         // remove current collection
         rc = _vecLogCL[ _work ]->removeDataToLow( offset, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Remove system log[%s] by to low "
                      "offset[%lld] failed, rc: %d",
                      _vecLogCL[ _work ]->toString().c_str(),
                      offset, rc ) ;

         if ( _vecLogCL[ _work ]->isEmpty() && _begin != _work )
         {
            _work = _decFileID( _work ) ;
         }
         _curLsn = _vecLogCL[ _work ]->getLastLSN() ;
      }

      _expectLsn.offset = offset ;
      _expectLsn.version = version ;

   done:
      if ( hasLock )
      {
         _latch.release() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogMgr::recordRow( const CHAR *row, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      DPS_LSN lsn ;

      ossScopedLock lock( &_latch, EXCLUSIVE ) ;

      try
      {
         dpsLogRecordHeader *pHeader = NULL ;
         BSONObj obj( row ) ;
         BSONElement eVer = obj.getField( FIELD_NAME_LSN_VERSION ) ;
         BSONElement eOff = obj.getField( FIELD_NAME_LSN_OFFSET ) ;
         BSONElement eData= obj.getField( FIELD_NAME_DATA ) ;

         if ( NumberInt != eVer.type() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in record row[%s]",
                    FIELD_NAME_LSN_VERSION, obj.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         lsn.version = ( DPS_LSN_VER )eVer.numberInt() ;

         if ( NumberLong != eOff.type() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in record row[%s]",
                    FIELD_NAME_LSN_OFFSET, obj.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         lsn.offset = ( DPS_LSN_OFFSET )eOff.numberLong() ;

         if ( BinData != eData.type() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in record row[%s]",
                    FIELD_NAME_DATA, obj.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         pHeader = ( dpsLogRecordHeader* )eData.value() ;
         SDB_ASSERT( lsn.version == pHeader->_version, "Version invalid" ) ;
         SDB_ASSERT( lsn.offset == pHeader->_lsn, "Offset invalid" ) ;

         if ( lsn.offset != _expectLsn.offset )
         {
            PD_LOG( PDERROR, "lsn[%lld] of row is not equal to lsn[%lld] of "
                    "local", eOff.numberInt(), _expectLsn.offset) ;
            rc = SDB_SYS ;
            goto error ;
         }

         rc = _writeData( obj, lsn ) ;
         PD_RC_CHECK( rc, PDERROR, "Write obj[%s] to system log failed, "
                      "rc: %d", obj.toString().c_str(), rc ) ;

         _expectLsn.version = lsn.version ;
         _curLsn = _expectLsn ;
         _expectLsn.offset += 1 ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse row data occur expection: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogMgr::_writeData( BSONObj &obj, const DPS_LSN &lsn )
   {
      INT32 rc             = SDB_OK ;
      catDCLogItem *pLog   = NULL ;
      UINT32 pos           = _work ;

      // write to collection
      pLog = _vecLogCL[ pos ] ;
      if ( pLog->isFull() )
      {
         pos = _incFileID( pos ) ;
         pLog = _vecLogCL[ pos ] ;

         if ( !pLog->isEmpty() )
         {
            rc = pLog->truncate( _pEduCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Truncate system log[%s] failed, "
                         "rc: %d", pLog->toString().c_str(), rc ) ;
         }
      }

      rc = pLog->writeData( obj, lsn, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Write obj[%s] to system log[%s] "
                   "failed, rc: %d", obj.toString().c_str(),
                   pLog->toString().c_str(), rc ) ;

      _work = pos ;
      if ( _begin == _work )
      {
         _begin = _incFileID( _begin ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogMgr::_readData( const BSONObj &match,
                                  catDCLogItem *pLog,
                                  _dpsMessageBlock *mb,
                                  const BSONObj &orderby,
                                  INT64 limit,
                                  INT32 maxTime,
                                  INT32 maxSize )
   {
      return pLog->readData( match, mb, _pEduCB, orderby, limit,
                             maxTime, maxSize ) ;
   }

   DPS_LSN _catDCLogMgr::_getStartLsn()
   {
      catDCLogItem *pLog = _vecLogCL[ _begin ]  ;
      return pLog->getFirstLSN() ;
   }

   INT32 _catDCLogMgr::_filterLog( dpsLogRecordHeader *pHeader,
                                   BOOLEAN &valid )
   {
      INT32 rc = SDB_OK ;
      valid = FALSE ;

      static const CHAR *s_FilterCLs[] = {
         CAT_COLLECTION_SPACE_COLLECTION,
         CAT_COLLECTION_INFO_COLLECTION,
         CAT_TASK_INFO_COLLECTION,
         CAT_DOMAIN_COLLECTION,
         CAT_HISTORY_COLLECTION,
         CAT_PROCEDURES_COLLECTION,
         AUTH_USR_COLLECTION
      } ;
      static UINT32 s_Size = sizeof( s_FilterCLs ) / sizeof( const CHAR* ) ;

      const CHAR *pFullName = NULL ;
      BSONObj obj ;
      BSONObj oldMatch, oldObj, newMatch ;

      switch( pHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
            rc = dpsRecord2Insert( (const CHAR *)pHeader, &pFullName, obj ) ;
            break ;
         case LOG_TYPE_DATA_UPDATE :
            rc = dpsRecord2Update( (const CHAR *)pHeader, &pFullName,
                                   oldMatch, oldObj, newMatch, obj ) ;
            break ;
         case LOG_TYPE_DATA_DELETE :
            rc = dpsRecord2Delete( (const CHAR *)pHeader, &pFullName, obj ) ;
            break;
         default :
            goto done ; /// don't care
      }

      PD_RC_CHECK( rc, PDERROR, "Parse log failed, rc: %d", rc ) ;

      for ( UINT32 i = 0 ; i < s_Size ; ++i )
      {
         if ( 0 == ossStrcmp( pFullName, s_FilterCLs[ i ] ) )
         {
            valid = TRUE ;
            break ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCLogMgr::saveSysLog( dpsLogRecordHeader *pHeader,
                                   DPS_LSN *pRetLSN )
   {
      return SDB_OK ;
/*
TODO:XUJIANHUI
Begin Fobidden DC
      INT32 rc = SDB_OK ;
      BOOLEAN valid = FALSE ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      DPS_LSN_OFFSET orgOffset = pHeader->_lsn ;

      /// filter log
      rc = _filterLog( pHeader, valid ) ;
      PD_RC_CHECK( rc, PDERROR, "Filter log failed, rc: %d", rc ) ;

      if ( !valid )
      {
         /// don't save the log
         goto done ;
      }
      else
      {
         ossScopedLock lock( &_latch, EXCLUSIVE ) ;

         _setVersion( pHeader->_version ) ;

         if ( DPS_INVALID_LSN_VERSION == _expectLsn.version )
         {
            ++_expectLsn.version ;
         }

         pHeader->_preLsn = _curLsn.offset ;
         pHeader->_lsn = _expectLsn.offset ;
         pHeader->_version = _expectLsn.version ;

         builder.append( FIELD_NAME_LSN_VERSION, (INT32)_expectLsn.version ) ;
         builder.append( FIELD_NAME_LSN_OFFSET, (INT64)_expectLsn.offset ) ;
         builder.append( FIELD_NAME_ORG_LSNOFFSET, (INT64)orgOffset ) ;
         builder.append( FIELD_NAME_DATALEN, (INT32)pHeader->_length ) ;
         builder.appendBinData( FIELD_NAME_DATA, (INT32)pHeader->_length,
                                BinDataGeneral,
                                ( const unsigned char *)pHeader ) ;

         obj = builder.obj() ;
         rc = _writeData( obj, _expectLsn ) ;
         PD_RC_CHECK( rc, PDERROR, "Write obj[%s] to system log failed, "
                      "rc: %d", obj.toString().c_str(), rc ) ;

         _curLsn = _expectLsn ;
         _expectLsn.offset += 1 ;
      }

   done:
      return rc ;
   error:
      goto done ;
END
*/
   }

   UINT32 _catDCLogMgr::_incFileID ( UINT32 fileID )
   {
      ++fileID ;
      if ( fileID >= CAT_SYSLOG_CL_NUM )
      {
         fileID = 0 ;
      }

      return fileID ;
   }

   UINT32 _catDCLogMgr::_decFileID ( UINT32 fileID )
   {
      if ( 0 == fileID )
      {
         fileID = CAT_SYSLOG_CL_NUM - 1 ;
      }
      else
      {
         --fileID ;
      }

      return fileID ;
   }

   void _catDCLogMgr::_setVersion( DPS_LSN_VER version )
   {
      _expectLsn.version = version ;
   }

}


