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

   Source File Name = seAdptAgentSession.cpp

   Descriptive Name = Agent session on search engine adapter.

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
#include "seAdptAgentSession.hpp"
#include "rtnContextBuff.hpp"
#include "msgMessage.hpp"
#include "utilCommon.hpp"

using engine::_pmdEDUCB ;

namespace seadapter
{
   BEGIN_OBJ_MSG_MAP( _seAdptAgentSession, _pmdAsyncSession)
      ON_MSG( MSG_BS_QUERY_REQ, _onOPMsg )
      ON_MSG( MSG_BS_GETMORE_REQ, _onOPMsg )
   END_OBJ_MSG_MAP()

   _seAdptAgentSession::_seAdptAgentSession( UINT64 sessionID )
   : _pmdAsyncSession( sessionID )
   {
      _seCltMgr = sdbGetSeCltMgr() ;
      _esClt = NULL ;
      _context = NULL ;
   }

   _seAdptAgentSession::~_seAdptAgentSession()
   {
      if ( _esClt )
      {
         _seCltMgr->releaseClt( _esClt ) ;
      }
      if ( _context )
      {
         SDB_OSS_DEL _context ;
      }
   }

   SDB_SESSION_TYPE _seAdptAgentSession::sessionType() const
   {
      return SDB_SESSION_SE_AGENT ;
   }

   EDU_TYPES _seAdptAgentSession::eduType() const
   {
      return EDU_TYPE_SE_AGENT ;
   }

   void _seAdptAgentSession::onRecieve( const NET_HANDLE netHandle,
                                        MsgHeader *msg )
   {
   }

   BOOLEAN _seAdptAgentSession::timeout( UINT32 interval )
   {
      return FALSE ;
   }

   void _seAdptAgentSession::onTimer( UINT64 timerID, UINT32 interval )
   {
   }

   void _seAdptAgentSession::_onAttach()
   {
   }

   void _seAdptAgentSession::_onDetach()
   {
   }

   INT32 _seAdptAgentSession::_onOPMsg( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;

      MsgOpReply reply ;
      BSONObj resultObj;
      const CHAR *msgBody = NULL ;
      INT32 bodySize = 0 ;
      utilCommObjBuff objBuff ;

      rc = objBuff.init() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Initialize result buffer failed[ %d ]", rc ) ;
      }
      else
      {
         switch ( msg->opCode )
         {
            case MSG_BS_QUERY_REQ:
               rc = _onQueryReq( msg, objBuff ) ;
               break ;
            case MSG_BS_GETMORE_REQ:
               rc = _onGetmoreReq( msg, objBuff ) ;
               break ;
            default:
               rc = SDB_UNKNOWN_MESSAGE ;
               break ;
         }
      }

      reply.header.opCode = MAKE_REPLY_TYPE( msg->opCode ) ;
      reply.header.messageLength = sizeof( MsgOpReply ) ;
      reply.header.requestID = msg->requestID ;
      reply.header.TID = msg->TID ;
      reply.header.routeID.value = 0 ;

      if ( objBuff.valid() )
      {
         if ( SDB_OK != rc )
         {
            if ( 0 == objBuff.getObjNum() )
            {
               _errorInfo =
                  utilGetErrorBson( rc, _pEDUCB->getInfo( EDU_INFO_ERROR ) ) ;
               objBuff.appendObj( _errorInfo ) ;
            }
         }

         if ( objBuff.getObjNum() > 0 )
         {
            msgBody = objBuff.data() ;
            bodySize = objBuff.dataSize() ;
            reply.numReturned = objBuff.getObjNum() ;
         }
      }
      else
      {
         reply.numReturned = 0 ;
      }

