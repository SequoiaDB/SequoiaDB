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

   Source File Name = coordCommandList.hpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05-02-2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMAND_LIST_HPP__
#define COORD_COMMAND_LIST_HPP__

#include "coordCommandCommon.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDListIntrBase define
   */
   class _coordCMDListIntrBase : public _coordCMDMonIntrBase
   {
      public:
         _coordCMDListIntrBase() ;
         virtual ~_coordCMDListIntrBase() ;

      private:
         virtual BOOLEAN _useContext() { return TRUE ; }
         virtual UINT32  _getControlMask() const { return COORD_CTRL_MASK_ALL ; }

   } ;
   typedef _coordCMDListIntrBase coordCMDListIntrBase ;

   /*
      _coordCMDListCurIntrBase define
   */
   class _coordCMDListCurIntrBase : public _coordCMDMonCurIntrBase
   {
      public:
         _coordCMDListCurIntrBase() ;
         virtual ~_coordCMDListCurIntrBase() ;
      private:
         virtual BOOLEAN _useContext() { return TRUE ; }
         virtual UINT32  _getControlMask() const { return COORD_CTRL_MASK_ALL ; }
   } ;
   typedef _coordCMDListCurIntrBase coordCMDListCurIntrBase ;

   /*
      _coordListTransCurIntr define
   */
   class _coordListTransCurIntr : public _coordCMDListIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordListTransCurIntr() ;
         virtual ~_coordListTransCurIntr() ;
      private:
         virtual void _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordListTransCurIntr coordListTransCurIntr ;

   /*
      _coordListTransIntr define
   */
   class _coordListTransIntr : public _coordCMDListIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordListTransIntr() ;
         virtual ~_coordListTransIntr() ;
      private:
         virtual void _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordListTransIntr coordListTransIntr ;

   /*
      _coordListTransCur define
   */
   class _coordListTransCur : public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordListTransCur() ;
         virtual ~_coordListTransCur() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordListTransCur coordListTransCur ;

   /*
      _coordListTrans define
   */
   class _coordListTrans : public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordListTrans() ;
         virtual ~_coordListTrans() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordListTrans coordListTrans ;

   /*
      _coordListBackupIntr define
   */
   class _coordListBackupIntr : public _coordCMDListIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordListBackupIntr() ;
         virtual ~_coordListBackupIntr() ;
      private:
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordListBackupIntr coordListBackupIntr ;

   /*
      _coordListBackup define
   */
   class _coordListBackup : public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordListBackup() ;
         virtual ~_coordListBackup() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordListBackup coordListBackup ;

   /*
      _coordCMDListGroups define
   */
   class _coordCMDListGroups : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListGroups() ;
         virtual ~_coordCMDListGroups() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListGroups coordCMDListGroups ;

   /*
      _coordCmdListGroupIntr define
   */
   class _coordCmdListGroupIntr : public _coordCMDListGroups
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListGroupIntr() ;
         virtual ~_coordCmdListGroupIntr() ;
   } ;
   typedef _coordCmdListGroupIntr coordCmdListGroupIntr ;

   /*
      _coordCMDListCollectionSpace define
   */
   class _coordCMDListCollectionSpace : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListCollectionSpace() ;
         virtual ~_coordCMDListCollectionSpace() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListCollectionSpace coordCMDListCollectionSpace ;

   /*
      _coordCMDListCollectionSpaceIntr define
   */
   class _coordCMDListCollectionSpaceIntr : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListCollectionSpaceIntr() ;
         virtual ~_coordCMDListCollectionSpaceIntr() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListCollectionSpaceIntr coordCMDListCollectionSpaceIntr ;

   /*
      _coordCMDListCollection define
   */
   class _coordCMDListCollection : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListCollection() ;
         virtual ~_coordCMDListCollection() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListCollection coordCMDListCollection ;

   /*
      _coordCMDListCollectionIntr define
   */
   class _coordCMDListCollectionIntr : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListCollectionIntr() ;
         virtual ~_coordCMDListCollectionIntr() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListCollectionIntr coordCMDListCollectionIntr ;

   /*
      _coordCMDListContexts define
   */
   class _coordCMDListContexts: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListContexts() ;
         virtual ~_coordCMDListContexts() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDListContexts coordCMDListContexts ;

   /*
      _coordCmdListContextIntr define
   */
   class _coordCmdListContextIntr : public _coordCMDListIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListContextIntr() ;
         virtual ~_coordCmdListContextIntr() ;
      private:
         virtual void _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) {}
   } ;
   typedef _coordCmdListContextIntr coordCmdListContextIntr ;

   /*
      _coordCMDListContextsCur define
   */
   class _coordCMDListContextsCur: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListContextsCur() ;
         virtual ~_coordCMDListContextsCur() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDListContextsCur coordCMDListContextsCur ;

   /*
      _coordCmdListContextCurIntr define
   */
   class _coordCmdListContextCurIntr : public _coordCMDListCurIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListContextCurIntr() ;
         virtual ~_coordCmdListContextCurIntr() ;
   } ;
   typedef _coordCmdListContextCurIntr coordCmdListContextCurIntr ;

   /*
      _coordCMDListSessions define
   */
   class _coordCMDListSessions: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListSessions() ;
         virtual ~_coordCMDListSessions() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDListSessions coordCMDListSessions ;

   /*
      _coordCmdListSessionIntr define
   */
   class _coordCmdListSessionIntr : public _coordCMDListIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListSessionIntr() ;
         virtual ~_coordCmdListSessionIntr() ;
      private:
         virtual void _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) {}
   } ;
   typedef _coordCmdListSessionIntr coordCmdListSessionIntr ;

   /*
      _coordCMDListSessionsCur define
   */
   class _coordCMDListSessionsCur: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListSessionsCur() ;
         virtual ~_coordCMDListSessionsCur() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDListSessionsCur coordCMDListSessionsCur ;

   /*
      _coordCmdListSessionCurIntr define
   */
   class _coordCmdListSessionCurIntr : public _coordCMDListCurIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListSessionCurIntr() ;
         virtual ~_coordCmdListSessionCurIntr() ;
   } ;
   typedef _coordCmdListSessionCurIntr coordCmdListSessionCurIntr ;

   /*
      _coordCMDListSequences define
   */
   class _coordCMDListSequences : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListSequences() ;
         virtual ~_coordCMDListSequences() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListSequences coordCMDListSequences ;

   /*
      _coordCMDListSequencesIntr define
   */
   class _coordCMDListSequencesIntr : public _coordCMDListSequences
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListSequencesIntr() ;
         virtual ~_coordCMDListSequencesIntr() ;
   } ;
   typedef _coordCMDListSequencesIntr coordCMDListSequencesIntr ;

   /*
      _coordCMDListUser define
   */
   class _coordCMDListUser : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListUser() ;
         virtual ~_coordCMDListUser() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListUser coordCMDListUser ;

   /*
      _coordCmdListUserIntr define
   */
   class _coordCmdListUserIntr : public _coordCMDListUser
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListUserIntr() ;
         virtual ~_coordCmdListUserIntr() ;
   } ;
   typedef _coordCmdListUserIntr coordCmdListUserIntr ;

   /*
      _coordCmdListTask define
   */
   class _coordCmdListTask : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListTask() ;
         virtual ~_coordCmdListTask() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCmdListTask coordCmdListTask ;

   /*
      _coordCmdListTaskIntr define
   */
   class _coordCmdListTaskIntr : public _coordCmdListTask
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListTaskIntr() ;
         virtual ~_coordCmdListTaskIntr() ;
   } ;
   typedef _coordCmdListTaskIntr coordCmdListTaskIntr ;

   /*
      _coordCmdListIndexes define
   */
   class _coordCmdListIndexes : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListIndexes() ;
         virtual ~_coordCmdListIndexes() ;
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCmdListIndexes coordCmdListIndexes ;

   /*
      _coordCmdListIndexesIntr define
   */
   class _coordCmdListIndexesIntr : public _coordCmdListIndexes
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListIndexesIntr() ;
         virtual ~_coordCmdListIndexesIntr() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCmdListIndexesIntr coordCmdListIndexesIntr ;

   /*
      _coordCMDListProcedures define
   */
   class _coordCMDListProcedures : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListProcedures() ;
         virtual ~_coordCMDListProcedures() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListProcedures coordCMDListProcedures ;

   /*
      _coordCMDListDomains define
   */
   class _coordCMDListDomains : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListDomains() ;
         virtual ~_coordCMDListDomains() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListDomains coordCMDListDomains ;

   /*
      _coordCmdListDomainIntr define
   */
   class _coordCmdListDomainIntr : public _coordCMDListDomains
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdListDomainIntr() ;
         virtual ~_coordCmdListDomainIntr() ;
   } ;
   typedef _coordCmdListDomainIntr coordCmdListDomainIntr ;

   /*
      _coordCMDListCSInDomain define
   */
   class _coordCMDListCSInDomain : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListCSInDomain() ;
         virtual ~_coordCMDListCSInDomain() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListCSInDomain coordCMDListCSInDomain ;

   /*
      _coordCMDListCLInDomain define
   */
   class _coordCMDListCLInDomain : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListCLInDomain() ;
         virtual ~_coordCMDListCLInDomain() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      private:
         INT32 _rebuildListResult( const std::vector<BSONObj> &infoFromCata,
                                   pmdEDUCB *cb,
                                   SINT64 &contextID ) ;
   } ;
   typedef _coordCMDListCLInDomain coordCMDListCLInDomain ;

   /*
      _coordCMDListLobs define
   */
   class _coordCMDListLobs : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListLobs() ;
         virtual ~_coordCMDListLobs() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDListLobs coordCMDListLobs ;

   /*
      _coordCMDListSvcTasks define
   */
   class _coordCMDListSvcTasks: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListSvcTasks() ;
         virtual ~_coordCMDListSvcTasks() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDListSvcTasks coordCMDListSvcTasks ;

   /*
      _coordCMDListSvcTasksIntr define
   */
   class _coordCMDListSvcTasksIntr : public _coordCMDListIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListSvcTasksIntr() ;
         virtual ~_coordCMDListSvcTasksIntr() ;
         void _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordCMDListSvcTasksIntr coordCMDListSvcTasksIntr ;

   class _coordCMDListDataSources : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListDataSources() ;
         virtual ~_coordCMDListDataSources() ;

      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordCMDListDataSources coordCMDListDataSources ;

   class _coordCMDListDataSourceIntr : public _coordCMDListDataSources
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListDataSourceIntr() ;
         virtual ~_coordCMDListDataSourceIntr() ;
   } ;
   typedef _coordCMDListDataSourceIntr coordCMDListDataSourceIntr ;

   /*
      _coordCMDListRecycleBin define
    */
   class _coordCMDListRecycleBin : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDListRecycleBin() ;
      ~_coordCMDListRecycleBin() ;

   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;
   typedef class _coordCMDListRecycleBin coordCMDListRecycleBin ;

   /*
      _coordCMDListRecycleBinIntr define
    */
   class _coordCMDListRecycleBinIntr : public _coordCMDListRecycleBin
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDListRecycleBinIntr() ;
      virtual ~_coordCMDListRecycleBinIntr() ;
   } ;
   typedef class _coordCMDListRecycleBinIntr coordCMDListRecycleBinIntr ;

   /* 
      _coordCMDListGrpModes define
    */
   class _coordCMDListGrpModes : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
   public:
      _coordCMDListGrpModes() ;
      virtual ~_coordCMDListGrpModes() ;

   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) ;
   } ;
   typedef class _coordCMDListGrpModes coordCMDListGrpModes ;

   /*
      _coordCMDListGroupModeIntr define
   */
   class _coordCMDListGroupModeIntr : public _coordCMDListGrpModes
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDListGroupModeIntr() ;
         virtual ~_coordCMDListGroupModeIntr() ;
   } ;
   typedef _coordCMDListGroupModeIntr coordCMDListGroupModeIntr ;

}

#endif // COORD_COMMAND_LIST_HPP__
