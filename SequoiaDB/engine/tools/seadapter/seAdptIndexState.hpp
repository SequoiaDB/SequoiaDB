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

   Source File Name = seAdptIndexState.hpp

   Descriptive Name = Index session state.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/24/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_INDEX_STATE_HPP__
#define SEADPT_INDEX_STATE_HPP__

#include "netDef.hpp"
#include "msg.hpp"
#include "pmdObjBase.hpp"
#include "rtnExtOprDef.hpp"
#include "../bson/bson.hpp"

using namespace engine ;
using bson::BSONObj ;
using bson::BSONObjSet ;

namespace seadapter
{
   class _seAdptIndexSession ;

   /*
    * Note: Any change of this enum, please change the function
    * seAdptGetIndexerStateDesp at the same time.
    */
   enum SEADPT_INDEXER_STATE
   {
      CONSULT = 1,      // Consult, find where to start.
      FULL_INDEX,       // Full indexing, fetch data from the original cl.
      INCREMENT_INDEX   // Increment indexing, fetch data from capped cl.
   } ;

   const CHAR* seAdptGetIndexerStateDesp( SEADPT_INDEXER_STATE state ) ;

   //  Indexer state interface, defining processing of all the states.
   class _seAdptIndexerState : public _pmdObjBase
   {
      DECLARE_OBJ_MSG_MAP()
   public:
      _seAdptIndexerState( _seAdptIndexSession *session ) ;
      virtual ~_seAdptIndexerState() {}

      virtual SEADPT_INDEXER_STATE type() const = 0 ;

      // onTimer() will be called by the async session framework if no new
      // message received within more than one second.
      virtual INT32 onTimer( UINT32 interval ) ;

      virtual INT32 handleQueryRes( NET_HANDLE handle, MsgHeader* msg ) ;
      virtual INT32 handleGetmoreRes( NET_HANDLE handle, MsgHeader* msg ) ;
      virtual INT32 handleCatalogRes( NET_HANDLE handle, MsgHeader* msg ) ;
      virtual INT32 handleKillCtxRes( NET_HANDLE handle, MsgHeader* msg ) ;

   protected:
      virtual INT32 _processTimeout() = 0 ;
      virtual INT32 _processQueryRes( INT64 contextID,
                                      INT32 startFrom, INT32 numReturned,
                                      vector<BSONObj> &resultSet ) ;
      virtual INT32 _processGetmoreRes( INT32 flag, INT64 contextID,
                                        INT32 startFrom, INT32 numReturned,
                                        vector<BSONObj> &resultSet ) ;
      virtual INT32 _processCatalogRes( INT64 contextID,
                                        INT32 startFrom, INT32 numReturned,
                                        vector<BSONObj> &resultSet ) ;

      /**
       * Update progress document on search engine(Record with _id of
       * SDBCOMMIT).
       */
      INT32 _updateProgress( UINT32 hashVal ) ;
      INT32 _getProgressInfo( BSONObj &progressInfo ) ;

      // Check if it's still the current index.
      INT32 _validateProgress( const BSONObj &progressInfo,
                               BOOLEAN &valid ) ;

      INT32 _cleanObsoleteContext( NET_HANDLE handle, MsgHeader *msg ) ;
      // Drop index on search engine.
      INT32 _cleanSearchEngine() ;

      virtual const CHAR *_getStepDesp() const = 0 ;

   protected:
      _seAdptIndexSession *_session ;
      BOOLEAN _begin ;
      UINT32 _timeout ;
      UINT16 _retryTimes ;
      BOOLEAN _rebuild ;
      BSONObj _idxDef ;
   } ;
   typedef _seAdptIndexerState seAdptIndexerState ;


   // Concrete state definitions.

   /**
    * Consultation state is the first state when an indexer starts.
    * Its main task is to check the indexing progress of the target text index,
    * and decide what to do next, full indexing or incremental indexing.
    */
   class _seAdptConsultState : public seAdptIndexerState
   {
   public:
      _seAdptConsultState( _seAdptIndexSession *session ) ;
      ~_seAdptConsultState() ;

      SEADPT_INDEXER_STATE type() const ;

   private:
      INT32 _processTimeout() ;
      INT32 _processQueryRes( INT64 contextID, INT32 startFrom,
                              INT32 numReturned, vector<BSONObj> &resultSet ) ;

      const CHAR *_getStepDesp() const ;
      INT32 _consult() ;
      INT32 _queryExpectRecord( INT64 expectLID ) ;
      BOOLEAN _progressMatch( const BSONObj &record ) ;

   private:
      BSONObj _progressInfo ;
   } ;
   typedef _seAdptConsultState seAdptConsultState ;

