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

   Source File Name = coordCommandSnapshot.hpp

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

#ifndef COORD_COMMAND_SNAPSHOT_HPP__
#define COORD_COMMAND_SNAPSHOT_HPP__

#include "coordCommandCommon.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDSnapshotIntrBase define
   */
   class _coordCMDSnapshotIntrBase : public _coordCMDMonIntrBase
   {
      public:
         _coordCMDSnapshotIntrBase() ;
         virtual ~_coordCMDSnapshotIntrBase() ;
      private:
         virtual BOOLEAN _useContext() { return TRUE ; }
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) {}
         virtual UINT32  _getControlMask() const { return COORD_CTRL_MASK_ALL ; }

   } ;
   typedef _coordCMDSnapshotIntrBase coordCMDSnapshotIntrBase ;

   /*
      _coordCMDSnapshotCurIntrBase define
   */
   class _coordCMDSnapshotCurIntrBase : public _coordCMDMonCurIntrBase
   {
      public:
         _coordCMDSnapshotCurIntrBase() ;
         virtual ~_coordCMDSnapshotCurIntrBase() ;
      private:
         virtual BOOLEAN _useContext() { return TRUE ; }
         virtual UINT32  _getControlMask() const { return COORD_CTRL_MASK_ALL ; }
   } ;
   typedef _coordCMDSnapshotCurIntrBase coordCMDSnapshotCurIntrBase ;

   /*
      _coordCmdSnapshotReset define
   */
   class _coordCmdSnapshotReset : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdSnapshotReset() ;
         virtual ~_coordCmdSnapshotReset() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         void _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;

         virtual INT32   _preExcute ( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      coordCtrlParam &ctrlParam,
                                      SET_RC &ignoreRCList ) ;
   } ;
   typedef _coordCmdSnapshotReset coordCmdSnapshotReset ;

   /*
      _coordSnapshotTransCurIntr define
   */
   class _coordSnapshotTransCurIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordSnapshotTransCurIntr() ;
         virtual ~_coordSnapshotTransCurIntr() ;
      private:
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordSnapshotTransCurIntr coordSnapshotTransCurIntr ;

   /*
      _coordSnapshotTransIntr define
   */
   class _coordSnapshotTransIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordSnapshotTransIntr() ;
         virtual ~_coordSnapshotTransIntr() ;
      private:
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordSnapshotTransIntr coordSnapshotTransIntr ;

   /*
      _coordSnapshotTransCur define
   */
   class _coordSnapshotTransCur : public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordSnapshotTransCur() ;
         virtual ~_coordSnapshotTransCur() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordSnapshotTransCur coordSnapshotTransCur ;

   /*
      _coordSnapshotTrans define
   */
   class _coordSnapshotTrans : public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordSnapshotTrans() ;
         virtual ~_coordSnapshotTrans() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordSnapshotTrans coordSnapshotTrans ;

   /*
      _coordCMDSnapshotDataBase define
   */
   class _coordCMDSnapshotDataBase: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotDataBase() ;
         virtual ~_coordCMDSnapshotDataBase() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;

         virtual UINT32 _getShowErrorMask () { return COORD_MASK_SHOWERROR ; }
   } ;
   typedef _coordCMDSnapshotDataBase coordCMDSnapshotDataBase ;

   /*
      _coordCMDSnapshotDataBaseIntr define
   */
   class _coordCMDSnapshotDataBaseIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotDataBaseIntr() ;
         virtual ~_coordCMDSnapshotDataBaseIntr() ;
         virtual void _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordCMDSnapshotDataBaseIntr coordCMDSnapshotDataBaseIntr ;

   /*
      _coordCMDSnapshotSystem define
   */
   class _coordCMDSnapshotSystem: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotSystem() ;
         virtual ~_coordCMDSnapshotSystem() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;

         virtual UINT32 _getShowErrorMask () { return COORD_MASK_SHOWERROR ; }
   } ;
   typedef _coordCMDSnapshotSystem coordCMDSnapshotSystem ;

   /*
      _coordCMDSnapshotSystemIntr define
   */
   class _coordCMDSnapshotSystemIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotSystemIntr() ;
         virtual ~_coordCMDSnapshotSystemIntr() ;
   } ;
   typedef _coordCMDSnapshotSystemIntr coordCMDSnapshotSystemIntr ;

   /*
      _coordCMDSnapshotHealth define
   */
   class _coordCMDSnapshotHealth: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotHealth() ;
         virtual ~_coordCMDSnapshotHealth() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotHealth coordCMDSnapshotHealth ;

   /*
      _coordCMDSnapshotHealthIntr define
   */
   class _coordCMDSnapshotHealthIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotHealthIntr() ;
         virtual ~_coordCMDSnapshotHealthIntr() ;
         virtual void _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordCMDSnapshotHealthIntr coordCMDSnapshotHealthIntr ;

   /*
      _coordCMDSnapshotCollections define
   */
   class _coordCMDSnapshotCollections: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotCollections() ;
         virtual ~_coordCMDSnapshotCollections() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotCollections coordCMDSnapshotCollections ;

   /*
      _coordCMDSnapshotCLIntr define
   */
   class _coordCMDSnapshotCLIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotCLIntr() ;
         virtual ~_coordCMDSnapshotCLIntr() ;
   } ;
   typedef _coordCMDSnapshotCLIntr coordCMDSnapshotCLIntr ;

   /*
      _coordCMDSnapshotSpaces define
   */
   class _coordCMDSnapshotSpaces: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotSpaces() ;
         virtual ~_coordCMDSnapshotSpaces() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotSpaces coordCMDSnapshotSpaces ;

   /*
      _coordCMDSnapshotCSIntr define
   */
   class _coordCMDSnapshotCSIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotCSIntr() ;
         virtual ~_coordCMDSnapshotCSIntr() ;
   } ;
   typedef _coordCMDSnapshotCSIntr coordCMDSnapshotCSIntr ;

   /*
      _coordCMDSnapshotContexts define
   */
   class _coordCMDSnapshotContexts: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotContexts() ;
         virtual ~_coordCMDSnapshotContexts() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotContexts coordCMDSnapshotContexts ;

   /*
      _coordCMDSnapshotContextIntr define
   */
   class _coordCMDSnapshotContextIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotContextIntr() ;
         virtual ~_coordCMDSnapshotContextIntr() ;
   } ;
   typedef _coordCMDSnapshotContextIntr coordCMDSnapshotContextIntr ;

   /*
      _coordCMDSnapshotContextsCur define
   */
   class _coordCMDSnapshotContextsCur: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotContextsCur() ;
         virtual ~_coordCMDSnapshotContextsCur() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotContextsCur coordCMDSnapshotContextsCur ;

   /*
      _coordCMDSnapshotContextCurIntr define
   */
   class _coordCMDSnapshotContextCurIntr : public coordCMDSnapshotCurIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotContextCurIntr() ;
         virtual ~_coordCMDSnapshotContextCurIntr() ;
   } ;
   typedef _coordCMDSnapshotContextCurIntr coordCMDSnapshotContextCurIntr ;

   /*
      _coordCMDSnapshotSessions define
   */
   class _coordCMDSnapshotSessions: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotSessions() ;
         virtual ~_coordCMDSnapshotSessions() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotSessions coordCMDSnapshotSessions ;

   /*
      _coordCMDSnapshotSessionIntr define
   */
   class _coordCMDSnapshotSessionIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotSessionIntr() ;
         virtual ~_coordCMDSnapshotSessionIntr() ;
   } ;
   typedef _coordCMDSnapshotSessionIntr coordCMDSnapshotSessionIntr ;

   /*
      _coordCMDSnapshotSessionsCur define
   */
   class _coordCMDSnapshotSessionsCur: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotSessionsCur() ;
         virtual ~_coordCMDSnapshotSessionsCur() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotSessionsCur coordCMDSnapshotSessionsCur ;

   /*
      _coordCMDSnapshotSessionCurIntr define
   */
   class _coordCMDSnapshotSessionCurIntr : public _coordCMDSnapshotCurIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotSessionCurIntr() ;
         virtual ~_coordCMDSnapshotSessionCurIntr() ;
   } ;
   typedef _coordCMDSnapshotSessionCurIntr coordCMDSnapshotSessionCurIntr ;

   /*
      _coordCMDSnapshotCata define
   */
   class _coordCMDSnapshotCata : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotCata() ;
         virtual ~_coordCMDSnapshotCata() ;
      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;

         virtual INT32 _processVCS( rtnQueryOptions &queryOpt,
                                    const CHAR *pName,
                                    rtnContext *pContext ) ;
   } ;
   typedef _coordCMDSnapshotCata coordCMDSnapshotCata ;

   /*
      _coordCMDSnapshotCataIntr define
   */
   class _coordCMDSnapshotCataIntr : public _coordCMDSnapshotCata
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotCataIntr() ;
         virtual ~_coordCMDSnapshotCataIntr() ;
   } ;
   typedef _coordCMDSnapshotCataIntr coordCMDSnapshotCataIntr ;

   /*
      _coordCMDSnapshotAccessPlans define
    */
   class _coordCMDSnapshotAccessPlans : public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

      public :
         _coordCMDSnapshotAccessPlans () ;
         virtual ~_coordCMDSnapshotAccessPlans () ;

      private:
         virtual const CHAR *getIntrCMDName () ;
         virtual const CHAR *getInnerAggrContent () ;
   } ;

   typedef _coordCMDSnapshotAccessPlans coordCMDSnapshotAccessPlans ;

   /*
      _coordCMDSnapshotAccessPlansIntr define
    */
   class _coordCMDSnapshotAccessPlansIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotAccessPlansIntr () ;
         virtual ~_coordCMDSnapshotAccessPlansIntr () ;
   } ;

   typedef _coordCMDSnapshotAccessPlansIntr coordCMDSnapshotAccessPlansIntr ;

   /*
      _coordCMDSnapshotConfigs define
   */
   class _coordCMDSnapshotConfigs: public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotConfigs() ;
         virtual ~_coordCMDSnapshotConfigs() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotConfigs coordCMDSnapshotConfigs ;

   /*
      _coordCMDSnapshotConfigIntr define
   */
   class _coordCMDSnapshotConfigsIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotConfigsIntr() ;
         virtual ~_coordCMDSnapshotConfigsIntr() ;
         virtual void _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordCMDSnapshotConfigsIntr coordCMDSnapshotConfigsIntr ;

   /*
      _coordCMDSnapshotSvcTasks define
   */
   class _coordCMDSnapshotSvcTasks : public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotSvcTasks() ;
         virtual ~_coordCMDSnapshotSvcTasks() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotSvcTasks coordCMDSnapshotSvcTasks ;

   /*
      _coordCMDSnapshotSvcTasksIntr define
   */
   class _coordCMDSnapshotSvcTasksIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotSvcTasksIntr() ;
         virtual ~_coordCMDSnapshotSvcTasksIntr() ;
   } ;
   typedef _coordCMDSnapshotSvcTasksIntr coordCMDSnapshotSvcTasksIntr ;

    /*
       _coordCMDSnapshotSequences define
    */
    class _coordCMDSnapshotSequences : public _coordCMDQueryBase
    {
       COORD_DECLARE_CMD_AUTO_REGISTER() ;
       public:
          _coordCMDSnapshotSequences() ;
          virtual ~_coordCMDSnapshotSequences() ;
       protected:
          virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                     string &clName,
                                     BSONObj &outSelector ) ;
    } ;
    typedef _coordCMDSnapshotSequences coordCMDSnapshotSequences ;

    /*
       _coordCMDSnapshotSequencesIntr define
     */
    class _coordCMDSnapshotSequencesIntr : public _coordCMDSnapshotSequences
    {
       COORD_DECLARE_CMD_AUTO_REGISTER() ;
       public:
          _coordCMDSnapshotSequencesIntr () ;
          virtual ~_coordCMDSnapshotSequencesIntr () ;
    } ;
    typedef _coordCMDSnapshotSequencesIntr coordCMDSnapshotSequencesIntr ;

    /*
       _coordCMDSnapshotQueries define
    */
    class _coordCMDSnapshotQueries : public _coordCMDMonBase
    {
       COORD_DECLARE_CMD_AUTO_REGISTER() ;
       public:
          _coordCMDSnapshotQueries() ;
          virtual ~_coordCMDSnapshotQueries() ;
       private:
          virtual const CHAR *getIntrCMDName() ;
          virtual const CHAR *getInnerAggrContent() ;
    } ;
    typedef _coordCMDSnapshotQueries coordCMDSnapshotQueries ;

   /*
      _coordSnapshotQueriesIntr define
   */
   class _coordSnapshotQueriesIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordSnapshotQueriesIntr() ;
         virtual ~_coordSnapshotQueriesIntr() ;
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordSnapshotQueriesIntr coordSnapshotQueriesIntr ;

   /*
      _coordCMDSnapshotLatchWaits define
   */
   class _coordCMDSnapshotLatchWaits : public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotLatchWaits() ;
         virtual ~_coordCMDSnapshotLatchWaits() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotLatchWaits coordCMDSnapshotLatchWaits ;

   /*
      _coordCMDSnapshotLockWaitsIntr define
   */
   class _coordCMDSnapshotLatchWaitsIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotLatchWaitsIntr() {}
         virtual ~_coordCMDSnapshotLatchWaitsIntr() {}
   } ;
   typedef _coordCMDSnapshotLatchWaitsIntr coordCMDSnapshotLatchWaitsIntr ;

   /*
      _coordCMDSnapshotLockWaits define
   */
   class _coordCMDSnapshotLockWaits : public _coordCMDMonBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotLockWaits() ;
         virtual ~_coordCMDSnapshotLockWaits() ;
      private:
         virtual const CHAR *getIntrCMDName() ;
         virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotLockWaits coordCMDSnapshotLockWaits ;

   /*
      _coordCMDSnapshotLockWaitsIntr define
   */
   class _coordCMDSnapshotLockWaitsIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotLockWaitsIntr() {}
         virtual ~_coordCMDSnapshotLockWaitsIntr() {}
   } ;
   typedef _coordCMDSnapshotLockWaitsIntr coordCMDSnapshotLockWaitsIntr ;

   /*
      _coordCMDSnapshotIndexStats define
   */
   class _coordCMDSnapshotIndexStats : public _coordCMDMonBase
   {
     COORD_DECLARE_CMD_AUTO_REGISTER() ;
     public:
        _coordCMDSnapshotIndexStats() ;
        virtual ~_coordCMDSnapshotIndexStats() ;
     private:
        virtual const CHAR *getIntrCMDName() ;
        virtual const CHAR *getInnerAggrContent() ;
   } ;
   typedef _coordCMDSnapshotIndexStats coordCMDSnapshotIndexStats ;

   /*
      _coordCMDSnapshotIndexStatsIntr define
   */
   class _coordCMDSnapshotIndexStatsIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotIndexStatsIntr() {}
         virtual ~_coordCMDSnapshotIndexStatsIntr() {}
   } ;
   typedef _coordCMDSnapshotIndexStatsIntr coordCMDSnapshotIndexStatsIntr ;

}
#endif // COORD_COMMAND_SNAPSHOT_HPP__
