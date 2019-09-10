/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = seAdptIndexSession.hpp

   Descriptive Name = Index session on search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_INDEX_SESSION_HPP_
#define SEADPT_INDEX_SESSION_HPP_

#include "utilESClt.hpp"
#include "pmdAsyncSession.hpp"
#include "dmsExtDataHandler.hpp"
#include "rtnExtOprDef.hpp"
#include "seAdptMgr.hpp"
#include "utilESBulkBuilder.hpp"

using namespace bson ;

#define SEADPT_FIELD_NAME_ID         "_id"

namespace seadapter
{
   enum SEADPT_SESSION_STATUS
   {
      SEADPT_SESSION_STAT_CONSULT = 1,       // To find where to start.
      SEADPT_SESSION_STAT_BEGIN,             // Start from the beginning,
      SEADPT_SESSION_STAT_UPDATE_CL_VERSION, // Update collection version.
      SEADPT_SESSION_STAT_QUERY_LAST_LID,
      SEADPT_SESSION_STAT_COMP_LAST_LID,
      SEADPT_SESSION_STAT_QUERY_NORMAL_TBL,
      SEADPT_SESSION_STAT_QUERY_CAP_TBL,
      SEADPT_SESSION_STAT_POP_CAP,
      SEADPT_SESSION_STAT_MAX
   } ;

   class _seAdptIndexSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()
   public:
      _seAdptIndexSession( UINT64 sessionID, const seIndexMeta *idxMeta ) ;
      virtual ~_seAdptIndexSession() ;

      virtual EDU_TYPES eduType() const ;
      virtual SDB_SESSION_TYPE sessionType() const ;
      virtual const CHAR* className() const { return "Indexer" ; }

      INT32 handleQueryRes( NET_HANDLE handle, MsgHeader* msg ) ;
      INT32 handleGetMoreRes( NET_HANDLE handle, MsgHeader *msg ) ;
      INT32 handleKillCtxRes( NET_HANDLE handle, MsgHeader *msg ) ;

      virtual void onRecieve( const NET_HANDLE netHandle, MsgHeader * msg ) ;
      virtual BOOLEAN timeout( UINT32 interval ) ;

      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;
   protected:
      virtual void _onAttach() ;
      virtual void _onDetach() ;

   private:
      void  _updateCLVersion( INT32 version ) ;
      void  _switchStatus( SEADPT_SESSION_STATUS newStatus ) ;
      INT32 _sendGetmoreReq( INT64 contextID, UINT64 requestID ) ;
      INT32 _queryOrigCollection() ;
      INT32 _queryLastCappedRecLID( BOOLEAN reverse = FALSE ) ;
      INT32 _queryCappedCollection( BSONObj &condition ) ;
      INT32 _cleanData( INT64 recLID ) ;
      INT32 _parseSrcData( const BSONObj &origObj, _rtnExtOprType &oprType,
                           const CHAR **origOID, INT64 &logicalID,
                           BSONObj &sourceObj ) ;
      INT32 _processNormalCLRecords( NET_HANDLE handle, MsgHeader *msg ) ;

      INT32 _processCappedCLRecords( NET_HANDLE handle, MsgHeader *msg ) ;
      INT32 _getLastIndexedLID( NET_HANDLE handle, MsgHeader *msg ) ;
      INT32 _markProgress( BSONObj &infoObj ) ;
      INT32 _updateProgress( INT64 logicalID ) ;

      INT32 _chkDoneMark( BOOLEAN &found ) ;
      INT32 _consult() ;
      INT32 _onSDBEOC() ;
      INT32 _startOver() ;
      void  _setQueryBusyFlag( BOOLEAN busy ) { _queryBusy = busy; }
      BOOLEAN _isQueryBusy() { return _queryBusy ; }
      INT32 _bulkPrepare() ;
      INT32 _bulkProcess( const utilESBulkActionBase &actionItem ) ;
      INT32 _bulkFinish() ;
      INT32 _createIndex( BOOLEAN force = FALSE ) ;
      INT32 _dropIndex() ;

      OSS_INLINE INT32 _findRecWithLID( INT64 logicalID, BOOLEAN &found ) ;

   private:
      string                  _origCLFullName ;
      string                  _cappedCLFullName ;
      INT32                   _origCLVersion ;
      string                  _origIdxName ;
      string                  _indexName ;
      string                  _typeName ;
      BSONObj                 _indexDef ;
      BSONObj                 _queryCond ;
      BSONObj                 _selector ; // Should contain _id and index fields.
      utilESClt               *_esClt ;
      SEADPT_SESSION_STATUS   _status ;
      INT64                   _lastPopLID ;
      BOOLEAN                 _quit ;
      INT64                   _queryCtxID ;
      BOOLEAN                 _queryBusy ;   // Where one query is in progress.
      INT64                   _expectLID ;
      utilESBulkBuilder       _bulkBuilder ;
   } ;
   typedef _seAdptIndexSession seAdptIndexSession ;

   OSS_INLINE INT32 _seAdptIndexSession::_findRecWithLID( INT64 logicalID,
                                                          BOOLEAN &found )
   {
      INT32 rc = SDB_OK ;
      std::ostringstream lidStr ;
      lidStr << logicalID ;

      rc = _esClt->documentExist( _indexName.c_str(), _typeName.c_str(),
                                  SEADPT_FIELD_NAME_ID, lidStr.str().c_str(),
                                  found ) ;
      PD_RC_CHECK( rc, PDERROR, "Document existence check failed[ %d ]", rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
}

#endif /* SEADPT_INDEX_SESSION_HPP_ */

