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

   Source File Name = coordSequenceAgent.cpp

   Descriptive Name = Coordinator Sequence Agent

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordSequenceAgent.hpp"
#include "coordCommandSequence.hpp"
#include "coordRemoteSession.hpp"
#include "coordCB.hpp"
#include "catGTSDef.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "msgDef.h"
#include "pmdEDU.hpp"
#include "ossLatch.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{
   class _coordSequence: public SDBObject
   {
   public:
      _coordSequence( const std::string& name )
      {
         _name = name ;
         _nextValue = 0 ;
         _acquireSize = 0 ;
         _increment = 0 ;
         _ID = UTIL_SEQUENCEID_NULL ;
      }
      ~_coordSequence() {}

   public:
      OSS_INLINE const std::string& name() const { return _name ; }
      OSS_INLINE const utilSequenceID ID() const { return _ID ; }
      OSS_INLINE INT64 nextValue() const { return _nextValue ; }
      OSS_INLINE INT32 acquireSize() const { return _acquireSize ; }
      OSS_INLINE INT32 increment() const { return _increment ; }

      OSS_INLINE void setID( const utilSequenceID ID )
      {
         _ID = ID ;
      }
      OSS_INLINE void setNextValue( INT64 nextValue )
      {
         _nextValue = nextValue ;
      }
      OSS_INLINE void setAcquireSize( INT32 acquireSize )
      {
         _acquireSize = acquireSize ;
      }
      OSS_INLINE void setIncrement( INT32 increment )
      {
         _increment = increment ;
      }
      OSS_INLINE void decreaseAcquireSize()
      {
         _acquireSize-- ;
      }

      OSS_INLINE void lock()
      {
         _latch.get() ;
      }

      OSS_INLINE void unlock()
      {
         _latch.release() ;
      }

      void copyFrom( const _coordSequence& other )
      {
         // do not change name
         _ID = other._ID ;
         _nextValue = other._nextValue ;
         _acquireSize = other._acquireSize ;
         _increment = other._increment ;
      }

   private:
      std::string    _name ;
      utilSequenceID _ID ;
      INT64          _nextValue ;
      INT32          _acquireSize ;
      INT32          _increment ;
      ossSpinXLatch  _latch ;
   } ;
   typedef _coordSequence coordSequence ;

   class _coordSequenceAgent::_operateSequence
   {
   public:
      virtual INT32 operator() ( coordSequenceAgent *pAgent,
                                 _coordSequence& seq,
                                 _pmdEDUCB *eduCB )
      {
         return SDB_OK ;
      }
      virtual ~_operateSequence() {}
   } ;

   class _coordSequenceAgent::_getNextValue:
         public _coordSequenceAgent::_operateSequence
   {
   public:
      _getNextValue( INT64 &nextValue ): _nextValue( nextValue )
      {
      }

      virtual INT32 operator() ( coordSequenceAgent *pAgent,
                                 _coordSequence& seq,
                                 _pmdEDUCB *eduCB ) ;
   private:
      INT64& _nextValue ;
   } ;

   class _coordSequenceAgent::_adjustNextValue:
         public _coordSequenceAgent::_operateSequence
   {
   public:
      _adjustNextValue( INT64 expectValue ): _expectValue( expectValue )
      {
      }

      virtual INT32 operator() ( coordSequenceAgent *pAgent,
                                 _coordSequence& seq,
                                 _pmdEDUCB *eduCB ) ;

   private:
      void _adjustAscendingly( _coordSequence& seq,
                               INT64 expectValue,
                               BOOLEAN& needAcquire ) ;
      void _adjustDecendingly( _coordSequence& seq,
                               INT64 expectValue,
                               BOOLEAN& needAcquire ) ;

   private:
      INT64 _expectValue ;
   } ;

   _coordSequenceAgent::_coordSequenceAgent()
   {
      _resource = NULL ;
   }

   _coordSequenceAgent::~_coordSequenceAgent()
   {
      clear() ;
   }

   INT32 _coordSequenceAgent::init( _coordResource* resource )
   {
      _resource = resource ;
      return SDB_OK ;
   }

   void _coordSequenceAgent::fini()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT_REMOVE_CACHE, "_coordSequenceAgent::removeCache" )
   BOOLEAN _coordSequenceAgent::removeCache( const std::string& name,
                                             const utilSequenceID ID )
   {
      BOOLEAN removed = FALSE ;
      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT_REMOVE_CACHE ) ;

      COORD_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      COORD_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         _coordSequence* cache = (*iter).second ;
         if( UTIL_SEQUENCEID_NULL != ID && ID != cache->ID() )
         {
            PD_LOG( PDWARNING, "Mismatch ID(%llu) for sequence[%s, %llu]",
                    ID, cache->name().c_str(), cache->ID() ) ;
            goto done ;
         }
         bucket.erase( name ) ;
         SDB_OSS_DEL( cache ) ;
         removed = TRUE ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_AGENT_REMOVE_CACHE, removed ) ;
      return removed ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT_CLEAR, "_coordSequenceAgent::clear" )
   void _coordSequenceAgent::clear()
   {
      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT_CLEAR ) ;

      for ( COORD_SEQ_MAP::bucket_iterator bucketIt = _sequenceCache.begin() ;
            bucketIt != _sequenceCache.end() ;
            bucketIt++ )
      {
         COORD_SEQ_MAP::Bucket& bucket = *bucketIt ;
         BUCKET_XLOCK( bucket ) ;

         for ( COORD_SEQ_MAP::map_const_iterator it = bucket.begin() ;
               it != bucket.end() ;
               it++ )
         {
            _coordSequence* cache = (*it).second ;
            SDB_OSS_DEL ( cache ) ;
         }

         bucket.clear() ;
      }

      PD_TRACE_EXIT( SDB_COORD_SEQ_AGENT_CLEAR ) ;
   }

   INT32 _coordSequenceAgent::getNextValue( const std::string& name,
                                            const utilSequenceID ID,
                                            INT64& nextValue,
                                            _pmdEDUCB* eduCB )
   {
      _getNextValue func( nextValue ) ;
      return _doOnSequence( name, ID, eduCB, func ) ;
   }

   INT32 _coordSequenceAgent::adjustNextValue( const std::string& name,
                                               const utilSequenceID ID,
                                               INT64 userValue,
                                               _pmdEDUCB* eduCB )
   {
      _adjustNextValue func( userValue ) ;
      return _doOnSequence( name, ID, eduCB, func ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT__GET_NEXT_VALUE, "_coordSequenceAgent::_getNextValue::operator" )
   INT32 _coordSequenceAgent::_getNextValue::operator() ( coordSequenceAgent *pAgent,
                                                          _coordSequence& seq,
                                                          _pmdEDUCB* eduCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT__GET_NEXT_VALUE ) ;

      SDB_ASSERT( seq.acquireSize() >= 0, "AcquireSize should >= 0" ) ;

      if ( seq.acquireSize() == 0 )
      {
         rc = pAgent->_acquireSequence( seq, eduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to acquire sequence[%s], rc=%d",
                    seq.name().c_str(), rc ) ;
            goto error ;
         }

         if ( seq.acquireSize() == 0 )
         {
            SDB_ASSERT( FALSE, "AcquireSize == 0" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid AcquireSize of sequence[%s], rc=%d",
                    seq.name().c_str(), rc ) ;
            goto error ;
         }
      }

      seq.decreaseAcquireSize() ;
      _nextValue = seq.nextValue() ;
      seq.setNextValue( seq.nextValue() + seq.increment() ) ;

   done:
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_AGENT__GET_NEXT_VALUE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT__ADJUST_NEXT_VALUE, "_coordSequenceAgent::_adjustNextValue::operator" )
   INT32 _coordSequenceAgent::_adjustNextValue::operator() ( coordSequenceAgent *pAgent,
                                                             _coordSequence& seq,
                                                             _pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needAcquire = FALSE ;
      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT__ADJUST_NEXT_VALUE ) ;

      SDB_ASSERT( seq.acquireSize() >= 0, "AcquireSize should >= 0" ) ;

      if ( 0 != seq.acquireSize() )
      {
         if ( seq.increment() > 0 )
         {
            _adjustAscendingly( seq, _expectValue, needAcquire ) ;
         }
         else
         {
            _adjustDecendingly( seq, _expectValue, needAcquire ) ;
         }
      }
      else
      {
         needAcquire = TRUE ;
      }

      if ( needAcquire )
      {
         rc = pAgent->_acquireSequence( seq, eduCB, TRUE, _expectValue ) ;
         if ( SDB_SEQUENCE_EXCEEDED == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to acquire sequence[%s], rc=%d",
                      seq.name().c_str(), rc ) ;
         goto done ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_AGENT__ADJUST_NEXT_VALUE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _coordSequenceAgent::_adjustNextValue::_adjustAscendingly(
         _coordSequence& seq,
         INT64 expectValue,
         BOOLEAN& needAcquire )
   {
      /*
         adjust rule:
         if expectValue >= maxValue
            acquire a new cache which value greater than expectValue
         if maxValue > expectValue >= nextValue
            adjust nextValue to be greater than expectValue
         if nextValue > expectValue
            ignore
      */
      INT64 maxValue = seq.nextValue() + seq.increment() * (seq.acquireSize() - 1) ;
      if ( expectValue >= maxValue )
      {
         seq.setAcquireSize( 0 ) ;
         needAcquire = TRUE ;
      }
      else if ( expectValue >= seq.nextValue() )
      {
         // any way, an UINT64 can hold the difference between two SINT64
         UINT64 diff = (UINT64) (expectValue - seq.nextValue() + 1) ;
         // acquireSize must be INT32, so this result is always in INT32 range.
         INT32 acquireCount = (INT32) (( diff - 1 ) / seq.increment() + 1) ;
         seq.setAcquireSize( seq.acquireSize() - acquireCount ) ;
         seq.setNextValue( seq.nextValue() + seq.increment() * acquireCount ) ;
      }
   }

   void _coordSequenceAgent::_adjustNextValue::_adjustDecendingly(
         _coordSequence& seq,
         INT64 expectValue,
         BOOLEAN& needAcquire )
   {
      /*
         adjust rule:
         if expectValue <= minValue
            acquire a new cache which value less than expectValue
         if minValue < expectValue <= nextValue
            adjust nextValue to be less than expectValue
         if nextValue < expectValue
            ignore
      */
      INT64 minValue = seq.nextValue() + seq.increment() * (seq.acquireSize() - 1) ;
      if ( expectValue <= minValue )
      {
         seq.setAcquireSize( 0 ) ;
         needAcquire = TRUE ;
      }
      else if ( expectValue <= seq.nextValue() )
      {
         UINT64 diff = (UINT64) (seq.nextValue() - expectValue + 1) ;
         INT32 acquireCount = (INT32) (( diff - 1 ) / (-seq.increment()) + 1) ;
         seq.setAcquireSize( seq.acquireSize() - acquireCount ) ;
         seq.setNextValue( seq.nextValue() + seq.increment() * acquireCount ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT__DO_ON_SEQUENCE, "_coordSequenceAgent::_doOnSequence" )
   INT32 _coordSequenceAgent::_doOnSequence( const std::string& name,
                                             const utilSequenceID ID,
                                             pmdEDUCB *eduCB,
                                             _operateSequence& func )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT__DO_ON_SEQUENCE ) ;
      BOOLEAN noCache = FALSE ;
      utilSequenceID cachedID = UTIL_SEQUENCEID_NULL ;

      rc = _doOnSequenceBySLock( name, ID, eduCB, func, noCache, &cachedID ) ;
      if ( SDB_OK == rc )
      {
         // if no sequence cache, should get bucket by XLock
         if ( noCache )
         {
            rc = _doOnSequenceByXLock( name, ID, eduCB, func ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
      }
      else
      {
         if ( SDB_SEQUENCE_NOT_EXIST == rc )
         {
            // remove cache if sequence not exist
            _removeCacheByID( name, cachedID ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_AGENT__DO_ON_SEQUENCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT__DO_ON_SEQ_BY_SLOCK, "_coordSequenceAgent::_doOnSequenceBySLock" )
   INT32 _coordSequenceAgent::_doOnSequenceBySLock( const std::string& name,
                                                    const utilSequenceID ID,
                                                    _pmdEDUCB *eduCB,
                                                    _operateSequence& func,
                                                    BOOLEAN& noCache,
                                                    utilSequenceID* cachedSeqID )
   {
      INT32 rc = SDB_OK ;
      _coordSequence* cache = NULL ;
      BOOLEAN cacheLocked = FALSE ;
      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT__DO_ON_SEQ_BY_SLOCK ) ;

      COORD_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_SLOCK( bucket ) ;

      COORD_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         cache = (*iter).second ;
         noCache = FALSE ;
         if( ID != UTIL_SEQUENCEID_NULL && cache->ID() != ID )
         {
            PD_LOG( PDWARNING, "Mismatch ID(%llu) for sequence[%s, %llu]",
                    ID, cache->name().c_str(), cache->ID() ) ;
            noCache = TRUE ;
            goto done ;
         }
      }
      else
      {
         // no sequence cache, should get bucket by XLock
         noCache = TRUE ;
         goto done ;
      }

      cache->lock() ;
      cacheLocked = TRUE ;

      rc = func( this, *cache, eduCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to process sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         // if rc==SDB_SEQUENCE_NOT_EXIST, we should delete the cache
         // here we get SLOCK, so we need to get XLOCK to delete the cache
         // check the id consistency when delete cache
         *cachedSeqID = cache->ID() ;
         goto error ;
      }

   done:
      if ( cacheLocked )
      {
         cache->unlock() ;
         cacheLocked = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_AGENT__DO_ON_SEQ_BY_SLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT__DO_ON_SEQ_BY_XLOCK, "_coordSequenceAgent::_doOnSequenceByXLock" )
   INT32 _coordSequenceAgent::_doOnSequenceByXLock( const std::string &name,
                                                    const utilSequenceID ID,
                                                    pmdEDUCB *eduCB,
                                                    _operateSequence& func )
   {
      INT32 rc = SDB_OK ;
      _coordSequence* cache = NULL ;
      _coordSequence* sequence = NULL ;
      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT__DO_ON_SEQ_BY_XLOCK ) ;

      COORD_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      COORD_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      // if mismatch in the cache, get from catalog and clear the
      // old cached sequence later.
      // if not get by ID, read from cache when name matched;
      // if get by ID, read from catalog when name match but ID mismatch
      if ( bucket.end() != iter && ( UTIL_SEQUENCEID_NULL == ID ||
           ((*iter).second)->ID() == ID ) )
      {
         cache = (*iter).second ;
      }
      else
      {
         _coordSequence seq = _coordSequence( name ) ;
         if ( UTIL_SEQUENCEID_NULL != ID )
         {
            seq.setID( ID );
         }

         rc = _acquireSequence( seq, eduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to acquire sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         // succeed to get seq from catalog, then remove old cached sequence.
         if( bucket.end() != iter &&
             UTIL_SEQUENCEID_NULL != ID &&
             ((*iter).second)->ID() != ID )
         {
            cache = (*iter).second ;
            PD_LOG( PDWARNING, "Mismatch ID(%llu) for sequence[%s, %llu]",
                    ID, cache->name().c_str(), cache->ID() ) ;
            bucket.erase( name ) ;
            SDB_OSS_DEL( cache ) ;
            cache = NULL ;
         }

         sequence = SDB_OSS_NEW _coordSequence( name ) ;
         if ( NULL == sequence )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         sequence->copyFrom( seq ) ;

         try
         {
            bucket.insert( COORD_SEQ_MAP::value_type( name, sequence ) ) ;
         }
         catch( std::exception & )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to insert sequence[%s] to cache",
                    name.c_str() ) ;
            goto error ;
         }

         cache = sequence ;
         sequence = NULL ;
      }

      // we have got XLOCK, so no need to lock cache

      rc = func( this, *cache, eduCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to process sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         if ( SDB_SEQUENCE_NOT_EXIST == rc )
         {
            // remove cache if sequence not exist
            // we have got XLOCK, so we can delete it directly
            bucket.erase( name ) ;
            SDB_OSS_DEL( cache ) ;
            cache = NULL ;
         }
         goto error ;
      }

   done:
      SAFE_OSS_DELETE( sequence ) ;
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_AGENT__DO_ON_SEQ_BY_XLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT__ACQUIRE_SEQ, "_coordSequenceAgent::_acquireSequence" )
   INT32 _coordSequenceAgent::_acquireSequence( _coordSequence& seq,
                                                _pmdEDUCB* eduCB,
                                                BOOLEAN hasExpectValue,
                                                INT64 expectValue )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT__ACQUIRE_SEQ ) ;

      coordGroupSession session ;
      pmdSubSession *subSession = NULL ;
      MsgHeader *reply = NULL ;
      BSONObj options ;
      CHAR *pBuffer = NULL ;
      INT32 bufferSize = 0 ;

      SDB_ASSERT( seq.acquireSize() == 0, "AcquireSize should be 0" ) ;

      try
      {
         BSONObjBuilder builder ;
         builder.append( CAT_SEQUENCE_NAME, seq.name() ) ;
         if ( seq.ID() != UTIL_SEQUENCEID_NULL )
         {
            builder.append( CAT_SEQUENCE_ID, (INT64)seq.ID() ) ;
         }
         if ( hasExpectValue )
         {
            builder.append( CAT_SEQUENCE_EXPECT_VALUE, expectValue ) ;
         }
         options = builder.obj() ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build acquire msg options for "
                 "sequence[%s], exception: %s, rc=%d",
                 seq.name().c_str(), e.what(), rc ) ;
         goto error ;
      }

      rc = session.init( _resource, eduCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init coord remote session, rc=%d", rc ) ;
         goto error ;
      }
      session.getGroupSel()->setPrimary( TRUE ) ;
      session.getGroupSel()->setServiceType( MSG_ROUTE_CAT_SERVICE ) ;

      rc = msgBuildSequenceAcquireMsg( &pBuffer, &bufferSize, 0,
                                       options, eduCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to build acquire sequence request, rc=%d",
                  rc ) ;
         goto error ;
      }

   retry:
      session.getSession()->clearSubSession() ;
      rc = session.sendMsg( (MsgHeader*)pBuffer, CATALOG_GROUPID,
                            NULL, &subSession ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to send acquire sequence msg, rc=%d", rc ) ;
         goto error ;
      }

      // recv reply
      rc = session.getSession()->waitReply1( TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to wait acquire sequence reply from "
                 "catalog group, rc=%d",
                 rc ) ;
         goto error ;
      }

      reply = subSession->getRspMsg() ;
      rc = _processAcquireReply( reply, seq ) ;
      if ( SDB_OK != rc )
      {
         coordGroupSessionCtrl* groupCtrl = session.getGroupCtrl() ;
         UINT32 primaryID = ((MsgOpReply*)reply)->startFrom ;

         if ( groupCtrl->canRetry( rc, reply->routeID,
                                   primaryID, TRUE, TRUE ) )
         {
            groupCtrl->incRetry() ;
            goto retry ;
         }

         PD_LOG( PDERROR, "Failed to process acquire sequence reply, rc=%d",
                 rc ) ;
         goto error ;
      }

   done:
      if ( pBuffer )
      {
         msgReleaseBuffer( pBuffer, eduCB ) ;
         bufferSize = 0 ;
      }
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_AGENT__ACQUIRE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT__PROCESS_ACQUIRE_REPLY, "_coordSequenceAgent::_processAcquireReply" )
   INT32 _coordSequenceAgent::_processAcquireReply( MsgHeader* msg,
                                                    _coordSequence& seq )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply *reply = ( MsgOpReply* )msg ;
      BSONObj obj ;
      BSONElement ele ;
      utilSequenceID ID = UTIL_SEQUENCEID_NULL ;
      INT64 nextValue = 0 ;
      INT32 acquireSize = 0 ;
      INT32 increment = 0 ;

      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT__PROCESS_ACQUIRE_REPLY ) ;
      SDB_ASSERT( -1 == reply->contextID, "ContextID must be -1" ) ;

      rc = reply->flags ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Recieve error reply for acquire sequence "
                 "request from node[%s], flag=%d",
                 routeID2String( msg->routeID ).c_str(), rc ) ;
         goto error ;
      }

      rc = msgExtractSequenceAcquireReply( (CHAR*)msg, obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to extract reply for acquire sequence "
                 "request from node[%s], rc=%d",
                 routeID2String( msg->routeID ).c_str(), rc ) ;
         goto error ;
      }

      // CAT_SEQUENCE_NAME
      ele = obj.getField( CAT_SEQUENCE_NAME ) ;
      if ( String == ele.type() )
      {
         std::string name = ele.String() ;
         if ( name != seq.name() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid sequence name[%s], expected[%s]",
                    name.c_str(), seq.name().c_str() ) ;
            goto error ;
         }
      }
      else if ( EOO == ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Missing option[%s]", CAT_SEQUENCE_NAME ) ;
         goto error ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_NAME ) ;
         goto error ;
      }

      // CAT_SEQUENCE_ID
      ele = obj.getField( CAT_SEQUENCE_ID ) ;
      if ( ele.isNumber() )
      {
         ID = ele.Long() ;
      }
      else if ( EOO == ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Missing option[%s]", CAT_SEQUENCE_ID ) ;
         goto error ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_ID ) ;
         goto error ;
      }

      // CAT_SEQUENCE_NEXT_VALUE
      ele = obj.getField( CAT_SEQUENCE_NEXT_VALUE ) ;
      if ( NumberLong == ele.type() )
      {
         nextValue = ele.numberLong() ;
      }
      else if ( EOO == ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Missing option[%s]", CAT_SEQUENCE_NEXT_VALUE ) ;
         goto error ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_NEXT_VALUE ) ;
         goto error ;
      }

      // CAT_SEQUENCE_ACQUIRE_SIZE
      ele = obj.getField( CAT_SEQUENCE_ACQUIRE_SIZE ) ;
      if ( NumberInt == ele.type() )
      {
         acquireSize = ele.numberInt() ;
      }
      else if ( EOO == ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Missing option[%s]", CAT_SEQUENCE_ACQUIRE_SIZE ) ;
         goto error ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_ACQUIRE_SIZE ) ;
         goto error ;
      }

      // CAT_SEQUENCE_INCREMENT
      ele = obj.getField( CAT_SEQUENCE_INCREMENT ) ;
      if ( NumberInt == ele.type() )
      {
         increment = ele.numberInt() ;
      }
      else if ( EOO == ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Missing option[%s]", CAT_SEQUENCE_INCREMENT ) ;
         goto error ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid type(%d) for option[%s]",
                 ele.type(), CAT_SEQUENCE_INCREMENT ) ;
         goto error ;
      }

      if ( UTIL_SEQUENCEID_NULL != ID && seq.ID() != ID )
      {
         PD_LOG( PDWARNING, "Mismatch ID(%llu) for sequence[%s, %llu]",
                 ID, seq.name().c_str(), seq.ID() ) ;
      }
      seq.setID( ID ) ;
      seq.setNextValue( nextValue ) ;
      seq.setAcquireSize( acquireSize ) ;
      seq.setIncrement( increment ) ;

   done:
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_AGENT__PROCESS_ACQUIRE_REPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_AGENT__REMOVE_CACHE_BY_ID, "_coordSequenceAgent::_removeCacheByID" )
   BOOLEAN _coordSequenceAgent::_removeCacheByID( const std::string& name,
                                                  const utilSequenceID ID )
   {
      BOOLEAN removed = FALSE ;
      PD_TRACE_ENTRY ( SDB_COORD_SEQ_AGENT__REMOVE_CACHE_BY_ID ) ;

      COORD_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      COORD_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         _coordSequence* cache = (*iter).second ;
         // if id not equal, means the cache has been changed, can't delete
         if ( cache->ID() == ID )
         {
            bucket.erase( name ) ;
            SDB_OSS_DEL( cache ) ;
            removed = TRUE ;
         }
      }

      PD_TRACE_EXITRC ( SDB_COORD_SEQ_AGENT__REMOVE_CACHE_BY_ID, removed ) ;
      return removed ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__COORD_SEQ_INVALIDATE_CACHE, "_coordSequenceInvalidateCache" )
   INT32 _coordSequenceInvalidateCache ( const BSONObj & commandObject,
                                         _pmdEDUCB * eduCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__COORD_SEQ_INVALIDATE_CACHE ) ;

      CHAR * buffer = NULL ;
      INT32 bufSize = 0 ;
      INT64 contextID = -1 ;
      coordCMDInvalidateSequenceCache invalidator ;

      SDB_ASSERT( NULL != eduCB, "eduCB can't null" ) ;

      rc = msgBuildSequenceInvalidateCacheMsg( &buffer, &bufSize, commandObject,
                                               0, eduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build sequence invalidate cache "
                   "msg, rc: %d", rc ) ;

      rc = invalidator.init( sdbGetCoordCB()->getResource(), eduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init sequence invalidate cache "
                 "command, rc: %d", rc ) ;

      rc = invalidator.execute( (MsgHeader*)buffer, eduCB, contextID, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute sequence invalidate "
                   "cache command, rc: %d", rc ) ;

   done :
      if ( NULL != buffer )
      {
         msgReleaseBuffer( buffer, eduCB ) ;
      }
      PD_TRACE_EXITRC( SDB__COORD_SEQ_INVALIDATE_CACHE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_INVALIDATE_CACHE_SEQ, "coordSequenceInvalidateCache" )
   INT32 coordSequenceInvalidateCache ( const CHAR * sequenceName,
                                        utilSequenceID sequenceID,
                                        _pmdEDUCB * eduCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_COORD_SEQ_INVALIDATE_CACHE_SEQ ) ;

      BSONObjBuilder builder ;
      BSONObj commandObject ;

      SDB_ASSERT( NULL != sequenceName, "sequence name is invalid" ) ;

      try
      {
         builder.append( FIELD_NAME_SEQUENCE_NAME, sequenceName ) ;
         if( UTIL_SEQUENCEID_NULL != sequenceID )
         {
            builder.append( FIELD_NAME_SEQUENCE_ID , (INT64)sequenceID ) ;
         }
         commandObject = builder.obj() ;
      }
      catch ( std::exception & e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception: %s", e.what() ) ;
         goto error ;
      }

      rc = _coordSequenceInvalidateCache( commandObject, eduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call sequence invalidate "
                   "command by sequence name [%s], rc: %d",
                   sequenceName, rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_INVALIDATE_CACHE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORD_SEQ_INVALIDATE_CACHE_COL, "coordSequenceInvalidateCache" )
   INT32 coordSequenceInvalidateCache ( const CHAR * collection,
                                        const CHAR * fieldName,
                                        _pmdEDUCB * eduCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_COORD_SEQ_INVALIDATE_CACHE_COL ) ;

      BSONObjBuilder builder ;
      BSONObj commandObject ;

      SDB_ASSERT( NULL != collection && NULL != fieldName,
                  "collection and field names are invalid" ) ;

      try
      {
         builder.append( FIELD_NAME_COLLECTION, collection ) ;
         builder.append( FIELD_NAME_AUTOINC_FIELD, fieldName ) ;
         commandObject = builder.obj() ;
      }
      catch ( std::exception & e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception: %s", e.what() ) ;
         goto error ;
      }

      rc = _coordSequenceInvalidateCache( commandObject, eduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call sequence invalidate "
                   "command for collection [%s] field [%s], rc: %d",
                   collection, fieldName, rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_COORD_SEQ_INVALIDATE_CACHE_COL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

