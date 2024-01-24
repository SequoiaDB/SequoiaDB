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

   Source File Name = clsAdapterJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/07/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsAdapterJob.hpp"
#include "clsMgr.hpp"
#include "rtnCB.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

using namespace bson ;

#define CLS_NAME_CAPPED_COLLECTION  "CappedCL"

namespace engine
{

   /*
      _clsAdapterJobBase implement
    */
   _clsAdapterJobBase::_clsAdapterJobBase ( const MsgHeader * requestHeader,
                                            const bson::BSONObj & requestObject,
                                            const NET_HANDLE & handle )
   : _rtnBaseJob(),
     _handle( handle )
   {
      SDB_ASSERT( NULL != requestHeader, "request message is invalid" ) ;
      _requestHeader = ( *requestHeader ) ;
      _requestObject = requestObject.getOwned() ;
      _makeName() ;
   }

   _clsAdapterJobBase::~_clsAdapterJobBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSADAPTERJOBBASE_DOIT, "_clsAdapterJobBase::doit" )
   INT32 _clsAdapterJobBase::doit ()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSADAPTERJOBBASE_DOIT ) ;

      PD_LOG( PDINFO, "%s: Start adapter job", name() ) ;

      BSONObjBuilder replyBuilder ;
      BSONObj replyObject ;

      try
      {
         rc = _handleMessage( replyBuilder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "%s: Failed to handle message, rc: %d",
                    name(), rc ) ;
         }

         replyObject = replyBuilder.done() ;
      }
      catch ( std::exception & e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "%s: Failed to handle message, exception: %s",
                 name(), e.what() ) ;
      }

      rc = sdbGetShardCB()->replyToRemoteEndpoint(
                                    _handle, &_requestHeader, replyObject ) ;
      PD_RC_CHECK( rc, PDERROR, "%s: Failed to send reply message to "
                   "handle: %u groupID: %u nodeID: %u serviceID: %u",
                   name(), _handle, _requestHeader.routeID.columns.groupID,
                   _requestHeader.routeID.columns.nodeID,
                   _requestHeader.routeID.columns.serviceID ) ;

      PD_LOG( PDINFO, "%s: Finish adapter job", name() ) ;

   done :
      PD_TRACE_EXITRC( SDB__CLSADAPTERJOBBASE_DOIT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _clsAdapterDumpTextIndexJob implement
    */
   _clsAdapterDumpTextIndexJob::_clsAdapterDumpTextIndexJob (
                                       const MsgHeader * requestHeader,
                                       const BSONObj & requestObject,
                                       INT64 peerVersion,
                                       INT64 localVersion,
                                       const NET_HANDLE & handle )
   : _clsAdapterJobBase( requestHeader,
                         requestObject,
                         handle ),
     _peerVersion( peerVersion ),
     _localVersion( localVersion )
   {
   }

   _clsAdapterDumpTextIndexJob::~_clsAdapterDumpTextIndexJob ()
   {
   }

   BOOLEAN _clsAdapterDumpTextIndexJob::muteXOn ( const _rtnBaseJob * other )
   {
      // not the same job type
      if ( RTN_JOB_CLS_ADAPTER_TEXT_INDEX != other->type() )
      {
         return FALSE ;
      }
      // dumping index information acquires lots of SU and mbContext locks,
      // exclude other index information requests to avoid avalanche
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSADAPTERTEXTINDEXJOB__HLDMSG, "_clsAdapterDumpTextIndexJob::_handleMessage" )
   INT32 _clsAdapterDumpTextIndexJob::_handleMessage (
                                                BSONObjBuilder & replyBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSADAPTERTEXTINDEXJOB__HLDMSG ) ;

      // Dump all text indices information, together with the node role and
      // index text version.
      // The text indices information will only be dumped when the node is
      // primary. So the adapter connection to slavery node will get none text
      // index information, and no indexing job will be started.

      // The result object is as follows:
      // {
      //    "IsPrimary" : TRUE | FALSE,
      //    "Version" : version_number,
      //    "Indexes" :
      //    [
      //       {
      //          "Collection" : collection_full_name,
      //          "CappedCL"   : capped_collection_full_name,
      //          "Index"      :
      //          {
      //             index_definition
      //          },
      //          "LogicalID"  : [ cl_logical_id, index_logical_id ]
      //       },
      //       ...
      //    ],
      //    "TotalCount" : index_count
      // }

      replyBuilder.appendBool( FIELD_NAME_IS_PRIMARY, pmdIsPrimary() ) ;
      replyBuilder.append( FIELD_NAME_VERSION, _localVersion ) ;

      // if versions are not the same, all text indices will be returned
      if ( _localVersion != _peerVersion )
      {
         // Dump text indices information into textIdxInfo.
         INT32 totalTextIndexCount = 0 ;
         rc = _dumpAllTextIndexInfo( replyBuilder, totalTextIndexCount ) ;
         replyBuilder.append( FIELD_NAME_TOTAL_COUNT, totalTextIndexCount ) ;
         PD_LOG( PDDEBUG, "%s: Dump %d text index", name(),
                 totalTextIndexCount ) ;
      }

      PD_TRACE_EXITRC( SDB_CLSADAPTERTEXTINDEXJOB__HLDMSG, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSADAPTERTEXTINDEXJOB__DUMPALLTXTIDXINFO, "_clsAdapterDumpTextIndexJob::_dumpAllTextIndexInfo" )
   INT32 _clsAdapterDumpTextIndexJob::_dumpAllTextIndexInfo (
                                                BSONObjBuilder & replyBuilder,
                                                INT32 & totalTextIndexCount )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSADAPTERTEXTINDEXJOB__DUMPALLTXTIDXINFO ) ;

      totalTextIndexCount = 0 ;

      // Dump text indices information both on primary and slave nodes.
      // The adapters need to cache them for searching.
      // Dump all index information and filter text index ones.
      MON_CS_SIM_LIST csList ;
      sdbGetDMSCB()->dumpInfo( csList, FALSE, TRUE, TRUE ) ;

      BSONArrayBuilder indexBuilder(
                           replyBuilder.subarrayStart( FIELD_NAME_INDEXES ) ) ;
      for ( MON_CS_SIM_LIST::iterator csItr = csList.begin() ;
            csItr != csList.end() ;
            ++ csItr )
      {
         for ( MON_CL_SIM_VEC::const_iterator clItr = csItr->_clList.begin() ;
               clItr != csItr->_clList.end() ;
               ++ clItr )
         {
            for ( MON_IDX_LIST::const_iterator idxItr = clItr->_idxList.begin() ;
                  idxItr != clItr->_idxList.end() ;
                  ++ idxItr )
            {
               UINT16 idxType = IXM_EXTENT_TYPE_NONE ;
               INT32 tmpRC = idxItr->getIndexType( idxType ) ;
               if ( SDB_OK != tmpRC )
               {
                  // Only warning, and process the remaining ones.
                  PD_LOG( PDWARNING, "%s: Failed to get type of index [%s] "
                          "from collection [%s], rc: %d", name(),
                          idxItr->getIndexName(), clItr->_name, tmpRC ) ;
                  continue ;
               }

               if ( IXM_EXTENT_TYPE_TEXT != idxType )
               {
                  // Only dump text indices in normal status.
                  continue ;
               }
               // The engine and the adapter use text version to know
               // changes of text indices. On engine side, the version
               // is increased during rebuilding, before the index is
               // marked as NORMAL. In that case, the adapter should
               // request for the information again in next round. So
               // we send the total text index number. Adapter should
               // check it to know whether all text index information
               // has been sent.
               ++ totalTextIndexCount ;
               if ( IXM_INDEX_FLAG_NORMAL != idxItr->_indexFlag )
               {
                  continue ;
               }

               _dumpTextIndexInfo( *clItr, *idxItr, indexBuilder ) ;
            }
         }
      }
      indexBuilder.done() ;

      PD_TRACE_EXITRC( SDB_CLSADAPTERTEXTINDEXJOB__DUMPALLTXTIDXINFO, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSADAPTERTEXTINDEXJOB__DUMPTXTIDXINFO, "_clsAdapterDumpTextIndexJob::_dumpTextIndexInfo" )
   void _clsAdapterDumpTextIndexJob::_dumpTextIndexInfo (
                                                   const monCLSimple & clInfo,
                                                   const monIndex & idxInfo,
                                                   BSONArrayBuilder & idxBuilder )
   {
      PD_TRACE_ENTRY( SDB_CLSADAPTERTEXTINDEXJOB__DUMPTXTIDXINFO ) ;

      try
      {
         // "extData.extData"
         CHAR cappedCLName[ DMS_MAX_EXT_NAME_SIZE * 2 + 1 + 1 ] = { 0 } ;
         ossSnprintf( cappedCLName, sizeof( cappedCLName ), "%s.%s",
                      idxInfo._extDataName, idxInfo._extDataName ) ;

         BSONObjBuilder subBuilder( idxBuilder.subobjStart() ) ;

         subBuilder.append( FIELD_NAME_COLLECTION, clInfo._name ) ;
         subBuilder.append( CLS_NAME_CAPPED_COLLECTION, cappedCLName ) ;
         subBuilder.appendObject( FIELD_NAME_INDEX,
                                  idxInfo._indexDef.objdata(),
                                  idxInfo._indexDef.objsize() ) ;

         // Append cl unique id and logical ids of cl and index.
         // They are used by the adapter to identify different indices with the
         // same meta data.
         // (1) If cs or cl has been recreated with the same name, the cl unique
         //     id will be different.
         // (2) If collection is truncated, it's logical id will change.
         // (3) If index has been recreated, the index logical id will be
         //     different.
         subBuilder.append( FIELD_NAME_UNIQUEID, (INT64)clInfo._clUniqueID ) ;
         subBuilder.append( FIELD_NAME_LOGICAL_ID, clInfo._logicalID ) ;
         subBuilder.append( FIELD_NAME_INDEXLID, idxInfo._indexLID ) ;

         subBuilder.done() ;
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDWARNING, "%s: Unexpected exception happened: %s", name(),
                 e.what() ) ;
      }

      PD_TRACE_EXIT( SDB_CLSADAPTERTEXTINDEXJOB__DUMPTXTIDXINFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CLSSTARTADAPTERTEXTINDEXJOB, "clsStartAdapterDumpTextIndexJob" )
   INT32 clsStartAdapterDumpTextIndexJob ( const MsgHeader * requestMessage,
                                           const BSONObj & requestObject,
                                           INT64 peerVersion,
                                           INT64 localVersion,
                                           const NET_HANDLE & handle,
                                           EDUID * eduID,
                                           BOOLEAN returnResult )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CLSSTARTADAPTERTEXTINDEXJOB ) ;

      SDB_ASSERT( NULL != requestMessage, "message is invalid" ) ;

      try
      {
         clsAdapterDumpTextIndexJob * job =
               SDB_OSS_NEW clsAdapterDumpTextIndexJob(
                     requestMessage, requestObject, peerVersion, localVersion,
                     handle ) ;
         PD_CHECK( NULL != job, SDB_OOM, error, PDERROR,
                   "Failed to allocate memory for adapter job" ) ;

         rc = rtnGetJobMgr()->startJob( job, RTN_JOB_MUTEX_REUSE, eduID,
                                        returnResult ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start adapter job, rc: %d", rc ) ;
      }
      catch ( std::exception & e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to start adapter job, error: %s",
                 e.what() ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CLSSTARTADAPTERTEXTINDEXJOB, rc ) ;
      return rc ;

   error :
      goto done ;
   }
}