   // When consultation state decides to do full indexing, it will be done by
   // this state. The steps are as follows:
   // 1. Create index on ES(drop at first if it exists).
   // 2. Pop all the data in the capped collection.
   // 3. Index all data in the original collection.
   // 4. Mark the progress in the SDBCOMMIT record.
   // Actions will be triggered by the timer, starting from cleaning ES.
   class _seAdptFullIndexState: public seAdptIndexerState
   {
      enum _STEP
      {
         CLEAN_ES,
         CRT_ES_IDX,
         CLEAN_DB_P1,
         CLEAN_DB_P2,
         QUERY_CL_VERSION,    // Version is required when query collection data.
         QUERY_DATA
      } ;
   public:
      _seAdptFullIndexState( _seAdptIndexSession *session ) ;
      ~_seAdptFullIndexState() ;

      SEADPT_INDEXER_STATE type() const ;

   private:
      INT32 _processTimeout() ;
      INT32 _processCatalogRes( INT64 contextID,
                                INT32 startFrom, INT32 numReturned,
                                vector<BSONObj> &resultSet ) ;

      INT32 _processQueryRes( INT64 contextID, INT32 startFrom,
                              INT32 numReturned,
                              vector<BSONObj> &resultSet ) ;

      INT32 _processGetmoreRes( INT32 flag, INT64 contextID,
                                INT32 startFrom, INT32 numReturned,
                                vector<BSONObj> &resultSet ) ;

   private:
      const CHAR *_getStepDesp() const ;

      INT32 _crtESIdx() ;

      /**
       * @brief Clean the source capped collection before full indexing. Do that
       * with a pop operation backwards to the first record.
       */
      INT32 _prepareCleanDB() ;

      /**
       * @brief Send pop command to the data node.
       * @param _session
       * @param targetObj The first record in the capped collection.
       */
      INT32 _doCleanDB( const BSONObj &targetObj ) ;

      INT32 _queryCLVersion() ;

      /**
       * @brief Send query request to the original collection.
       */
      INT32 _queryData() ;

      /**
       * @brief Generate options for querying the original collection.
       */
      INT32 _genQueryOptions( BSONObj &query, BSONObj &selector ) ;


      INT32 _getMore( INT64 contextID ) ;

      INT32 _parseRecord( const BSONObj &origRecord, string &finalID,
                          BSONObj &finalRecord ) ;

      INT32 _genRecordByKeySet( BSONObjSet keySet, BSONObj &record ) ;
   private:
      _STEP _step ;
      INT32 _clVersion ;
   } ;
   typedef _seAdptFullIndexState seAdptFullIndexState ;

   // Incremental indexing steps are as follows:
   // 1. Get a batch of data from the capped collection.
   // 2. Parse and reformat these objects for indexing.
   // 3. Index these documents on ES.
   // 4. Pop the capped collection to the "last" done position.
   // 5. Repeate step 1~4.
   class _seAdptIncIndexState : public seAdptIndexerState
   {
      enum _STEP
      {
         QUERY_DATA,
         GETMORE_DATA,
         CLEAN_SRC,
      } ;
   public:
      _seAdptIncIndexState( _seAdptIndexSession *session ) ;
      ~_seAdptIncIndexState() ;

      SEADPT_INDEXER_STATE type() const ;

   private:
      INT32 _processTimeout() ;
      INT32 _processQueryRes( INT64 contextID, INT32 startFrom,
                              INT32 numReturned,
                              vector<BSONObj> &resultSet ) ;

      INT32 _processGetmoreRes( INT32 flag, INT64 contextID,
                                INT32 startFrom, INT32 numReturned,
                                vector<BSONObj> &resultSet ) ;

      const CHAR *_getStepDesp() const ;
      INT32 _genQueryOptions( BSONObj &query, BSONObj &selector ) ;
      INT32 _queryData() ;
      INT32 _getMore( INT64 contextID ) ;

      /**
       * @brief Process documents fetched from capped collection.
       * @param docs Documents to be processed.
       */
      INT32 _processDocuments( const vector<BSONObj> &docs ) ;

      /**
       * @brief Process one document.
       * @param logicalID Logical ID of the record.
       */
      INT32 _processDocument( const BSONObj &document, INT64 &logicalID,
                              BOOLEAN &isRebuildRecord ) ;

      /**
       * @brief Check if the first record is the one we expected. If not, it
       * means the incremental indexing has broken and not able to recover.
       * Index on search engine will be dropped. Full indexing will be started.
       * @param doc First record of data fetched in this round.
       */
      INT32 _consistencyCheck( const BSONObj &doc ) ;

      INT32 _parseRecord( const BSONObj &origObj, _rtnExtOprType &oprType,
                          string &finalID, INT64 &logicalID,
                          BSONObj &sourceObj, string *newFinalID ) ;

      INT32 _cleanData() ;

      BOOLEAN _typeSupport( INT32 type )
      {
         return ( type > engine::RTN_EXT_INVALID && type
                  <= engine::RTN_EXT_DUMMY ) ;
      }

   private:
      _STEP _step ;

      // Whether new data has been fetched from data node.
      BOOLEAN _hasNewData ;
      BOOLEAN _firstBatchData ;
      UINT32 _expectRecHash ;
      INT64 _queryCtxID ;
   } ;
   typedef _seAdptIncIndexState seAdptIncIndexState ;
}

#endif /* SEADPT_INDEX_STATE_HPP__ */
