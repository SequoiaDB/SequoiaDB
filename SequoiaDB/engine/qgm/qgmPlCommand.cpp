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

   Source File Name = qgmPlCommand.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlCommand.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "rtnCB.hpp"
#include "dpsLogWrapper.hpp"
#include "coordCB.hpp"
#include "coordFactory.hpp"
#include "coordTransOperator.hpp"
#include "msgMessage.hpp"
#include "qgmBuilder.hpp"
#include "rtnCommandList.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{
   _qgmPlCommand::_qgmPlCommand( INT32 type,
                                 const qgmDbAttr &fullName,
                                 const qgmField &indexName,
                                 const qgmOPFieldVec &indexColumns,
                                 const BSONObj &partition,
                                 BOOLEAN uniqIndex )
   :_qgmPlan( QGM_PLAN_TYPE_COMMAND, _qgmField() )
   {
      _commandType = type ;
      _fullName = fullName ;
      _indexName = indexName ;
      _indexColumns = indexColumns ;
      _uniqIndex = uniqIndex ;
      _contextID = -1 ;
      _initialized = TRUE ;
      if ( !partition.isEmpty() )
      {
         _partition = partition ;
      }
   }

   _qgmPlCommand::~_qgmPlCommand()
   {
      close() ;
   }

   void _qgmPlCommand::close()
   {
      _killContext() ;
      return ;
   }

   string _qgmPlCommand::toString() const
   {
      stringstream ss ;

      ss << "Type:" ;
      if ( SQL_GRAMMAR::CRTCS == _commandType )
      {
         ss << "create collectionspace" << '\n'
            << "Name:" << _fullName.toString() << '\n';
      }
      else if ( SQL_GRAMMAR::DROPCS == _commandType )
      {
         ss << "drop collectionspace" << '\n'
            << "Name:" << _fullName.toString() << '\n';
      }
      else if ( SQL_GRAMMAR::CRTCL == _commandType )
      {
         ss << "create collection" << '\n'
            << "Name:" << _fullName.toString() << '\n';
         if ( !_partition.isEmpty() )
         {
            ss << "Shardingkey:" << _partition.toString() << '\n';
         }
      }
      else if ( SQL_GRAMMAR::DROPCL == _commandType )
      {
         ss << "drop collection" << '\n'
            << "Name:" << _fullName.toString() << '\n';
      }
      else if ( SQL_GRAMMAR::CRTINDEX == _commandType )
      {
         ss << "create index" << '\n'
            << "Name:" << _fullName.toString() << '\n'
            << "Index:" << _indexName.toString() << '\n'
            << "Columns:" << qgmBuilder::buildOrderby( _indexColumns )
                               .toString() << '\n' ;
         if ( _uniqIndex )
         {
            ss <<  "Unique:true" << '\n';
         }
         else
         {
            ss <<  "Unique:false" << '\n';
         }
      }
      else if ( SQL_GRAMMAR::DROPINDEX == _commandType )
      {
         ss << "drop index" << '\n'
            << "Name:" << _fullName.toString() << '\n'
            << "Index:" << _indexName.toString() << '\n';
      }
      else if ( SQL_GRAMMAR::LISTCL == _commandType )
      {
         ss << "list collections" << '\n';
      }
      else if ( SQL_GRAMMAR::LISTCS == _commandType )
      {
         ss << "list collectinospaces" << '\n';
      }
      else if ( SQL_GRAMMAR::BEGINTRAN == _commandType )
      {
         ss << "begin transaction" << '\n' ;
      }
      else if ( SQL_GRAMMAR::ROLLBACK == _commandType )
      {
         ss << "rollback\n" ;
      }
      else if ( SQL_GRAMMAR::COMMIT == _commandType )
      {
         ss <<"commit\n" ;
      }
      else
      {
         /// do noting.
      }

      return ss.str() ;
   }

   BOOLEAN _qgmPlCommand::needRollback() const
   {
      if ( SQL_GRAMMAR::COMMIT == _commandType )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   void _qgmPlCommand::buildRetInfo( BSONObjBuilder &builder ) const
   {
      if ( SQL_GRAMMAR::CRTINDEX == _commandType )
      {
         _wrResult.toBSON( builder ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLCOMMAND__EXEC, "_qgmPlCommand::_execute" )
   INT32 _qgmPlCommand::_execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLCOMMAND__EXEC ) ;
      INT32 rc = SDB_OK ;
      SDB_ROLE role = pmdGetKRCB()->getDBRole();

      rc = SDB_ROLE_COORD == role?
           _executeOnCoord( eduCB ) : _executeOnData( eduCB ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPLCOMMAND__EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLCOMMAND_EXECONCOORD, "_qgmPlCommand::_executeOnCoord" )
   INT32 _qgmPlCommand::_executeOnCoord( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLCOMMAND_EXECONCOORD ) ;
      INT32 rc = SDB_OK ;

      CoordCB *pCoord = pmdGetKRCB()->getCoordCB() ;
      coordCommandFactory *pFactory = coordGetFactory() ;
      coordOperator *pOpr = NULL ;

      CHAR *msg = NULL ;
      INT32 bufSize = 0 ;
      const CHAR *pCommand = NULL ;
      rtnContextBuf buff ;

      if ( SQL_GRAMMAR::CRTCS == _commandType )
      {
         pCommand = CMD_NAME_CREATE_COLLECTIONSPACE ;
         BSONObj obj = BSON( FIELD_NAME_NAME << _fullName.toString() ) ;
         rc = msgBuildQueryMsg( &msg, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE,
                                0, 0, 0, -1,
                                &obj, NULL, NULL, NULL,
                                eduCB ) ;
      }
      else if ( SQL_GRAMMAR::DROPCS == _commandType )
      {
         pCommand = CMD_NAME_DROP_COLLECTIONSPACE ;
         BSONObj obj = BSON( FIELD_NAME_NAME << _fullName.toString() ) ;
         rc = msgBuildQueryMsg( &msg, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE,
                                0, 0, 0, -1,
                                &obj, NULL, NULL, NULL,
                                eduCB ) ;
      }
      else if ( SQL_GRAMMAR::CRTCL == _commandType )
      {
         pCommand = CMD_NAME_CREATE_COLLECTION ;
         BSONObj obj ;
         if ( _partition.isEmpty() )
         {
            obj = BSON( FIELD_NAME_NAME << _fullName.toString() ) ;
         }
         else
         {
            obj = BSON( FIELD_NAME_NAME << _fullName.toString() <<
                        FIELD_NAME_SHARDINGKEY << _partition ) ;
         }
         rc = msgBuildQueryMsg( &msg, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION,
                                0, 0, 0, -1,
                                &obj, NULL, NULL, NULL,
                                eduCB ) ;
      }
      else if ( SQL_GRAMMAR::DROPCL == _commandType )
      {
         pCommand = CMD_NAME_DROP_COLLECTION ;
         BSONObj obj = BSON( FIELD_NAME_NAME << _fullName.toString() ) ;
         rc = msgBuildQueryMsg( &msg, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION,
                                0, 0, 0, -1,
                                &obj, NULL, NULL, NULL,
                                eduCB ) ;
      }
      else if ( SQL_GRAMMAR::CRTINDEX == _commandType )
      {
         pCommand = CMD_NAME_CREATE_INDEX ;
         BSONObjBuilder builder ;
         qgmOPFieldVec::const_iterator itr = _indexColumns.begin() ;
         for ( ; itr != _indexColumns.end(); itr++ )
         {
            builder.append( itr->value.attr().toString(),
                            SQL_GRAMMAR::ASC == itr->type?
                            1 : -1 ) ;
         }

         BSONObj index ;
         if ( !_uniqIndex )
         {
            index = BSON( IXM_FIELD_NAME_KEY << builder.obj() <<
                          IXM_FIELD_NAME_NAME << _indexName.toString() ) ;
         }
         else
         {
            BSONObjBuilder indexBuilder ;
            indexBuilder.append( IXM_FIELD_NAME_KEY, builder.obj()) ;
            indexBuilder.append( IXM_FIELD_NAME_NAME, _indexName.toString()) ;
            indexBuilder.appendBool( IXM_FIELD_NAME_UNIQUE, TRUE ) ;
            index = indexBuilder.obj() ;
         }

         BSONObj obj = BSON( FIELD_NAME_COLLECTION << _fullName.toString() <<
                             FIELD_NAME_INDEX << index ) ;
         rc = msgBuildQueryMsg( &msg, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX,
                                0, 0, 0, -1,
                                &obj, NULL, NULL, NULL,
                                eduCB ) ;
      }
      else if ( SQL_GRAMMAR::DROPINDEX == _commandType )
      {
         pCommand = CMD_NAME_DROP_INDEX ;
         BSONObj obj = BSON( FIELD_NAME_COLLECTION << _fullName.toString() <<
                             FIELD_NAME_INDEX <<
                                 BSON( IXM_FIELD_NAME_NAME << _indexName.toString() <<
                                       IXM_FIELD_NAME_KEY << "" )
                            ) ;
         rc = msgBuildQueryMsg( &msg, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX,
                                0, 0, 0, -1,
                                &obj, NULL, NULL, NULL,
                                eduCB ) ;
      }
      else if ( SQL_GRAMMAR::LISTCS == _commandType )
      {
         pCommand = CMD_NAME_LIST_COLLECTIONSPACES ;
         BSONObj obj ;
         rc = msgBuildQueryMsg( &msg, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONSPACES,
                                0, 0, 0, -1,
                                &obj, NULL, NULL, NULL,
                                eduCB ) ;
      }
      else if ( SQL_GRAMMAR::LISTCL == _commandType )
      {
         pCommand = CMD_NAME_LIST_COLLECTIONS ;
         BSONObj obj ;
         rc = msgBuildQueryMsg( &msg, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONS,
                                0, 0, 0, -1,
                                &obj, NULL, NULL, NULL,
                                eduCB ) ;
      }
      else if ( SQL_GRAMMAR::BEGINTRAN == _commandType )
      {
         coordTransBegin opr ;
         MsgOpTransBegin transMsg ;
         transMsg.header.messageLength = sizeof( MsgOpTransBegin ) ;
         transMsg.header.opCode = MSG_BS_TRANS_BEGIN_REQ ;
         transMsg.header.TID = 0 ;
         transMsg.header.routeID.value = 0 ;

         rc = opr.init( pCoord->getResource(), eduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }
         rc = opr.execute( ( MsgHeader *)&transMsg, eduCB,
                           _contextID, &buff ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }
      }
      else if ( SQL_GRAMMAR::ROLLBACK == _commandType )
      {
         coordTransRollback opr ;
         MsgOpTransRollback transMsg ;
         transMsg.header.messageLength = sizeof( MsgOpTransRollback ) ;
         transMsg.header.opCode = MSG_BS_TRANS_ROLLBACK_REQ ;
         transMsg.header.TID = 0 ;
         transMsg.header.routeID.value = 0 ;

         rc = opr.init( pCoord->getResource(), eduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }
         rc = opr.execute( ( MsgHeader *)&transMsg, eduCB,
                           _contextID, &buff ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }
      }
      else if ( SQL_GRAMMAR::COMMIT == _commandType )
      {
         coordTransCommit opr ;
         MsgOpTransCommit transMsg ;
         transMsg.header.messageLength = sizeof( MsgOpTransCommit ) ;
         transMsg.header.opCode = MSG_BS_TRANS_COMMIT_REQ ;
         transMsg.header.TID = 0 ;
         transMsg.header.routeID.value = 0 ;

         rc = opr.init( pCoord->getResource(), eduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }
         rc = opr.execute( ( MsgHeader *)&transMsg, eduCB,
                           _contextID, &buff ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "Invalid command type:%d", _commandType ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( rc )
      {
         PD_LOG( PDERROR, "Build message failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( pCommand )
      {
         rc = pFactory->create( pCommand, pOpr ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                    pCommand, rc ) ;
            goto error ;
         }
         rc = pOpr->init( pCoord->getResource(), eduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    pOpr->getName(), rc ) ;
            goto error ;
         }
         SDB_ASSERT( msg, "Msg cant' be NULL" ) ;
         if ( !msg )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         rc = pOpr->execute( (MsgHeader*)msg, eduCB, _contextID, &buff ) ;
         if ( buff.recordNum() == 1 )
         {
            BSONObj tmpResult ;
            buff.nextObj( tmpResult ) ;
            _wrResult.setResultObj( tmpResult ) ;
         }

         if ( rc )
         {
            PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                    pOpr->getName(), rc ) ;
            goto error ;
         }
      }

   done:
      if ( NULL != msg )
      {
         msgReleaseBuffer( msg, eduCB ) ;
      }
      if ( pOpr )
      {
         pFactory->release( pOpr ) ;
      }
      PD_TRACE_EXITRC( SDB__QGMPLCOMMAND_EXECONCOORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLCOMMAND__EXECONDATA, "_qgmPlCommand::_executeOnData" )
   INT32 _qgmPlCommand::_executeOnData( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLCOMMAND__EXECONDATA ) ;
      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb = pmdGetKRCB();
      SDB_DMSCB *dmsCB = pKrcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = pKrcb->getDPSCB() ;
      SDB_RTNCB *rtnCB = pKrcb->getRTNCB() ;

      if ( dpsCB && eduCB->isFromLocal() && !dpsCB->isLogLocal() )
      {
         dpsCB = NULL ;
      }

      if ( SQL_GRAMMAR::CRTCS == _commandType )
      {
         rc = rtnCreateCollectionSpaceCommand( _fullName.toString().c_str(),
                                               eduCB, dmsCB, dpsCB,
                                               UTIL_CSUNIQUEID_LOCAL ) ;
      }
      else if ( SQL_GRAMMAR::DROPCS == _commandType )
      {
         rc = rtnDropCollectionSpaceCommand( _fullName.toString().c_str(),
                                             eduCB, dmsCB, dpsCB ) ;
      }
      else if ( SQL_GRAMMAR::CRTCL == _commandType )
      {
         if ( _partition.isEmpty() )
         {
            // pass 0 for attributes, which indicates no-compression for the
            // moment
            rc = rtnCreateCollectionCommand( _fullName.toString().c_str(),
                                             0, eduCB, dmsCB, dpsCB,
                                             UTIL_CLUNIQUEID_LOCAL ) ;
         }
         else
         {
            rc = rtnCreateCollectionCommand( _fullName.toString().c_str(),
                                             _partition, 0,
                                             eduCB, dmsCB, dpsCB,
                                             UTIL_CLUNIQUEID_LOCAL ) ;
         }
      }
      else if ( SQL_GRAMMAR::DROPCL == _commandType )
      {
         rc = rtnDropCollectionCommand( _fullName.toString().c_str(),
                                        eduCB, dmsCB, dpsCB ) ;
      }
      else if ( SQL_GRAMMAR::CRTINDEX == _commandType )
      {
         BSONObjBuilder builder ;
         BSONObj index ;
         qgmOPFieldVec::const_iterator itr = _indexColumns.begin() ;
         for ( ; itr != _indexColumns.end(); itr++ )
         {
            builder.append( itr->value.attr().toString(),
                            SQL_GRAMMAR::ASC == itr->type?
                            1 : -1 ) ;
         }
         index = builder.obj() ;
         if ( !_uniqIndex )
         {
            rc = rtnCreateIndexCommand( _fullName.toString().c_str(),
                                        BSON( IXM_FIELD_NAME_KEY << index <<
                                              IXM_FIELD_NAME_NAME << _indexName.toString() ),
                                        eduCB, dmsCB, dpsCB ) ;
         }
         else
         {
            BSONObjBuilder indexBuilder ;
            indexBuilder.append( IXM_FIELD_NAME_KEY, index ) ;
            indexBuilder.append( IXM_FIELD_NAME_NAME, _indexName.toString() ) ;
            indexBuilder.appendBool( IXM_FIELD_NAME_UNIQUE, TRUE ) ;
            rc =  rtnCreateIndexCommand( _fullName.toString().c_str(),
                                         indexBuilder.obj(),
                                         eduCB, dmsCB, dpsCB,
                                         FALSE,
                                         SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                         &_wrResult ) ;
         }
      }
      else if ( SQL_GRAMMAR::DROPINDEX == _commandType )
      {
         BSONObj identifier = BSON( IXM_FIELD_NAME_NAME << _indexName.toString() ) ;
         BSONElement ele = identifier.firstElement() ;
         rc = rtnDropIndexCommand( _fullName.toString().c_str(),
                                   ele, eduCB, dmsCB, dpsCB ) ;
      }
      else if ( SQL_GRAMMAR::LISTCS == _commandType )
      {
         BSONObj empty ;
         _rtnListCollectionspacesInner cmdListCS ;
         rc = cmdListCS.init( 0, 0, -1, empty.objdata(), empty.objdata(),
                              empty.objdata(), empty.objdata() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init command[list collectionspace] failed, "
                    "rc: %d", rc ) ;
            goto error ;
         }
         rc = cmdListCS.doit( eduCB, dmsCB, rtnCB, dpsCB, 1, &_contextID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Run command[list collectionspace] failed, "
                    "rc: %d", rc ) ;
            goto error ;
         }
      }
      else if ( SQL_GRAMMAR::LISTCL == _commandType )
      {
         BSONObj empty ;
         _rtnListCollections cmdListCL ;
         rc = cmdListCL.init( 0, 0, -1, empty.objdata(),
                              empty.objdata(), empty.objdata(),
                              empty.objdata() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init command[list collection] failed, rc: %d",
                    rc ) ;
            goto error ;
         }
         rc = cmdListCL.doit( eduCB, dmsCB, rtnCB, dpsCB, 1, &_contextID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Run command[list collection] failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      else if ( SQL_GRAMMAR::BEGINTRAN == _commandType )
      {
         rc = _checkTransOperator( dpsCB ? TRUE : FALSE ) ;
         if ( SDB_OK == rc )
         {
            if ( eduCB->isAutoCommitTrans() )
            {
               rc = SDB_RTN_ALREADY_IN_AUTO_TRANS ;
               PD_LOG( PDWARNING, "Already in autocommit transaction, rc: %d",
                       rc ) ;
            }
            else
            {
               rc = rtnTransBegin( eduCB ) ;
            }
         }
      }
      else if ( SQL_GRAMMAR::ROLLBACK == _commandType )
      {
         if ( eduCB->isTransaction() )
         {
            rc = rtnTransRollback( eduCB, dpsCB ) ;
         }
      }
      else if ( SQL_GRAMMAR::COMMIT == _commandType )
      {
         if ( eduCB->isTransaction() )
         {
            rc = rtnTransCommit( eduCB, dpsCB ) ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "invalid command type:%d", _commandType ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLCOMMAND__EXECONDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLCOMMAND__FETCHNEXT, "_qgmPlCommand::_fetchNext" )
   INT32 _qgmPlCommand::_fetchNext( qgmFetchOut &next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLCOMMAND__FETCHNEXT ) ;
      INT32 rc = SDB_OK ;

      rtnContextBuf buffObj ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = rtnGetMore( _contextID, 1, buffObj, _eduCB, rtnCB ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to getmore from non-coord, rc = %d",
                    rc) ;
         }
         goto error ;
      }

      try
      {
         next.obj = BSONObj( buffObj.data() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexcepted err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPLCOMMAND__FETCHNEXT, rc ) ;
      return rc ;
   error:
      if ( SDB_DMS_EOC == rc )
         _contextID = -1 ;
      goto done ;
   }

   void _qgmPlCommand::_killContext()
   {
      if ( -1 != _contextID )
      {
         SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
         rtnKillContexts( 1, &_contextID, _eduCB, rtnCB ) ;
         _contextID = -1 ;
      }
      return ;
   }
}

