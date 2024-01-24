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

   Source File Name = catDCManager.hpp

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
#ifndef CAT_DCMANAGER_HPP__
#define CAT_DCMANAGER_HPP__

#include "pmd.hpp"
#include "netDef.hpp"
#include "catDCLogMgr.hpp"
#include "catEventHandler.hpp"
#include "rtnContextBuff.hpp"
#include "dpsLogDef.hpp"
#include "dpsMessageBlock.hpp"

using namespace bson ;

namespace engine
{
   class sdbCatalogueCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;
   class _SDB_DMSCB ;
   class _clsDCMgr ;
   class _clsDCBaseInfo ;

   /*
      _catDCManager define
   */
   class _catDCManager : public SDBObject, public _catEventHandler
   {
   public:
      _catDCManager() ;
      ~_catDCManager() ;

      catDCLogMgr* getLogMgr() { return _pLogMgr ; }

      INT32 init() ;
      INT32 fini() ;

      void  attachCB( _pmdEDUCB *cb ) ;
      void  detachCB( _pmdEDUCB *cb ) ;

      INT32 processMsg( const NET_HANDLE &handle, MsgHeader *pMsg ) ;

      INT32 active() ;
      INT32 deactive() ;

      INT32 updateGlobalAddr() ;

      _clsDCBaseInfo *getDCInfo() { return _pDCBaseInfo ; }

      BOOLEAN isDCActivated() const ;
      BOOLEAN isDCReadonly() const ;
      BOOLEAN isImageEnabled() const ;
      BOOLEAN groupInImage( UINT32 groupID ) ;
      BOOLEAN groupInImage( const string &groupName ) ;

      void    setWritedCommand( BOOLEAN writed ) { _isWritedCmd = writed ; }
      BOOLEAN isWritedCommand() const { return _isWritedCmd ; }

      INT32 updateDCCache()
      {
         return _mapData2DCMgr( _pDCMgr ) ;
      }

      INT32 getCATVersion( UINT32 &version ) ;
      INT32 setCATVersion( UINT32 version ) ;

   public :
      // functions of _catEventHandler
      virtual const CHAR *getHandlerName () { return "catDCManager" ; }
      virtual INT32 onBeginCommand ( MsgHeader *pReqMsg ) ;
      virtual INT32 onEndCommand ( MsgHeader *pReqMsg, INT32 result ) ;

   // message process functions
   protected:
      INT32 processCommandMsg( const NET_HANDLE &handle, MsgHeader *pMsg,
                               BOOLEAN writable ) ;
      INT32 processCmdAlterImage( const NET_HANDLE &handle,
                                  const CHAR *pQuery,
                                  rtnContextBuf &ctxBuff ) ;

   // inner process
   protected:
      INT32 processCmdCreateImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdRemoveImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdAttachImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdEnableImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdDetachImage( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdDisableImage( const NET_HANDLE &handle,
                                    _clsDCMgr *pDCMgr,
                                    const BSONObj &objQuery,
                                    BSONObjBuilder &retObjBuilder) ;
      INT32 processCmdActivate( const NET_HANDLE &handle,
                                _clsDCMgr *pDCMgr,
                                const BSONObj &objQuery,
                                BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdDeactivate( const NET_HANDLE &handle,
                                  _clsDCMgr *pDCMgr,
                                  const BSONObj &objQuery,
                                  BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdEnableReadonly( const NET_HANDLE &handle,
                                      _clsDCMgr *pDCMgr,
                                      const BSONObj &objQuery,
                                      BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdDisableReadonly( const NET_HANDLE &handle,
                                       _clsDCMgr *pDCMgr,
                                       const BSONObj &objQuery,
                                       BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdSetActiveLocation( const NET_HANDLE &handle,
                                         _clsDCMgr *pDCMgr,
                                         const BSONObj &objQuery,
                                         BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdSetLocation( const NET_HANDLE &handle,
                                   _clsDCMgr *pDCMgr,
                                   const BSONObj &objQuery,
                                   BSONObjBuilder &retObjBuilder ) ;
      INT32 processCmdAlterMaintenanceMode( const NET_HANDLE &handle,
                                            _clsDCMgr *pDCMgr,
                                            const BSONObj &objQuery,
                                            BSONObjBuilder &retObjBuilder,
                                            const BOOLEAN &isStartMode ) ;

   protected:
      void  _fillRspHeader( MsgHeader *rspMsg, const MsgHeader *reqMsg ) ;

      INT32 _mapData2DCMgr( _clsDCMgr *pDCMgr ) ;

      void  _dcBaseInfoGroups2Obj( _clsDCBaseInfo *pInfo,
                                   BSONObjBuilder &builder,
                                   const CHAR *pFieldName ) ;

      BOOLEAN _isAddrConflict( const string &addr,
                               const vector< pmdAddrPair > &dstAddr ) ;

      INT32   _checkGroupsValid( map< string, string > &mapGroups,
                                 nodeMgrAgent *pNodeAgent ) ;

   // tool fuctions
   private:
      INT16 _majoritySize() ;
      INT32 _updateGlobalInfo() ;
      INT32 _updateImageInfo() ;

      INT32 _checkMaintenanceMode( const BSONObj &option,
                                   const BSONObj &groupObj,
                                   const UINT32 &groupID,
                                   const BOOLEAN &isStartMode,
                                   clsGroupMode &groupMode,
                                   ossPoolString *pHostName ) ;

      INT32 _buildGroupModeInfo( const BSONObj &groupObj,
                                 const ossPoolString &hostName,
                                 clsGroupMode &groupMode ) ;

   private:
      _SDB_DMSCB                 *_pDmsCB;
      _dpsLogWrapper             *_pDpsCB;
      _SDB_RTNCB                 *_pRtnCB;
      sdbCatalogueCB             *_pCatCB;
      pmdEDUCB                   *_pEduCB;

      _clsDCMgr                  *_pDCMgr ;
      _clsDCBaseInfo             *_pDCBaseInfo ;
      _catDCLogMgr               *_pLogMgr ;

      // for commands
      BOOLEAN                    _isWritedCmd ;
      DPS_LSN                    _lsn ;
      _dpsMessageBlock           _mb ;
      BOOLEAN                    _isActived ;

   } ;
   typedef _catDCManager catDCManager ;

}

#endif // CAT_DCMANAGER_HPP__

