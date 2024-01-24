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

   Source File Name = catSequenceManager.cpp

   Descriptive Name = Sequence manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "catSequenceManager.hpp"
#include "catGTSDef.hpp"
#include "catCommon.hpp"
#include "dmsCB.hpp"
#include "dpsLogWrapper.hpp"
#include "rtn.hpp"
#include "rtnContextBuff.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{
   class _catSequenceManager::_operateSequence
   {
   public:
      virtual INT32 operator()( _catSequenceManager *pMgr,
                                _catSequence& seq,
                                _pmdEDUCB* eduCB,
                                INT16 w )
      {
         return SDB_OK ;
      }
      virtual ~_operateSequence() {}
   } ;

   class _catSequenceManager::_acquireSequence:
         public _catSequenceManager::_operateSequence
   {
   public:
      _acquireSequence( _catSequenceAcquirer &acquirer ):
            _acquirer( acquirer )
      {
      }

      virtual INT32 operator()( _catSequenceManager *pMgr,
                                _catSequence &seq,
                                _pmdEDUCB *eduCB,
                                INT16 w ) ;
   private:
      INT32 _acquireAscendingSequence( _catSequence& seq,
                                       _catSequenceAcquirer& acquirer,
                                       BOOLEAN& needUpdate ) ;
      INT32 _acquireDescendingSequence( _catSequence& seq,
                                        _catSequenceAcquirer& acquirer,
                                        BOOLEAN& needUpdate ) ;
   private:
      _catSequenceAcquirer& _acquirer ;
   } ;

   class _catSequenceManager::_adjustSequence:
         public _catSequenceManager::_operateSequence
   {
   public:
      _adjustSequence( INT64 expectValue, utilSequenceID *pAlteredSeqID ) :
            _expectValue( expectValue ),
            _pAlteredSeqID( pAlteredSeqID )
      {
      }

      virtual INT32 operator()( _catSequenceManager *pMgr,
                                _catSequence &seq,
                                _pmdEDUCB *eduCB,
                                INT16 w ) ;
   private:
      INT32 _adjustAscendingSequence( _catSequence& seq,
                                      INT64 expectValue,
                                      BOOLEAN& needUpdate ) ;
      INT32 _adjustDecendingSequence( _catSequence& seq,
                                      INT64 expectValue,
                                      BOOLEAN& needUpdate ) ;
   private:
      INT64 _expectValue ;
      utilSequenceID* _pAlteredSeqID ;
   } ;

   _catSequenceManager::_catSequenceManager()
   {
   }

   _catSequenceManager::~_catSequenceManager()
   {
      _cleanCache( FALSE ) ;
   }

   INT32 _catSequenceManager::active()
   {
      _cleanCache( FALSE ) ;
      return SDB_OK ;
   }

   INT32 _catSequenceManager::deactive()
   {
      _cleanCache( TRUE ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_CREATE_SEQ, "_catSequenceManager::createSequence" )
   INT32 _catSequenceManager::createSequence( const std::string& name,
                                              utilCLUniqueID clUniqueID,
                                              const BSONObj& options,
                                              _pmdEDUCB* eduCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONElement ele ;
      utilGlobalID ID = UTIL_GLOBAL_NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_CREATE_SEQ ) ;

      _catSequence sequence( name ) ;

      rc = _catSequence::validateFieldNames( options ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Invalid options of sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if( options.hasField( CAT_SEQUENCE_CURRENT_VALUE ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Field(%s) not allowed when create sequence",
                 CAT_SEQUENCE_CURRENT_VALUE ) ;
         goto error ;
      }

      rc = sequence.setOptions( options, TRUE, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if( options.hasField( CAT_SEQUENCE_ID ) )
      {
         ele = options.getField( CAT_SEQUENCE_ID ) ;
         if ( !ele.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid type[%d] of sequence options[%s]",
                    ele.type(), CAT_SEQUENCE_OID ) ;
            goto error ;
         }
         ID = ele.Long() ;
      }
      else
      {
         rc = catUpdateGlobalID( eduCB, w, ID ) ;
         if( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get globalID when create sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
      }

      sequence.setID( ID ) ;
      sequence.setOID( OID::gen() ) ;
      sequence.setVersion( 0 ) ;
      sequence.setCLUniqueID( clUniqueID ) ;
      sequence.setInitial( TRUE ) ;

      rc = sequence.validate() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Invalid sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      rc = sequence.toBSONObj( obj, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = insertSequence( name, obj, eduCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert sequence [%s], rc: %d",
                   name.c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_CREATE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_INSERT_SEQ, "_catSequenceManager::insertSequence" )
   INT32 _catSequenceManager::insertSequence ( const std::string & name,
                                               BSONObj & options,
                                               _pmdEDUCB * eduCB,
                                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_GTS_SEQ_MGR_INSERT_SEQ ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      PD_CHECK( bucket.end() == iter, SDB_SEQUENCE_EXIST, error, PDERROR,
                "Sequence [%s] is already existing", name.c_str() ) ;

      rc = _insertSequence( options, eduCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert sequence [%s], rc: %d",
                   name.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_GTS_SEQ_MGR_INSERT_SEQ, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_DROP_SEQ, "_catSequenceManager::dropSequence" )
   INT32 _catSequenceManager::dropSequence( const std::string& name,
                                            _pmdEDUCB* eduCB, INT16 w,
                                            utilSequenceID *pDroppedSeqID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_DROP_SEQ ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         _catSequence* sequence = (*iter).second ;
         bucket.erase( name ) ;
         SDB_OSS_DEL sequence ;
      }

      rc = _deleteSequence( name, eduCB, w, pDroppedSeqID ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_DROP_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_ALTER_SEQ, "_catSequenceManager::alterSequence" )
   INT32 _catSequenceManager::alterSequence( const std::string& name,
                                             const BSONObj& options,
                                             _pmdEDUCB* eduCB,
                                             INT16 w,
                                             bson::BSONObj* oldOptions,
                                             UINT32* alterMask,
                                             utilSequenceID* pAlteredSeqID )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      UINT32 fieldMask = UTIL_ARG_FIELD_EMPTY ;
      _catSequence* cache = NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_ALTER_SEQ ) ;

      _catSequence sequence( name ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         // found from cache
         cache = (*iter).second ;
         sequence.copyFrom( *cache ) ;
      }
      else
      {
         // not found from cache, acquire from collection
         BSONObj seqObj ;
         rc = _findSequence( name, seqObj, eduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to find sequence[%s] from system collection, rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         rc = sequence.loadOptions( seqObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
      }

      if ( NULL != oldOptions )
      {
         // safe to be copied the old values under x-lock
         rc = sequence.toBSONObj( *oldOptions, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save old BSONObj for "
                      "sequence[%s] before update, rc: %d",
                      name.c_str(), rc ) ;
      }

      // update the sequence now
      rc = sequence.setOptions( options, FALSE, FALSE, &fieldMask ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if ( NULL != alterMask )
      {
         // update the alter mask if needed
         *alterMask = fieldMask ;
      }

      if ( UTIL_ARG_FIELD_EMPTY == fieldMask )
      {
         // nothing changed
         goto done ;
      }

      sequence.increaseVersion() ;

      rc = sequence.validate() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Invalid sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      rc = sequence.toBSONObj( obj, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build BSONObj for sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      rc = _updateSequence( name, obj, eduCB, w, pAlteredSeqID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to update sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if ( NULL != cache )
      {
         cache->copyFrom( sequence ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_ALTER_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_RENAME_SEQ, "_catSequenceManager::renameSequence" )
   INT32 _catSequenceManager::renameSequence( const std::string& oldName,
                                              const std::string& newName,
                                              _pmdEDUCB* eduCB,
                                              INT16 w,
                                              utilSequenceID* pAlteredSeqID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_RENAME_SEQ ) ;

      BSONObj obj ;
      _catSequence* cache = NULL ;

      _catSequence *pSequence = SDB_OSS_NEW _catSequence( newName ) ;
      if ( NULL == pSequence )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc sequence[%s], rc=%d",
                 newName.c_str(), rc ) ;
         goto error ;
      }

      {
         CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( oldName ) ;
         BUCKET_XLOCK( bucket ) ;

         CAT_SEQ_MAP::map_const_iterator iter = bucket.find( oldName ) ;
         if ( bucket.end() != iter )
         {
            cache = (*iter).second ;
            pSequence->copyFrom( *cache ) ;
         }
         else
         {
            BSONObj seqObj ;
            rc = _findSequence( oldName, seqObj, eduCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR,
                       "Failed to find sequence[%s] from system collection, rc=%d",
                       oldName.c_str(), rc ) ;
               goto error ;
            }

            rc = pSequence->loadOptions( seqObj ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                       oldName.c_str(), rc ) ;
               goto error ;
            }
         }

         pSequence->increaseVersion() ;

         rc = pSequence->validate() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Invalid sequence[%s], rc=%d",
                    newName.c_str(), rc ) ;
            goto error ;
         }

         rc = pSequence->toBSONObj( obj, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build BSONObj for sequence[%s], rc=%d",
                    newName.c_str(), rc ) ;
            goto error ;
         }

         rc = _updateSequence( oldName, obj, eduCB, w, pAlteredSeqID ) ;
         if ( SDB_IXM_DUP_KEY == rc )
         {
            rc = SDB_SEQUENCE_EXIST ;
            goto error ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to update sequence[%s], rc=%d",
                    oldName.c_str(), rc ) ;
            goto error ;
         }

         bucket.erase( oldName ) ;
         SDB_OSS_DEL( cache ) ;
      }

      try
      {
         CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( newName ) ;
         BUCKET_XLOCK( bucket ) ;
         bucket.insert( CAT_SEQ_MAP::value_type( newName, pSequence ) ) ;
         pSequence = NULL ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDWARNING,
                 "Failed to insert sequence[%s] to cache, exception: %s, rc: %d",
                 newName.c_str(), e.what(), rc ) ;
         rc = SDB_OK ;
      }
   done:
      if ( pSequence )
      {
         SDB_OSS_DEL( pSequence ) ;
      }
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_RENAME_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_RESTART_SEQ, "_catSequenceManager::restartSequence" )
   INT32 _catSequenceManager::restartSequence( const std::string& name,
                                               const INT64 startValue,
                                               _pmdEDUCB* eduCB,
                                               INT16 w,
                                               utilSequenceID* pAlteredSeqID )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      _catSequence* cache = NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_RESTART_SEQ ) ;

      _catSequence sequence( name ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         // found from cache
         cache = (*iter).second ;
         sequence.copyFrom( *cache ) ;
      }
      else
      {
         // not found from cache, acquire from collection
         BSONObj seqObj ;
         rc = _findSequence( name, seqObj, eduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR,
                    "Failed to find sequence[%s] from system collection, rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         rc = sequence.loadOptions( seqObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
      }

      if ( startValue < sequence.getMinValue() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The start value[%lld] cannot be less than the "
                 "minimum value[%lld]", startValue, sequence.getMinValue() ) ;
         goto error ;
      }
      else if ( startValue > sequence.getMaxValue() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The start value[%lld] cannot be greater than the "
                 "maximum value[%lld]", startValue, sequence.getMaxValue() ) ;
         goto error ;
      }

      sequence.setCurrentValue( startValue ) ;
      sequence.setCachedValue( startValue ) ;
      sequence.setInitial( TRUE ) ;
      sequence.setExceeded( FALSE ) ;

      rc = sequence.toBSONObj( obj, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build BSONObj for sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      rc = _updateSequence( name, obj, eduCB, w, pAlteredSeqID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to update sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if ( NULL != cache )
      {
         cache->copyFrom( sequence ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_RESTART_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _catSequenceManager::acquireSequence( const std::string& name,
                                               const utilSequenceID ID,
                                               _catSequenceAcquirer& acquirer,
                                               _pmdEDUCB* eduCB, INT16 w )
   {
      _acquireSequence func( acquirer ) ;
      return _doOnSequence( name, ID, eduCB, w, func ) ;
   }

   INT32 _catSequenceManager::adjustSequence( const std::string &name,
                                              INT64 expectValue,
                                              _pmdEDUCB *eduCB, INT16 w,
                                              utilSequenceID* pAlteredSeqID )
   {
      _adjustSequence func( expectValue, pAlteredSeqID ) ;
      return _doOnSequence( name, UTIL_SEQUENCEID_NULL, eduCB, w, func ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR_RESET_SEQ, "_catSequenceManager::resetSequence" )
   INT32 _catSequenceManager::resetSequence( const std::string& name,
                                             _pmdEDUCB* eduCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      _catSequence* cache = NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR_RESET_SEQ ) ;

      _catSequence sequence( name ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         cache = (*iter).second ;
         sequence.copyFrom( *cache ) ;
      }
      else
      {
         BSONObj seqObj ;
         rc = _findSequence( name, seqObj, eduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to find sequence[%s] from system collection, rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         rc = sequence.loadOptions( seqObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
      }

      sequence.setCachedValue( sequence.getStartValue() ) ;
      sequence.setCurrentValue( sequence.getStartValue() ) ;
      sequence.setInitial( TRUE ) ;
      sequence.setExceeded( FALSE ) ;
      sequence.setCycledCount( 0 ) ;
      sequence.increaseVersion() ;

      rc = sequence.toBSONObj( obj, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build BSONObj for sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      rc = _updateSequence( name, obj, eduCB, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to reset sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      if ( NULL != cache )
      {
         cache->copyFrom( sequence ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR_RESET_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ACQUIRE_SEQ, "_catSequenceManager::_acquireSequence::operator" )
   INT32 _catSequenceManager::_acquireSequence::operator()( _catSequenceManager *pMgr,
                                                            _catSequence& seq,
                                                            _pmdEDUCB* eduCB,
                                                            INT16 w )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needUpdate = FALSE ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ACQUIRE_SEQ ) ;

      if ( seq.getIncrement() > 0 )
      {
         rc = _acquireAscendingSequence( seq, _acquirer, needUpdate ) ;
      }
      else
      {
         rc = _acquireDescendingSequence( seq, _acquirer, needUpdate );
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to acquire sequence[%s], rc=%d",
                 seq.getName().c_str(), rc ) ;
         goto error ;
      }

      _acquirer.ID = seq.getID() ;

      if ( needUpdate )
      {
         BSONObjBuilder builder( 128 ) ;
         BSONObj options ;
         try
         {
            builder.append( CAT_SEQUENCE_CURRENT_VALUE, seq.getCurrentValue() ) ;
            builder.append( CAT_SEQUENCE_INITIAL, (bool) seq.isInitial() ) ;
            builder.append( CAT_SEQUENCE_CYCLED_COUNT, seq.getCycledCount() ) ;
            options = builder.obj() ;
         }
         catch( std::exception& e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Failed to build bson, exception: %s, rc=%d",
                    e.what(), rc ) ;
            goto error ;
         }

         rc = pMgr->_updateSequence( seq.getName(), options, eduCB, w ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to update sequence[%s], rc=%d",
                    seq.getName().c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__ACQUIRE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ACQUIRE_ASCENDING_SEQ, "_catSequenceManager::_acquireSequence::_acquireAscendingSequence" )
   INT32 _catSequenceManager::_acquireSequence::_acquireAscendingSequence(
         _catSequence& seq,
         _catSequenceAcquirer& acquirer,
         BOOLEAN& needUpdate )
   {
      INT32 rc = SDB_OK ;
      INT64 nextValue = 0 ;
      INT64 fetchInc = 0 ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ACQUIRE_ASCENDING_SEQ ) ;

      SDB_ASSERT( seq.getIncrement() > 0, "increment should be > 0" ) ;

      needUpdate = FALSE ;

      if ( seq.isInitial() )
      {
         // Reason why we get CurrentValue instead of StartValue: At the most
         // beginning, CurrentValue equals StartValue. But after restarting,
         // we need the new CurrentValue, and it may not equal StartValue.
         nextValue = seq.getCurrentValue() ;
         needUpdate = TRUE ;
      }
      else if ( seq.isExceeded() )
      {
         // reach the maximum limit
         if ( !seq.isCycled() )
         {
            rc = SDB_SEQUENCE_EXCEEDED ;
            PD_LOG( PDERROR, "Sequence[%s] value(%lld) is reach the maximum value(%lld)",
                    seq.getName().c_str(), seq.getCurrentValue(), seq.getMaxValue() ) ;
            goto error ;
         }
         else
         {
            // restart from minValue
            nextValue = seq.getMinValue() ;
            seq.setCurrentValue( nextValue ) ;
            seq.setCachedValue( nextValue ) ;
            seq.increaseCycledCount() ;
            needUpdate = TRUE ;
         }
      }
      else
      {
         // safe to increase value
         nextValue = seq.getCachedValue() + seq.getIncrement() ;
      }

      fetchInc = ( seq.getAcquireSize() - 1 ) * (INT64) seq.getIncrement() ;

      // use minus to avoid overflow
      if ( seq.getCurrentValue() - nextValue >= fetchInc )
      {
         seq.setCachedValue( nextValue + fetchInc ) ;
         acquirer.nextValue = nextValue ;
         acquirer.acquireSize = seq.getAcquireSize() ;
         acquirer.increment = seq.getIncrement() ;
      }
      else
      {
         INT64 cachedInc = seq.getCacheSize() * (INT64) seq.getIncrement() ;
         if ( seq.isInitial() || seq.isExceeded() )
         {
            // exclude the start value
            cachedInc -= (INT64) seq.getIncrement() ;
         }
         if ( seq.getCurrentValue() <= seq.getMaxValue() - cachedInc )
         {
            seq.setCachedValue( nextValue + fetchInc ) ;
            seq.setCurrentValue( seq.getCurrentValue() + cachedInc ) ;
         }
         else
         {
            INT64 newCurrentValue = seq.getCurrentValue() +
               ( seq.getMaxValue() - seq.getCurrentValue() ) / seq.getIncrement() * seq.getIncrement() ;
            seq.setCurrentValue( newCurrentValue ) ;
            // use minus to avoid overflow
            if ( seq.getCurrentValue() - nextValue >= fetchInc )
            {
               seq.setCachedValue( nextValue + fetchInc ) ;
            }
            else
            {
               INT64 newCachedValue = seq.getCachedValue() +
                  ( seq.getCurrentValue() - seq.getCachedValue() ) / seq.getIncrement() * seq.getIncrement() ;
               seq.setCachedValue( newCachedValue ) ;
            }
         }

         acquirer.nextValue = nextValue ;
         acquirer.acquireSize = ( seq.getCachedValue() - nextValue ) / seq.getIncrement() + 1 ;
         acquirer.increment = seq.getIncrement() ;
         needUpdate = TRUE ;
      }

      if ( seq.isInitial() )
      {
         seq.setInitial( FALSE ) ;
      }

      if ( seq.isExceeded() )
      {
         seq.setExceeded( FALSE ) ;
      }

      // reach the maxValue, so mark exceeded
      if ( seq.getCachedValue() == seq.getMaxValue() ||
           seq.getCachedValue() > seq.getMaxValue() - seq.getIncrement() )
      {
         seq.setExceeded( TRUE ) ;
         needUpdate = TRUE ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__ACQUIRE_ASCENDING_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ACQUIRE_DESCENDING_SEQ, "_catSequenceManager::_acquireSequence::_acquireDescendingSequence" )
   INT32 _catSequenceManager::_acquireSequence::_acquireDescendingSequence(
         _catSequence& seq,
         _catSequenceAcquirer& acquirer,
         BOOLEAN& needUpdate )
   {
      INT32 rc = SDB_OK ;
      INT64 nextValue = 0 ;
      INT64 fetchInc = 0 ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ACQUIRE_DESCENDING_SEQ ) ;

      SDB_ASSERT( seq.getIncrement() < 0, "increment should be < 0" ) ;

      needUpdate = FALSE ;

      if ( seq.isInitial() )
      {
         // Reason why we get CurrentValue instead of StartValue: At the most
         // beginning, CurrentValue equals StartValue. But after restarting,
         // we need the new CurrentValue, and it may not equal StartValue.
         nextValue = seq.getCurrentValue() ;
         needUpdate = TRUE ;
      }
      else if ( seq.isExceeded() )
      {
         // reach the minimum limit
         if ( !seq.isCycled() )
         {
            rc = SDB_SEQUENCE_EXCEEDED ;
            PD_LOG( PDERROR, "Sequence[%s] value(%lld) is reach the minimum value(%lld)",
                    seq.getName().c_str(), seq.getCurrentValue(), seq.getMinValue() ) ;
            goto error ;
         }
         else
         {
            // restart from maxValue
            nextValue = seq.getMaxValue() ;
            seq.setCurrentValue( nextValue ) ;
            seq.setCachedValue( nextValue ) ;
            seq.increaseCycledCount() ;
            needUpdate = TRUE ;
         }
      }
      else
      {
         // safe to decrease value
         nextValue = seq.getCachedValue() + seq.getIncrement() ;
      }

      fetchInc = ( seq.getAcquireSize() - 1 ) * (INT64) seq.getIncrement() ;

      // use minus to avoid overflow
      if ( nextValue - seq.getCurrentValue() >= -fetchInc )
      {
         seq.setCachedValue( nextValue + fetchInc ) ;
         acquirer.nextValue = nextValue ;
         acquirer.acquireSize = seq.getAcquireSize() ;
         acquirer.increment = seq.getIncrement() ;
      }
      else
      {
         INT64 cachedInc = seq.getCacheSize() * (INT64) seq.getIncrement() ;
         if ( seq.isInitial() || seq.isExceeded() )
         {
            // exclude the start value
            cachedInc -= (INT64) seq.getIncrement() ;
         }
         if ( seq.getCurrentValue() >= seq.getMinValue() - cachedInc )
         {
            seq.setCachedValue( nextValue + fetchInc ) ;
            seq.setCurrentValue( seq.getCurrentValue() + cachedInc ) ;
         }
         else
         {
            INT64 newCurrentValue = seq.getCurrentValue() +
               ( seq.getMinValue() - seq.getCurrentValue() ) / seq.getIncrement() * seq.getIncrement() ;
            seq.setCurrentValue( newCurrentValue ) ;
            // use minus to avoid overflow
            if ( nextValue - seq.getCurrentValue() >= -fetchInc )
            {
               seq.setCachedValue( nextValue + fetchInc ) ;
            }
            else
            {
               INT64 newCachedValue = seq.getCachedValue() +
                  ( seq.getCurrentValue() - seq.getCachedValue() ) / seq.getIncrement() * seq.getIncrement() ;
               seq.setCachedValue( newCachedValue ) ;
            }
         }

         acquirer.nextValue = nextValue ;
         acquirer.acquireSize = ( seq.getCachedValue() - nextValue ) / seq.getIncrement() + 1 ;
         acquirer.increment = seq.getIncrement() ;
         needUpdate = TRUE ;
      }

      if ( seq.isInitial() )
      {
         seq.setInitial( FALSE ) ;
      }

      if ( seq.isExceeded() )
      {
         seq.setExceeded( FALSE ) ;
      }

      // reach the minValue, so mark exceeded
      if ( seq.getCachedValue() == seq.getMinValue() ||
           seq.getCachedValue() < seq.getMinValue() - seq.getIncrement() )
      {
         seq.setExceeded( TRUE ) ;
         needUpdate = TRUE ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__ACQUIRE_DESCENDING_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ADJUST_SEQUENCE, "_catSequenceManager::_adjustSequence::operator" )
   INT32 _catSequenceManager::_adjustSequence::operator()(
         _catSequenceManager *pMgr,
         _catSequence& seq,
         _pmdEDUCB* eduCB,
         INT16 w )
   {
      INT32 rc = SDB_OK ;
      INT32 adjustRC = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ADJUST_SEQUENCE ) ;

      BOOLEAN needUpdate = FALSE ;
      if ( seq.getIncrement() > 0 )
      {
         adjustRC = _adjustAscendingSequence( seq, _expectValue, needUpdate ) ;
      }
      else
      {
         adjustRC = _adjustDecendingSequence( seq, _expectValue, needUpdate ) ;
      }

      if ( needUpdate )
      {
         BSONObj options ;
         try
         {
            BSONObjBuilder ob( 64 ) ;
            ob.append( CAT_SEQUENCE_CURRENT_VALUE, seq.getCurrentValue() ) ;
            ob.append( CAT_SEQUENCE_INITIAL, (bool) seq.isInitial() ) ;
            options = ob.obj() ;
         }
         catch( std::exception& e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Failed to build bson, exception: %s, rc=%d",
                    e.what(), rc ) ;
            goto error ;
         }

         rc = pMgr->_updateSequence( seq.getName(), options, eduCB, w ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to update sequence[%s], rc=%d",
                    seq.getName().c_str(), rc ) ;
            goto error ;
         }
      }

      rc = adjustRC ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( _pAlteredSeqID != NULL )
      {
         *_pAlteredSeqID = seq.getID() ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__ADJUST_SEQUENCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ADJUST_ASCENDING_SEQ, "_catSequenceManager::_adjustSequence::_adjustAscendingSequence" )
   INT32 _catSequenceManager::_adjustSequence::_adjustAscendingSequence(
         _catSequence &seq,
         INT64 expectValue,
         BOOLEAN &needUpdate )
   {
      /*
         adjust rule:
         |-----A-----|-----B-----|-----C-----|-----D-----|
                 nextValue  currentValue  maxValue

         A: expectValue < nextValue
            value has been used
         B: nextValue <= expectValue < currentValue
            adjust nextValue to be greater than expectValue
         C: currentValue <= expectValue < maxValue
            adjust currentValue to be greater than expectValue
         D: maxValue <= expectValue
            mark sequence as exceed
       */
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ADJUST_ASCENDING_SEQ ) ;

      INT32 rc = SDB_OK ;
      UINT64 diff = 0 ;
      UINT64 reminder = 0 ;
      UINT64 diffInc = 0 ;
      INT64 newCachedValue = 0 ;
      INT64 newNextValue = 0 ;

      // When initial, no cache exists. Make the first cache.
      if ( seq.isInitial() )
      {
         INT64 cachedInc = seq.getCacheSize() * (INT64) seq.getIncrement() ;
         cachedInc -= seq.getIncrement() ; // exclude the start value
         seq.setCachedValue( seq.getStartValue() ) ;
         seq.setCurrentValue( seq.getStartValue() + cachedInc ) ;
         seq.setInitial( FALSE ) ;
         needUpdate = TRUE ;
      }

      if ( seq.isExceeded() || seq.getCachedValue() >= expectValue )
      {
         rc = SDB_SEQUENCE_VALUE_USED ;
         goto done ;
      }

      diff = ( UINT64 )( expectValue - seq.getCachedValue() ) ;
      reminder = diff % seq.getIncrement() ;
      diffInc = reminder == 0 ? diff : (diff - reminder + seq.getIncrement()) ;
      newCachedValue = seq.getCachedValue() + diffInc ;
      if ( diffInc < diff || // check for diffInc overflow
           newCachedValue < seq.getCachedValue() || // check for newCachedValue overflow
           newCachedValue > seq.getMaxValue() - seq.getIncrement() )
      {
         seq.setCachedValue( seq.getMaxValue() ) ;
         seq.setCurrentValue( seq.getMaxValue() ) ;
         seq.setExceeded( TRUE ) ;
         needUpdate = TRUE ;
         goto done ;
      }

      newNextValue = newCachedValue + seq.getIncrement() ;
      if ( newNextValue <= seq.getCurrentValue() )
      {
         seq.setCachedValue( newCachedValue ) ;
      }
      else
      {
         seq.setCachedValue( newCachedValue ) ;
         seq.setCurrentValue( newCachedValue ) ;
         needUpdate = TRUE ;
      }

   done:
      PD_TRACE_EXIT ( SDB_GTS_SEQ_MGR__ADJUST_ASCENDING_SEQ ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__ADJUST_DECENDING_SEQ, "_catSequenceManager::_adjustSequence::_adjustDecendingSequence" )
   INT32 _catSequenceManager::_adjustSequence::_adjustDecendingSequence(
         _catSequence& seq,
         INT64 expectValue,
         BOOLEAN& needUpdate )
   {
      /*
         adjust rule:
         |-----A-----|-----B-----|-----C-----|-----D-----|
                 nextValue  currentValue  minValue

         A: expectValue > nextValue
            value has been used
         B: nextValue >= expectValue > currentValue
            adjust nextValue to be less than expectValue
         C: currentValue >= expectValue > minValue
            adjust currentValue to be less than expectValue
         D: minValue <= expectValue
            mark sequence as exceed
       */
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__ADJUST_DECENDING_SEQ ) ;

      INT32 rc = SDB_OK ;
      UINT64 diff = 0 ;
      UINT64 reminder = 0 ;
      UINT64 diffInc = 0 ;
      INT64 newCachedValue = 0 ;
      INT64 newNextValue = 0 ;

      // When initial, no cache exists. Make the first cache.
      if ( seq.isInitial() )
      {
         INT64 cachedInc = seq.getCacheSize() * (INT64) seq.getIncrement() ;
         cachedInc -= seq.getIncrement() ; // exclude the start value
         seq.setCachedValue( seq.getStartValue() ) ;
         seq.setCurrentValue( seq.getStartValue() + cachedInc ) ;
         seq.setInitial( FALSE ) ;
         needUpdate = TRUE ;
      }

      if ( seq.isExceeded() || seq.getCachedValue() <= expectValue )
      {
         rc = SDB_SEQUENCE_VALUE_USED ;
         goto done ;
      }

      diff = ( UINT64 )( seq.getCachedValue() - expectValue ) ;
      reminder = diff % (-seq.getIncrement()) ;
      diffInc = reminder == 0 ? diff : (diff - reminder - seq.getIncrement()) ;
      newCachedValue = seq.getCachedValue() - diffInc ;
      if ( diffInc < diff || // check for diffInc overflow
           newCachedValue > seq.getCachedValue() || // check for newCachedValue overflow
           newCachedValue < seq.getMinValue() - seq.getIncrement() )
      {
         seq.setCachedValue( seq.getMinValue() ) ;
         seq.setCurrentValue( seq.getMinValue() ) ;
         seq.setExceeded( TRUE ) ;
         needUpdate = TRUE ;
         goto done ;
      }

      newNextValue = newCachedValue + seq.getIncrement() ;
      if ( newNextValue >= seq.getCurrentValue() )
      {
         seq.setCachedValue( newCachedValue ) ;
      }
      else
      {
         seq.setCachedValue( newCachedValue ) ;
         seq.setCurrentValue( newCachedValue ) ;
         needUpdate = TRUE ;
      }

   done:
      PD_TRACE_EXIT ( SDB_GTS_SEQ_MGR__ADJUST_DECENDING_SEQ ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__DO_ON_SEQUENCE, "_catSequenceManager::_doOnSequence" )
   INT32 _catSequenceManager::_doOnSequence( const std::string& name,
                                             const utilSequenceID ID,
                                             _pmdEDUCB* eduCB,
                                             INT16 w,
                                             _operateSequence& func )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN noCache = FALSE ;
      utilSequenceID cachedID = UTIL_SEQUENCEID_NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__DO_ON_SEQUENCE ) ;

      rc = _doOnSequenceBySLock( name, ID, eduCB, w, func, noCache, cachedID ) ;
      if ( SDB_OK == rc )
      {
         // if no sequence cache, should get bucket by XLock
         if ( noCache )
         {
            rc = _doOnSequenceByXLock( name, ID, eduCB, w, func ) ;
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
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__DO_ON_SEQUENCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_SLOCK, "_catSequenceManager::_doOnSequenceBySLock" )
   INT32 _catSequenceManager::_doOnSequenceBySLock( const std::string& name,
                                                    const utilSequenceID ID,
                                                    _pmdEDUCB* eduCB,
                                                    INT16 w,
                                                    _operateSequence& func,
                                                    BOOLEAN& noCache,
                                                    utilSequenceID &cachedSeqID )
   {
      INT32 rc = SDB_OK ;
      _catSequence* cache = NULL ;
      BOOLEAN cacheLocked = FALSE ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_SLOCK ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_SLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         cache = (*iter).second ;
         noCache = FALSE ;
      }
      else
      {
         // no sequence cache, should get bucket by XLock
         noCache = TRUE ;
         goto done ;
      }

      cache->lock() ;
      cacheLocked = TRUE ;

      // check ID
      if ( ID != UTIL_SEQUENCEID_NULL && ID != cache->getID() )
      {
         PD_LOG( PDERROR, "Mismatch ID(%llu) for sequence[%s, %llu]",
                 ID, cache->getName().c_str(), cache->getID() ) ;
         rc = SDB_SEQUENCE_NOT_EXIST ;
         goto error ;
      }

      rc = func( this, *cache, eduCB, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to process sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         // if rc==SDB_SEQUENCE_NOT_EXIST, we should delete the cache
         // here we get SLOCK, so we need to get XLOCK to delete the cache
         // check the ID consistency when delete cache
         cachedSeqID = cache->getID() ;
         goto error ;
      }

   done:
      if ( cacheLocked )
      {
         cache->unlock() ;
         cacheLocked = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_SLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_XLOCK, "_catSequenceManager::_doOnSequenceByXLock" )
   INT32 _catSequenceManager::_doOnSequenceByXLock( const std::string& name,
                                                   const utilSequenceID ID,
                                                   _pmdEDUCB* eduCB,
                                                   INT16 w,
                                                   _operateSequence& func )
   {
      INT32 rc = SDB_OK ;
      _catSequence* cache = NULL ;
      _catSequence* sequence = NULL ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_XLOCK ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         cache = (*iter).second ;
      }
      else
      {
         BSONObj seqObj ;

         rc = _findSequence( name, seqObj, eduCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to find sequence[%s] from system collection, rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         sequence = SDB_OSS_NEW _catSequence( name ) ;
         if ( NULL == sequence )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         rc = sequence->loadOptions( seqObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to set sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         try
         {
            bucket.insert( CAT_SEQ_MAP::value_type( name, sequence ) ) ;
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

      // check id
      if ( ID != UTIL_SEQUENCEID_NULL && ID != cache->getID() )
      {
         PD_LOG( PDERROR, "Mismatch ID(%llu) for sequence[%s, %llu]",
                 ID, cache->getName().c_str(), cache->getID() ) ;
         rc = SDB_SEQUENCE_NOT_EXIST ;
         goto error ;
      }

      rc = func( this, *cache, eduCB, w ) ;
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
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__DO_ON_SEQ_BY_XLOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__INSERT_SEQ, "_catSequenceManager::_insertSequence" )
   INT32 _catSequenceManager::_insertSequence( BSONObj& sequence,
                                               _pmdEDUCB* eduCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__INSERT_SEQ ) ;

      SDB_DMSCB* dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB* dpsCB = pmdGetKRCB()->getDPSCB() ;

      rc = rtnInsert( GTS_SEQUENCE_COLLECTION_NAME,
                      sequence, 1, 0, eduCB,
                      dmsCB, dpsCB, w ) ;
      if ( SDB_OK != rc && SDB_IXM_DUP_KEY != rc )
      {
         BSONObj hint ;
         rtnDelete( GTS_SEQUENCE_COLLECTION_NAME,
                    sequence, hint, 0, eduCB,
                    dmsCB, dpsCB ) ;
         goto error ;
      }
      else if ( SDB_IXM_DUP_KEY == rc )
      {
         rc = SDB_SEQUENCE_EXIST ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__INSERT_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__DELETE_SEQ, "_catSequenceManager::_deleteSequence" )
   INT32 _catSequenceManager::_deleteSequence( const std::string& name,
                                               _pmdEDUCB* eduCB, INT16 w,
                                               utilSequenceID *pDeletedSeqID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__DELETE_SEQ ) ;

      BSONObj matcher ;
      BSONObj selector ;
      BSONObj hint ;
      INT64 contextID = -1 ;
      rtnQueryOptions options ;
      SDB_DMSCB* dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB* dpsCB = pmdGetKRCB()->getDPSCB() ;
      SDB_RTNCB* rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnContextPtr pContext ;
      rtnContextBuf contextBuf ;

      // Use queryAndRemove() to get the sequence id while removing the record
      try
      {
         // 1. Build the option
         BSONObjBuilder matcherOb( 128 ) ;
         matcher = matcherOb.append( CAT_SEQUENCE_NAME, name ).obj() ;
         options.setQuery( matcher ) ;

         BSONObjBuilder selectorOb( 32 ) ;
         selector = selectorOb.append( CAT_SEQUENCE_ID, 0 ).obj() ;
         options.setSelector( selector ) ;

         BSONObjBuilder hintOb( 64 ) ;
         BSONObjBuilder modifyOb( hintOb.subobjStart( FIELD_NAME_MODIFY ) ) ;
         modifyOb.append( FIELD_NAME_OP, FIELD_OP_VALUE_REMOVE ) ;
         modifyOb.appendBool( FIELD_NAME_OP_REMOVE, TRUE ) ;
         modifyOb.done() ;
         hint = hintOb.obj() ;
         options.setHint( hint ) ;

         options.setFlag( FLG_QUERY_MODIFY ) ;
         options.setCLFullName( GTS_SEQUENCE_COLLECTION_NAME ) ;

         // 2. Do query and remove
         rc = rtnQuery( options, eduCB, dmsCB, rtnCB, contextID, &pContext ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to find sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         if ( pContext && pContext->isWrite() )
         {
            pContext->setWriteInfo( dpsCB, w ) ;
         }

         rc = rtnGetMore( contextID, 1, contextBuf, eduCB, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_SEQUENCE_NOT_EXIST ;
            PD_LOG( PDERROR, "Sequence[%s] is not found, rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get sequence[%s], rc=%d",
                      name.c_str(), rc ) ;

         // 3. Process result
         if ( contextBuf.recordNum() != 1 )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Unexpected sequence num[%lld], which should be 1",
                    contextBuf.recordNum() ) ;
            goto error ;
         }

         if ( pDeletedSeqID != NULL )
         {
            BSONObj result( contextBuf.data() ) ;
            INT64 num = 0 ;
            rc = rtnGetNumberLongElement( result, CAT_SEQUENCE_ID, num ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get the sequence id, rc=%d",
                         rc ) ;
            *pDeletedSeqID = (utilSequenceID) num ;
         }

      }
      catch( std::exception& e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to build bson, exception: %s, rc=%d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, eduCB ) ;
      }
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__DELETE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__UPDATE_SEQ, "_catSequenceManager::_updateSequence" )
   INT32 _catSequenceManager::_updateSequence( const std::string& name,
                                               const BSONObj& options,
                                               _pmdEDUCB* eduCB, INT16 w,
                                               utilSequenceID* pUpdatedSeqID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__UPDATE_SEQ ) ;

      BSONObj matcher ;
      BSONObj selector ;
      BSONObj hint ;
      INT64 contextID = -1 ;
      rtnQueryOptions queryOptions ;
      SDB_DMSCB* dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB* dpsCB = pmdGetKRCB()->getDPSCB() ;
      SDB_RTNCB* rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnContextPtr pContext ;
      rtnContextBuf contextBuf ;

      // Use queryAndUpdate() to get the sequence id while updating the record
      try
      {
         // 1. Build the option
         BSONObjBuilder matcherOb( 128 ) ;
         matcher = matcherOb.append( CAT_SEQUENCE_NAME, name ).obj() ;
         queryOptions.setQuery( matcher ) ;

         BSONObjBuilder selectorOb( 32 ) ;
         selector = selectorOb.append( CAT_SEQUENCE_ID, 0 ).obj() ;
         queryOptions.setSelector( selector ) ;

         // { $Modify: { OP: "update", Update: { $set: <options> }, ReturnNew: true } }
         BSONObjBuilder hintOb ;
         BSONObjBuilder modifyOb( hintOb.subobjStart( FIELD_NAME_MODIFY ) ) ;
         modifyOb.append( FIELD_NAME_OP, FIELD_OP_VALUE_UPDATE ) ;
         BSONObjBuilder updateOb( modifyOb.subobjStart( FIELD_NAME_OP_UPDATE ) ) ;
         updateOb.append( "$set", options ) ;
         updateOb.done() ;
         modifyOb.appendBool( FIELD_NAME_RETURNNEW, TRUE ) ;
         modifyOb.done() ;
         hint = hintOb.obj() ;
         queryOptions.setHint( hint ) ;

         queryOptions.setFlag( FLG_QUERY_MODIFY ) ;
         queryOptions.setCLFullName( GTS_SEQUENCE_COLLECTION_NAME ) ;

         // 2. Do query and update
         rc = rtnQuery( queryOptions, eduCB, dmsCB, rtnCB, contextID, &pContext ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to find sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         if ( pContext && pContext->isWrite() )
         {
            pContext->setWriteInfo( dpsCB, w ) ;
         }

         rc = rtnGetMore( contextID, 1, contextBuf, eduCB, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_SEQUENCE_NOT_EXIST ;
            PD_LOG( PDERROR, "Sequence[%s] is not found, rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get sequence[%s], rc=%d",
                      name.c_str(), rc ) ;

         // 3. Parse result
         if ( contextBuf.recordNum() != 1 )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Unexpected sequence num[%lld], which should be 1",
                    contextBuf.recordNum() ) ;
            goto error ;
         }

         if ( pUpdatedSeqID != NULL )
         {
            BSONObj result( contextBuf.data() ) ;
            INT64 num = 0 ;
            rc = rtnGetNumberLongElement( result, CAT_SEQUENCE_ID, num ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get the sequence id, rc=%d",
                         rc ) ;
            *pUpdatedSeqID = (utilSequenceID) num ;
         }

      }
      catch( std::exception& e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to build bson, exception: %s, rc=%d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, eduCB ) ;
      }
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__UPDATE_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__FIND_SEQ, "_catSequenceManager::_findSequence" )
   INT32 _catSequenceManager::_findSequence( const std::string& name,
                                             BSONObj& sequence,
                                             _pmdEDUCB* eduCB )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isExist = FALSE ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObj hint ;
      INT64 contextID = -1 ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__FIND_SEQ ) ;

      SDB_DMSCB* dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB* rtnCB = pmdGetKRCB()->getRTNCB() ;

      try
      {
         matcher = BSON( CAT_SEQUENCE_NAME << name ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build update matcher for sequence[%s], exception=%s",
                 name.c_str(), e.what() ) ;
         goto error ;
      }

      rc = rtnQuery( GTS_SEQUENCE_COLLECTION_NAME,
                     selector, matcher, orderBy, hint, 0, eduCB, 0, 1,
                     dmsCB, rtnCB, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to query sequence[%s], rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

      while( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore ( contextID, 1, buffObj, eduCB, rtnCB ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            PD_LOG( PDERROR, "Failed to get sequence[%s], rc=%d",
                    name.c_str(), rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            sequence = result.copy() ;
            isExist = TRUE ;
            break ;
         }
      }

      if ( FALSE == isExist )
      {
         rc = SDB_SEQUENCE_NOT_EXIST ;
         PD_LOG( PDERROR, "Sequence[%s] is not found, rc=%d",
                 name.c_str(), rc ) ;
         goto error ;
      }

   done:
      if( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, eduCB ) ;
         contextID = -1 ;
      }
      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__FIND_SEQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__REMOVE_CACHE_BY_ID, "_catSequenceManager::_removeCacheByID" )
   BOOLEAN _catSequenceManager::_removeCacheByID( const std::string& name, utilSequenceID ID )
   {
      BOOLEAN removed = FALSE ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__REMOVE_CACHE_BY_ID ) ;

      CAT_SEQ_MAP::Bucket& bucket = _sequenceCache.getBucket( name ) ;
      BUCKET_XLOCK( bucket ) ;

      CAT_SEQ_MAP::map_const_iterator iter = bucket.find( name ) ;
      if ( bucket.end() != iter )
      {
         _catSequence* cache = (*iter).second ;
         // if id not equal, means the cache has been changed, can't delete
         if ( cache->getID() == ID )
         {
            bucket.erase( name ) ;
            SDB_OSS_DEL( cache ) ;
            removed = TRUE ;
         }
      }

      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__REMOVE_CACHE_BY_ID, removed ) ;
      return removed ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_SEQ_MGR__CLEAN_CACHE, "_catSequenceManager::_cleanCache" )
   void _catSequenceManager::_cleanCache( BOOLEAN needFlush )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_SEQ_MGR__CLEAN_CACHE ) ;

      pmdEDUCB* eduCB = pmdGetThreadEDUCB() ;

      for ( CAT_SEQ_MAP::bucket_iterator bucketIt = _sequenceCache.begin() ;
            bucketIt != _sequenceCache.end() ;
            bucketIt++ )
      {
         CAT_SEQ_MAP::Bucket& bucket = *bucketIt ;
         BUCKET_XLOCK( bucket ) ;

         for ( CAT_SEQ_MAP::map_const_iterator it = bucket.begin() ;
               it != bucket.end() ;
               it++ )
         {
            _catSequence* cache = (*it).second ;
            BSONObj options ;
            rc = SDB_OK ;

            if ( needFlush )
            {
               try
               {
                  options = BSON( CAT_SEQUENCE_CURRENT_VALUE << cache->getCachedValue()
                               << CAT_SEQUENCE_INITIAL << (bool) cache->isInitial() ) ;

                  rc = _updateSequence( cache->getName(), options, eduCB, 1 ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDWARNING, "Failed to flush sequence[%s], rc=%d",
                             cache->getName().c_str(), rc ) ;
                  }
               }
               catch( std::exception& e )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDWARNING, "Failed to flush sequence[%s], exception: %s, rc=%d",
                          cache->getName().c_str(), e.what(), rc ) ;
               }
            }

            SDB_OSS_DEL ( cache ) ;
         }

         bucket.clear() ;
      }

      PD_TRACE_EXITRC ( SDB_GTS_SEQ_MGR__CLEAN_CACHE, rc ) ;
   }
}

