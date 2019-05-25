/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
   class _coordCMDSnapshotConfigIntr : public _coordCMDSnapshotIntrBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSnapshotConfigIntr() ;
         virtual ~_coordCMDSnapshotConfigIntr() ;
   } ;
   typedef _coordCMDSnapshotConfigIntr coordCMDSnapshotConfigIntr ;

}
#endif // COORD_COMMAND_SNAPSHOT_HPP__
