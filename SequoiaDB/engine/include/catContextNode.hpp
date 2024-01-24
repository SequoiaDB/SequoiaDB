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

   Source File Name = catContextNode.hpp

   Descriptive Name = RunTime Context of Catalog Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context of Catalog.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef CATCONTEXTNODE_HPP_
#define CATCONTEXTNODE_HPP_

#include "catContext.hpp"

namespace engine
{
   /*
    * _catCtxNodeBase define
    */
   class _catCtxNodeBase : public _catContextBase
   {
   public :
      _catCtxNodeBase ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxNodeBase () ;

   protected :
      virtual INT32 _preExecuteInternal ( _pmdEDUCB *cb, INT16 w )
      { return SDB_OK ; }

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb, INT16 w )
      { return SDB_OK ; }
      virtual INT32 _clearInternal(  _pmdEDUCB *cb, INT16 w  )
      { return SDB_OK ; }

      virtual INT32 _initQuery ( const NET_HANDLE &handle,
                                 MsgHeader *pMsg,
                                 const CHAR *pQuery,
                                 const CHAR *pHint,
                                 _pmdEDUCB *cb ) ;

      INT32 _countNodes ( const CHAR *pCollection,
                          const BSONObj &matcher,
                          UINT64 &count,
                          _pmdEDUCB *cb ) ;

      virtual INT32 _buildP1Reply( bson::BSONObjBuilder &builder ) ;

   protected :
      UINT32 _groupID ;
      BOOLEAN _isLocalConnection ;
   } ;

   /*
    * _catCtxActiveGrp define
    */
   class _catCtxActiveGrp : public _catCtxNodeBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _catCtxActiveGrp )
   public :
      _catCtxActiveGrp ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxActiveGrp () ;

      virtual const CHAR* name() const
      {
         return "CAT_ACTIVE_GROUP" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_ACTIVE_GROUP ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;
   } ;

   typedef class _catCtxActiveGrp catCtxActiveGrp ;

   /*
    * _catCtxShutdownGrp define
    */
   class _catCtxShutdownGrp : public _catCtxNodeBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _catCtxShutdownGrp )
   public :
      _catCtxShutdownGrp ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxShutdownGrp () ;

      virtual const CHAR* name() const
      {
         return "CAT_SHUTDOWN_GROUP" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_SHUTDOWN_GROUP ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;
   } ;

   typedef class _catCtxShutdownGrp catCtxShutdownGrp ;

   /*
    * _catCtxRemoveGrp define
    */
   class _catCtxRemoveGrp : public _catCtxNodeBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _catCtxRemoveGrp )
   public :
      _catCtxRemoveGrp ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxRemoveGrp () ;

      virtual const CHAR* name() const
      {
         return "CAT_REMOVE_GROUP" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_REMOVE_GROUP ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;
   } ;

   typedef class _catCtxRemoveGrp catCtxRemoveGrp ;

   /*
    * _catCtxAlterGrp define
    */
   class _catCtxAlterGrp : public _catCtxNodeBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _catCtxAlterGrp )
   public:
      _catCtxAlterGrp ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxAlterGrp () ;

      virtual const CHAR* name() const
      {
         return "CAT_ALTER_GROUP" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_ALTER_GROUP ;
      }

      INT32 open( MSG_TYPE cmdType,
                  const BSONObj &queryObj,
                  const BSONObj &hintObj,
                  rtnContextBuf &buffObj,
                  _pmdEDUCB *cb ) ;

   protected:
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;

      virtual INT32 _buildP1Reply( BSONObjBuilder &builder ) ;

   private:
      INT32 _setActiveLocation() ;

      INT32 _checkCriticalMode( BOOLEAN isStartMode ) ;

      INT32 _startCriticalMode() ;

      INT32 _stopCriticalMode() ;

      INT32 _checkMaintenanceMode( BOOLEAN isStartMode ) ;

      INT32 _startMaintenanceMode() ;

      INT32 _stopMaintenanceMode() ;

   private:
      BSONObj                _boHint ;
      const CHAR*            _pActionName ;
      BSONObj                _option ;
      clsGroupMode           _grpMode ;
      catNodeManager         *_pCatNodeMgr ;
   } ;

   typedef class _catCtxAlterGrp catCtxAlterGrp ;

   /*
    * _catCtxCreateNode define
    */
   class _catCtxCreateNode : public _catCtxNodeBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _catCtxCreateNode )
   public :
      _catCtxCreateNode ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxCreateNode () ;

      virtual const CHAR* name() const
      {
         return "CAT_CREATE_NODE" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_CREATE_NODE ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb, INT16 w ) ;

      virtual INT32 _buildP2Reply( bson::BSONObjBuilder &builder );

      INT32 _checkLocalHost ( BOOLEAN isLocalHost,
                              BOOLEAN &isValid,
                              _pmdEDUCB *cb ) ;

      std::string _getServiceName ( UINT16 localPort,
                                    MSG_ROUTE_SERVICE_TYPE type ) ;

   protected :
      std::string _hostName ;
      std::string _dbPath ;
      std::string _localSvc ;
      std::string _replSvc ;
      std::string _shardSvc ;
      std::string _cataSvc ;
      std::string _nodeName ;
      UINT16 _nodeID ;
      INT32 _nodeStatus ;
      INT32 _nodeRole ;
      INT32 _groupRole ;
      UINT32 _instanceID ;
   } ;

   typedef class _catCtxCreateNode catCtxCreateNode ;

   /*
    * _catCtxRemoveNode define
    */
   class _catCtxRemoveNode : public _catCtxNodeBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _catCtxRemoveNode )
   public :
      _catCtxRemoveNode ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxRemoveNode () ;

      virtual const CHAR* name() const
      {
         return "CAT_REMOVE_NODE" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_REMOVE_NODE ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;

      INT32 _getRemovedGroupsObj ( const BSONObj &boNodeList,
                                   UINT16 &removeNodeID,
                                   ossPoolString &location ) ;

      INT32 _deactiveGroup ( _pmdEDUCB *cb, INT16 w ) ;

   protected :
      std::string _hostName ;
      std::string _localSvc ;
      std::string _nodeName ;
      ossPoolString _location ;
      INT32 _nodeCount ;
      UINT16 _nodeID ;
      BOOLEAN _forced ;
      BOOLEAN _needDeactive ;
   } ;

   typedef class _catCtxRemoveNode catCtxRemoveNode ;
}

#endif //CATCONTEXTNODE_HPP_