      reply.flags = rc ;
      reply.header.messageLength += bodySize ;
      rc = _reply( &reply, msgBody, bodySize ) ;
      PD_RC_CHECK( rc, PDERROR, "Reply the message failed[ %d ]" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptAgentSession::_onQueryReq( MsgHeader *msg,
                                           utilCommObjBuff &objBuff,
                                           pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      CHAR *pCollectionName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = 0 ;
      CHAR *pQuery = NULL ;
      CHAR *pFieldSelector = NULL ;
      CHAR *pOrderBy = NULL ;
      CHAR *pHint = NULL ;

      string indexName ;
      string typeName ;
      utilCommObjBuff resultObjs ;
      seIdxMetaMgr *idxMetaCache = NULL ;
      BOOLEAN cacheLocked = FALSE ;

      rc = msgExtractQuery( (CHAR *)msg, &flag, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Session[ %s ] extract query message "
                   "failed[ %d ]", sessionName(), rc ) ;

      try
      {
         BSONObj matcher( pQuery ) ;
         BSONObj selector ( pFieldSelector ) ;
         BSONObj orderBy ( pOrderBy ) ;
         BSONObj hint ( pHint ) ;
         BSONObj newHint ;

         if ( !_esClt )
         {
            rc = _seCltMgr->getClt( &_esClt ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Connect to search engine failed[ %d ]", rc ) ;
               goto error ;
            }
         }

         if ( _context )
         {
            SDB_OSS_DEL _context ;
            _context = NULL ;
         }

         rc = _selectIndex( pCollectionName, hint, newHint,
                            indexName, typeName ) ;
         PD_RC_CHECK( rc, PDERROR, "Select index failed[ %d ]", rc ) ;

         _context = SDB_OSS_NEW seAdptContextQuery( indexName,
                                                    typeName, _esClt ) ;
         if ( !_context )
         {
            rc = SDB_OOM ;
            PD_LOG_MSG( PDERROR, "Allocate memory for query context failed, "
                        "size[ %d ]", sizeof( seAdptContextQuery ) ) ;
            goto error ;
         }

         rc = _context->open( matcher, selector, orderBy,
                              newHint, objBuff, eduCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG_MSG( PDERROR, "Open context for rewrite query failed[ %d ]",
                           rc ) ;
            }
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG_MSG( PDERROR, "Session[ %s ] create BSON objects for query "
                     "items failed: %s", sessionName(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      if ( cacheLocked )
      {
         SDB_ASSERT( idxMetaCache, "Index meta cache should not be NULL" ) ;
         idxMetaCache->unlock( SHARED ) ;
      }
      return rc ;
   error:
      if ( _context )
      {
         SDB_OSS_DEL _context ;
         _context = NULL ;
      }
      goto done ;
   }

   INT32 _seAdptAgentSession::_onGetmoreReq( MsgHeader *msg,
                                             utilCommObjBuff &objBuff,
                                             pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;

      rc = _context->getMore( 1, objBuff ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG_MSG( PDERROR, "Get more rewrite query failed[ %d ]", rc ) ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptAgentSession::_reply( MsgOpReply *header, const CHAR *buff,
                                      UINT32 size )
   {
      INT32 rc = SDB_OK ;

      if ( size > 0 )
      {
         rc = routeAgent()->syncSend( _netHandle, (MsgHeader *)header,
                                      (void *)buff, size ) ;
      }
      else
      {
         rc = routeAgent()->syncSend( _netHandle, (void *)header ) ;
      }

      PD_RC_CHECK( rc, PDERROR, "Session[ %s ] send reply message failed[ %d ]",
                   sessionName(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _seAdptAgentSession::_defaultMsgFunc ( NET_HANDLE handle,
                                                MsgHeader * msg )
   {
      return _onOPMsg( handle, msg ) ;
   }

   INT32 _seAdptAgentSession::_selectIndex( const CHAR *clName,
                                            const BSONObj &hint,
                                            BSONObj &newHint,
                                            string &index,
                                            string &type )
   {
      INT32 rc = SDB_OK ;
      seIdxMetaMgr* idxMetaCache = NULL ;
      IDX_META_VEC idxMetas ;
      BOOLEAN cacheLocked = FALSE ;
      BSONObjBuilder builder ;
      UINT32 textIdxNum = 0 ;

      idxMetaCache = sdbGetSeAdapterCB()->getIdxMetaCache() ;
      idxMetaCache->lock( SHARED ) ;
      cacheLocked = TRUE ;

      rc = idxMetaCache->getIdxMetas( clName, idxMetas ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "No index information found for collection"
                     "[ %s ], rc[ %d ]", clName, rc ) ;
         goto error ;
      }
      if ( 0 == idxMetas.size() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "No index information found for "
                     "collection[ %s ]", clName ) ;
         goto error ;
      }

      try
      {
         BSONObjIterator itr( hint ) ;
         while ( itr.more() )
         {
            string esIdxName ;
            BSONElement ele = itr.next() ;
            string idxName = ele.str() ;
            if ( idxName.empty() )
            {
               continue ;
            }

            if ( _isTextIdx( idxMetas, idxName, esIdxName ) )
            {
               if ( 0 == textIdxNum )
               {
                  index = esIdxName ;
               }
               textIdxNum++ ;
            }
            else
            {
               builder.append( ele ) ;
            }
         }

         if ( 0 == textIdxNum )
         {
            if ( 1 == idxMetas.size() )
            {
               index = idxMetas.front().getEsIdxName() ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Text index name should be specified in "
                           "hint when there are multiple text indices") ;
               goto error ;
            }
         }
         idxMetaCache->unlock() ;
         cacheLocked = FALSE ;

         newHint = builder.obj() ;
         type = sdbGetSeAdapterCB()->getDataNodeGrpName() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "Original hint[ %s ], new hint[ %s ]",
              hint.toString( FALSE, TRUE ).c_str(),
              newHint.toString( FALSE, TRUE ).c_str() ) ;
   done:
      if ( cacheLocked )
      {
         idxMetaCache->unlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _seAdptAgentSession::_isTextIdx( const IDX_META_VEC &idxMetas,
                                            const string &idxName,
                                            string &esIdxName )
   {
      BOOLEAN found = FALSE ;

      for ( IDX_META_VEC_CITR itr = idxMetas.begin(); itr != idxMetas.end();
            ++itr )
      {
         if ( idxName == itr->getOrigIdxName() )
         {
            found = TRUE ;
            esIdxName = itr->getEsIdxName() ;
            break ;
         }
      }

      return found ;
   }
}

