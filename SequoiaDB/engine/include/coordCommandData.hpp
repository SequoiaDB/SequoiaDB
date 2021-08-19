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

   Source File Name = coordCommandData.hpp

   Descriptive Name = Coord Commands for Data Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMAND_DATA_HPP__
#define COORD_COMMAND_DATA_HPP__

#include "coordCommand2Phase.hpp"
#include "rtnAlterJob.hpp"
#include "clsTask.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordDataCMD2Phase define
   */
   class _coordDataCMD2Phase : public _coordCMD2Phase
   {
      public:
         _coordDataCMD2Phase() ;
         virtual ~_coordDataCMD2Phase() ;
      protected :
         virtual INT32 _generateDataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          const vector<BSONObj> &cataObjs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseDataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  coordCMDArguments *pArgs,
                                                  CHAR **ppMsgBuf,
                                                  INT32 *pBufSize ) ;

         virtual void  _releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                INT32 bufSize,
                                                pmdEDUCB *cb ) ;

         virtual INT32 _doOnCataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord **ppContext,
                                        coordCMDArguments *pArgs,
                                        CoordGroupList *pGroupLst,
                                        vector<BSONObj> *pReplyObjs ) ;

         virtual INT32 _doOnDataGroup ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        rtnContextCoord **ppContext,
                                        coordCMDArguments *pArgs,
                                        const CoordGroupList &groupLst,
                                        const vector<BSONObj> &cataObjs,
                                        CoordGroupList &sucGroupLst ) ;

         virtual INT32 _doAudit ( coordCMDArguments *pArgs, INT32 rc ) ;

      protected :
         /*
            Get list of Data Groups from Catalog with the P1 catalog command
         */
         virtual BOOLEAN _flagGetGrpLstFromCata () { return TRUE ; }

         /*
            command on collection
         */
         virtual BOOLEAN _flagDoOnCollection () { return TRUE ; }

         /*
            update catalog info before send command to Catalog
         */
         virtual BOOLEAN _flagUpdateBeforeCata () { return FALSE ; }

         /*
            update catalog info before send command to Data Groups
         */
         virtual BOOLEAN _flagUpdateBeforeData () { return FALSE ; }

         /*
            whether to use group in Coord cache
         */
         virtual BOOLEAN _flagUseGrpLstInCoord () { return FALSE ; }
   } ;
   typedef _coordDataCMD2Phase coordDataCMD2Phase ;

   /*
      _coordDataCMD3Phase define
   */
   class _coordDataCMD3Phase : public _coordDataCMD2Phase
   {
      public:
         _coordDataCMD3Phase() ;
         virtual ~_coordDataCMD3Phase() ;
      protected :
         virtual INT32 _doOnCataGroupP2 ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnContextCoord **ppContext,
                                          coordCMDArguments *pArgs,
                                          const CoordGroupList &pGroupLst ) ;

         virtual INT32 _doOnDataGroupP2 ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnContextCoord **ppContext,
                                          coordCMDArguments *pArgs,
                                          const CoordGroupList &groupLst,
                                          const vector<BSONObj> &cataObjs ) ;
   } ;
   typedef _coordDataCMD3Phase coordDataCMD3Phase ;

   /*
      _coordAlterCMDArguments define
    */
   class _coordAlterCMDArguments : public _coordCMDArguments
   {
      public :
         _coordAlterCMDArguments () ;
         virtual ~_coordAlterCMDArguments () ;

         OSS_INLINE const rtnAlterTask * getTaskRunner () const
         {
            return _task ;
         }

         OSS_INLINE void setTaskRunner ( const rtnAlterTask * task )
         {
            _task = task ;
            _postTasks.clear() ;
         }

         OSS_INLINE BOOLEAN hasPostTasks () const
         {
            return _postTasks.size() > 0 ;
         }

         INT32 addPostTask ( UINT64 postTask ) ;

         OSS_INLINE const ossPoolList<UINT64> & getPostTasks () const
         {
            return _postTasks ;
         }

         INT32 addPostTaskObj ( const BSONObj &taskObj ) ;

         OSS_INLINE const ossPoolVector<BSONObj> & getPostTasksObj () const
         {
            return _postTasksObj ;
         }

         void clear () ;

      protected :
         const rtnAlterTask *    _task ;
         ossPoolList<UINT64>     _postTasks ;
         ossPoolVector<BSONObj>  _postTasksObj ;
   } ;

   typedef class _coordAlterCMDArguments coordAlterCMDArguments ;

   /*
      _coordDataCMDAlter define
    */
   class _coordDataCMDAlter : public _coordDataCMD3Phase
   {
      public :
         _coordDataCMDAlter () ;
         virtual ~_coordDataCMDAlter () ;

         virtual INT32 execute ( MsgHeader * pMsg,
                                 pmdEDUCB * cb,
                                 INT64 & contextID,
                                 rtnContextBuf * buf ) ;

      protected :
         virtual coordCMDArguments * _getArguments () ;

         virtual INT32 _doOnCataGroup ( MsgHeader * pMsg,
                                        pmdEDUCB * cb,
                                        rtnContextCoord ** ppContext,
                                        coordCMDArguments * pArgs,
                                        CoordGroupList * pGroupLst,
                                        vector<BSONObj> * pReplyObjs ) ;

         virtual INT32 _doOnCataGroupP2 ( MsgHeader * pMsg,
                                          pmdEDUCB * cb,
                                          rtnContextCoord ** ppContext,
                                          coordCMDArguments * pArgs,
                                          const CoordGroupList & groupLst ) ;

         virtual INT32 _doOnDataGroupP2 ( MsgHeader * pMsg,
                                          pmdEDUCB * cb,
                                          rtnContextCoord ** ppContext,
                                          coordCMDArguments * pArgs,
                                          const CoordGroupList & groupLst,
                                          const vector<BSONObj> & cataObjs ) ;

         virtual INT32 _rollbackOnDataGroup ( MsgHeader * pMsg,
                                              pmdEDUCB * cb,
                                              coordCMDArguments * pArgs,
                                              const CoordGroupList & groupLst ) ;

         virtual INT32 _doCommit ( MsgHeader *pMsg,
                                   pmdEDUCB * cb,
                                   rtnContextCoord **ppContext,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _parseMsg ( MsgHeader * pMsg,
                                   coordCMDArguments * pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader * pMsg,
                                          pmdEDUCB * cb,
                                          coordCMDArguments * pArgs,
                                          CHAR ** ppMsgBuf,
                                          INT32 * pBufSize ) ;

         virtual void _releaseCataMsg ( CHAR * pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB * cb ) ;

         virtual RTN_ALTER_OBJECT_TYPE _getObjectType () const = 0 ;
         virtual MSG_TYPE _getCatalogMessageType () const = 0 ;

         virtual INT32 _extractPostTasks ( const bson::BSONObj & reply ) ;
         INT32 _getPostTasksObj ( pmdEDUCB * cb ) ;
         INT32 _checkPostTask( pmdEDUCB *cb, const bson::BSONObj &taskObj ) ;

         virtual INT32 _buildPostTasks ( const ossPoolList< UINT64 > & postTasks,
                                         bson::BSONObj & taskDesc ) ;

         virtual INT32 _executePostTasks ( const CHAR * name,
                                           const ossPoolList< UINT64 > & postTasks,
                                           pmdEDUCB * cb,
                                           CLS_TASK_TYPE *type ) ;

         virtual INT32 _cancelPostTasks ( const CHAR * name,
                                          const ossPoolList< UINT64 > & postTasks,
                                          pmdEDUCB * cb ) ;

      protected :
         coordAlterCMDArguments  _arguments ;
         rtnAlterJob             _alterJob ;
   } ;

   typedef class _coordDataCMDAlter coordDataCMDAlter ;

   /*
      _coordCMDTestCollectionSpace define
   */
   class _coordCMDTestCollectionSpace : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDTestCollectionSpace() ;
         virtual ~_coordCMDTestCollectionSpace() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTestCollectionSpace coordCMDTestCollectionSpace ;

   /*
      _coordCMDTestCollection define
   */
   class _coordCMDTestCollection : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDTestCollection() ;
         virtual ~_coordCMDTestCollection() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTestCollection coordCMDTestCollection ;

   /*
      _coordCmdWaitTask define
   */
   class _coordCmdWaitTask : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdWaitTask() ;
         virtual ~_coordCmdWaitTask() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCmdWaitTask coordCmdWaitTask ;

   /*
      _coordCmdCancelTask define
   */
   class _coordCmdCancelTask : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdCancelTask() ;
         virtual ~_coordCmdCancelTask() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCmdCancelTask coordCmdCancelTask ;

   /*
      _coordCMDTruncate define
   */
   class _coordCMDTruncate : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDTruncate() ;
         virtual ~_coordCMDTruncate() ;
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTruncate coordCMDTruncate ;

   /*
      _coordCMDCreateCollectionSpace define
   */
   class _coordCMDCreateCollectionSpace : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDCreateCollectionSpace() ;
         virtual ~_coordCMDCreateCollectionSpace() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDCreateCollectionSpace coordCMDCreateCollectionSpace ;

   /*
      _coordCMDDropCollectionSpace define
   */
   class _coordCMDDropCollectionSpace : public _coordDataCMD3Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDDropCollectionSpace() ;
         virtual ~_coordCMDDropCollectionSpace() ;
      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _doComplete ( MsgHeader *pMsg,
                                     pmdEDUCB * cb,
                                     coordCMDArguments *pArgs ) ;

      protected :
         /*
            command on collection
         */
         virtual BOOLEAN _flagDoOnCollection () { return FALSE ; }

   } ;
   typedef _coordCMDDropCollectionSpace coordCMDDropCollectionSpace ;

   /*
      _coordCMDRenameCollectionSpace define
   */
   class _coordCMDRenameCollectionSpace : public _coordDataCMD3Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDRenameCollectionSpace() ;
         virtual ~_coordCMDRenameCollectionSpace() ;
      protected :
         virtual INT32 _parseMsg( MsgHeader *pMsg,
                                  coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         coordCMDArguments *pArgs,
                                         CHAR **ppMsgBuf,
                                         INT32 *pBufSize ) ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _doComplete( MsgHeader *pMsg,
                                    pmdEDUCB * cb,
                                    coordCMDArguments *pArgs ) ;

         // Not a collection command
         virtual BOOLEAN _flagDoOnCollection () { return FALSE ; }

   } ;
   typedef _coordCMDRenameCollectionSpace coordCMDRenameCollectionSpace ;

   /*
      _coordCMDAlterCollectionSpace define
    */
   class _coordCMDAlterCollectionSpace : public _coordDataCMDAlter
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

      public :
         _coordCMDAlterCollectionSpace () ;
         virtual ~_coordCMDAlterCollectionSpace () ;

      protected :
         OSS_INLINE virtual RTN_ALTER_OBJECT_TYPE _getObjectType () const
         {
            return RTN_ALTER_COLLECTION_SPACE ;
         }

         OSS_INLINE virtual MSG_TYPE _getCatalogMessageType () const
         {
            return MSG_CAT_ALTER_CS_REQ ;
         }

         // Not a collection command
         virtual BOOLEAN _flagDoOnCollection () { return FALSE ; }
   } ;

   /*
      _coordCMDCreateCollection define
   */
   class _coordCMDCreateCollection : public _coordDataCMD2Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDCreateCollection() ;
         virtual ~_coordCMDCreateCollection() ;

      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  coordCMDArguments *pArgs,
                                                  CHAR **ppMsgBuf,
                                                  INT32 *pBufSize ) ;

         virtual void  _releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                INT32 bufSize,
                                                pmdEDUCB *cb ) ;

         virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              coordCMDArguments *pArgs,
                                              const CoordGroupList &groupLst ) ;
      protected :
         /*
            Commit on Catalog when rollback on Data groups failed
         */
         virtual BOOLEAN _flagCommitOnRollbackFailed () { return TRUE ; }

         /*
            update catalog info before send command to Data Groups
         */
         virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

   } ;
   typedef _coordCMDCreateCollection coordCMDCreateCollection ;

   /*
      _coordCMDDropCollection define
   */
   class _coordCMDDropCollection : public _coordDataCMD3Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDDropCollection() ;
         virtual ~_coordCMDDropCollection() ;
      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _generateDataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          const vector<BSONObj> &cataObjs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseDataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _doComplete ( MsgHeader *pMsg,
                                     pmdEDUCB * cb,
                                     coordCMDArguments *pArgs ) ;

         /*
            use coord cache but not use group list, because split
            will change the version and groups without lock
         */
         virtual BOOLEAN _flagUseGrpLstInCoord () { return TRUE ; }

   } ;
   typedef _coordCMDDropCollection coordCMDDropCollection ;

   /*
      _coordCMDRenameCollection define
   */
   class _coordCMDRenameCollection : public _coordDataCMD3Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDRenameCollection() ;
         virtual ~_coordCMDRenameCollection() ;
      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _doComplete ( MsgHeader *pMsg,
                                     pmdEDUCB * cb,
                                     coordCMDArguments *pArgs ) ;

         /*
            update catalog info before send command to Data Groups
         */
         virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

   } ;
   typedef _coordCMDRenameCollection coordCMDRenameCollection ;

   /*
      _coordCMDAlterCollection define
    */
   class _coordCMDAlterCollection : public _coordDataCMDAlter
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

      public:
         _coordCMDAlterCollection () ;
         virtual ~_coordCMDAlterCollection () ;

         virtual utilWriteResult*  getWriteResult() { return &_wrResult ; }

      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _doComplete ( MsgHeader *pMsg,
                                     pmdEDUCB * cb,
                                     coordCMDArguments *pArgs ) ;

         virtual INT32 _doRollback ( MsgHeader * pMsg,
                                     pmdEDUCB * cb,
                                     rtnContextCoord ** ppCoordCtxForCata,
                                     coordCMDArguments * pArguments,
                                     CoordGroupList & sucGroupLst,
                                     INT32 failedRC ) ;

      protected :
         /*
            update catalog info before send command to Data Groups
         */
         virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

         /*
            use group in Coord cache, since we only have short-term
            locks in Catalog
         */
         virtual BOOLEAN _flagUseGrpLstInCoord () { return TRUE ; }

         OSS_INLINE virtual RTN_ALTER_OBJECT_TYPE _getObjectType () const
         {
            return RTN_ALTER_COLLECTION ;
         }

         OSS_INLINE virtual MSG_TYPE _getCatalogMessageType () const
         {
            return MSG_CAT_ALTER_COLLECTION_REQ ;
         }

         virtual INT32 _executePostTasks ( const CHAR * name,
                                           const ossPoolList< UINT64 > & postTasks,
                                           pmdEDUCB * cb,
                                           CLS_TASK_TYPE *type ) ;

         virtual INT32 _cancelPostTasks ( const CHAR * name,
                                          const ossPoolList< UINT64 > & postTasks,
                                          pmdEDUCB * cb ) ;

         INT32 _waitPostTasks ( const bson::BSONObj & taskDesc,
                                pmdEDUCB * cb ) ;
         INT32 _cancelPostTask ( UINT64 taskID, pmdEDUCB * cb ) ;

         INT32 _invalidateSequences ( const CHAR * collection,
                                      const rtnAlterTask * task,
                                      pmdEDUCB * cb ) ;
         INT32 _invalidateSequences ( const CHAR * collection,
                                      const autoIncFieldsList & autoIncList,
                                      pmdEDUCB * cb ) ;

      protected:
         utilWriteResult            _wrResult ;
   } ;

   typedef _coordCMDAlterCollection coordCMDAlterCollection ;

   /*
      _coordCMDLinkCollection define
   */
   class _coordCMDLinkCollection : public _coordDataCMD2Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDLinkCollection() ;
         virtual ~_coordCMDLinkCollection() ;

      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  coordCMDArguments *pArgs,
                                                  CHAR **ppMsgBuf,
                                                  INT32 *pBufSize ) ;

         virtual void  _releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                INT32 bufSize,
                                                pmdEDUCB *cb ) ;

         virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              coordCMDArguments *pArgs,
                                              const CoordGroupList &groupLst ) ;

      protected :
         /*
            update catalog info before send command to Data Groups
         */
         virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

         /*
            Rollback on Catalog before rollback on Data groups
         */
         virtual BOOLEAN _flagRollbackCataBeforeData () { return TRUE ; }

      private:
         string            _subCLName ;
   } ;
   typedef _coordCMDLinkCollection coordCMDLinkCollection ;

   /*
      _coordCMDUnlinkCollection define
   */
   class _coordCMDUnlinkCollection : public _coordDataCMD2Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDUnlinkCollection() ;
         virtual ~_coordCMDUnlinkCollection() ;

      protected :

         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  coordCMDArguments *pArgs,
                                                  CHAR **ppMsgBuf,
                                                  INT32 *pBufSize ) ;

         virtual void  _releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                INT32 bufSize,
                                                pmdEDUCB *cb ) ;

         virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              coordCMDArguments *pArgs,
                                              const CoordGroupList &groupLst ) ;

      protected :
         /*
            update catalog info before send command to Data Groups
         */
         virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

         /*
            Rollback on Catalog before rollback on Data groups
         */
         virtual BOOLEAN _flagRollbackCataBeforeData () { return TRUE ; }

      protected:
         string            _subCLName ;

   } ;
   typedef _coordCMDUnlinkCollection coordCMDUnlinkCollection ;

   /*
      _coordCMDSplit define
   */
   class _coordCMDSplit : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDSplit() ;
         virtual ~_coordCMDSplit() ;

         virtual INT32 execute ( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf ) ;

      protected :
         INT32 _getCLCount ( const CHAR *clFullName,
                             const CoordGroupList &groupList,
                             pmdEDUCB *cb,
                             UINT64 &count,
                             rtnContextBuf *buf ) ;

         INT32 _splitPrepare( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              INT64 &contextID,
                              rtnContextBuf *buf ) ;

         INT32 _splitReady( pmdEDUCB *cb,
                            INT64 &contextID,
                            rtnContextBuf *buf,
                            UINT64 &taskID,
                            BSONObj &taskInfoObj ) ;

         INT32 _splitPost( UINT64 taskID,
                           pmdEDUCB *cb,
                           INT64 &contextID,
                           rtnContextBuf *buf ) ;

         INT32 _splitRollback( UINT64 taskID, pmdEDUCB *cb ) ;

         INT32 _splitParamCheck( const BSONObj &obj,
                                 pmdEDUCB *cb ) ;

         INT32 _makeSplitRange( pmdEDUCB *cb,
                                const CoordGroupList &srcGrpLst,
                                rtnContextBuf *buf ) ;

         INT32 _getBoundByPercent ( const CHAR *cl,
                                    FLOAT64 percent,
                                    const CoordCataInfoPtr &cataInfo,
                                    const CoordGroupList &groupList,
                                    pmdEDUCB *cb,
                                    BSONObj &lowBound,
                                    BSONObj &upBound,
                                    rtnContextBuf *buf ) ;

         INT32 _getBoundByCondition ( const CHAR *cl,
                                      const BSONObj &begin,
                                      const BSONObj &end,
                                      const CoordGroupList &groupList,
                                      pmdEDUCB *cb,
                                      BSONObj &lowBound,
                                      BSONObj &upBound,
                                      rtnContextBuf *buf ) ;

         INT32 _getBoundRecordOnData ( const CHAR *cl,
                                       const BSONObj &condition,
                                       const BSONObj &hint,
                                       const BSONObj &sort,
                                       INT32 flag,
                                       INT64 skip,
                                       const CoordGroupList &groupList,
                                       pmdEDUCB *cb,
                                       const BSONObj &shardingKey,
                                       BSONObj &record,
                                       rtnContextBuf *buf ) ;

      protected:
         string               _clName ;
         string               _srcGroup ;
         string               _dstGroup ;
         BOOLEAN              _async ;
         FLOAT64              _percent ;

         BSONElement          _eleSplitQuery ;
         BSONElement          _eleSplitEndQuery ;

         BSONObj              _lowBound ;
         BSONObj              _upBound ;

         coordCataSel         _cataSel ;
         BSONObj              _shardkingKey ;

   } ;
   typedef _coordCMDSplit coordCMDSplit ;

   /*
      _coordCMDCreateIndex define
   */
   class _coordCMDCreateIndex : public _coordDataCMD2Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDCreateIndex() ;
         virtual ~_coordCMDCreateIndex() ;

         virtual utilWriteResult*  getWriteResult() { return &_wrResult ; }

      protected :

         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

         virtual INT32 _generateRollbackDataMsg ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  coordCMDArguments *pArgs,
                                                  CHAR **ppMsgBuf,
                                                  INT32 *pBufSize ) ;

         virtual void  _releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                INT32 bufSize,
                                                pmdEDUCB *cb ) ;

         virtual INT32 _rollbackOnDataGroup ( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              coordCMDArguments *pArgs,
                                              const CoordGroupList &groupLst ) ;

      protected :
         /*
            update catalog info before send command to Data Groups
         */
         virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

         /*
            use group in Coord cache, since we only have short-term
            locks in Catalog
         */
         virtual BOOLEAN _flagUseGrpLstInCoord () { return TRUE ; }

      protected:
         string                  _indexName ;
         utilWriteResult         _wrResult ;
   } ;
   typedef _coordCMDCreateIndex coordCMDCreateIndex ;

   /*
      _coordCMDDropIndex define
   */
   class _coordCMDDropIndex : public _coordDataCMD2Phase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDDropIndex() ;
         virtual ~_coordCMDDropIndex() ;
      protected :
         virtual INT32 _parseMsg ( MsgHeader *pMsg,
                                   coordCMDArguments *pArgs ) ;

         virtual INT32 _generateCataMsg ( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          coordCMDArguments *pArgs,
                                          CHAR **ppMsgBuf,
                                          INT32 *pBufSize ) ;

         virtual void  _releaseCataMsg( CHAR *pMsgBuf,
                                        INT32 bufSize,
                                        pmdEDUCB *cb ) ;

      protected :
         /*
            update catalog info before send command to Data Groups
         */
         virtual BOOLEAN _flagUpdateBeforeData () { return TRUE ; }

         /*
            use group in Coord cache, since we only have short-term
            locks in Catalog
         */
         virtual BOOLEAN _flagUseGrpLstInCoord () { return TRUE ; }

   } ;
   typedef _coordCMDDropIndex coordCMDDropIndex ;

   /*
      _coordCMDPop define
   */
   class _coordCMDPop : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDPop() ;
         virtual ~_coordCMDPop() ;
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDPop coordCMDPop ;
}

#endif // COORD_COMMAND_DATA_HPP__
