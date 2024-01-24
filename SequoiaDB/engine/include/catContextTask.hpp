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

   Source File Name = catContextTask.hpp

   Descriptive Name = Sub-tasks for Catalog RunTime Context of Header

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

#ifndef CATCONTEXTTASK_HPP_
#define CATCONTEXTTASK_HPP_

#include "rtnContext.hpp"
#include "catLevelLock.hpp"
#include "catalogueCB.hpp"
#include "IDataSource.hpp"

using namespace bson ;

namespace engine
{
   class _SDB_DMSCB ;

   /*
    * _catCtxTaskBase define
    */
   class _catCtxTaskBase : public SDBObject
   {
   public :
      _catCtxTaskBase () ;

      virtual ~_catCtxTaskBase () {} ;

      void enableLocks () { _needLocks = TRUE ; }

      void disableLocks () { _needLocks = FALSE ; }

      void disableUpdate () { _needUpdate = FALSE ; }

      virtual INT32 checkTask ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;

      virtual INT32 recheckTask( _pmdEDUCB *cb ) ;

      virtual INT32 preExecute ( _pmdEDUCB *cb,
                                 SDB_DMSCB *pDmsCB,
                                 SDB_DPSCB *pDpsCB,
                                 INT16 w ) ;

      virtual INT32 execute ( _pmdEDUCB *cb,
                              SDB_DMSCB *pDmsCB,
                              SDB_DPSCB *pDpsCB,
                              INT16 w ) ;

      virtual INT32 rollback ( _pmdEDUCB *cb,
                               SDB_DMSCB *pDmsCB,
                               SDB_DPSCB *pDpsCB,
                               INT16 w ) ;

      void addIgnoreRC ( INT32 rc ) ;

      BOOLEAN isIgnoredRC ( INT32 rc ) ;

   protected :
      virtual INT32 _checkInternal ( _pmdEDUCB *cb,
                                     catCtxLockMgr &lockMgr ) = 0 ;

      virtual INT32 _recheckInternal( _pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32 _preExecuteInternal ( _pmdEDUCB *cb,
                                          SDB_DMSCB *pDmsCB,
                                          SDB_DPSCB *pDpsCB,
                                          INT16 w ) = 0 ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w ) = 0 ;

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb,
                                        SDB_DMSCB *pDmsCB,
                                        SDB_DPSCB *pDpsCB,
                                        INT16 w ) = 0 ;

   protected :
      BOOLEAN _needLocks ;
      BOOLEAN _needUpdate ;
      BOOLEAN _hasUpdated ;
      std::set<INT32> _ignoreRC ;
   } ;
   typedef class _catCtxTaskBase catCtxTaskBase ;
   /*
    * _catCtxDataTask define
    */
   class _catCtxDataTask : public _catCtxTaskBase
   {
   public :
      _catCtxDataTask ( const std::string &dataName ) ;

      virtual ~_catCtxDataTask () {} ;

      const BSONObj &getDataObj () const { return _boData ; }

      const std::string &getDataName () const { return _dataName ; }

      BOOLEAN isDataSource() const
      {
         return UTIL_INVALID_DS_UID != _dsUID ;
      }

      UTIL_DS_UID getDataSourceUID() const
      {
         return _dsUID ;
      }

   protected :
      virtual INT32 _preExecuteInternal ( _pmdEDUCB *cb,
                                          SDB_DMSCB *pDmsCB,
                                          SDB_DPSCB *pDpsCB,
                                          INT16 w )
      { return SDB_OK ; }

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb,
                                        SDB_DMSCB *pDmsCB,
                                        SDB_DPSCB *pDpsCB,
                                        INT16 w )
      { return SDB_OK ; }

   protected :
      std::string _dataName ;
      BSONObj _boData ;
      UTIL_DS_UID _dsUID ;
   } ;

   typedef _catCtxDataTask catCtxDataTask ;
   /*
    * _catCtxDropCSTask define
    */
   class _catCtxDropCSTask : public _catCtxDataTask
   {
   public :
      _catCtxDropCSTask ( const std::string &csName ) ;
      virtual ~_catCtxDropCSTask () {}

   protected :
      virtual INT32 _checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w ) ;
   } ;

   /*
    * _catCtxDropCLTask define
    */
   class _catCtxDropCLTask : public _catCtxDataTask
   {
   public :
      _catCtxDropCLTask ( const std::string &clName, INT32 version,
                          BOOLEAN rmTaskAndIdx ) ;

      virtual ~_catCtxDropCLTask () {}

      INT32 getVersion () const { return _version ; }

      const CAT_PAIR_CLNAME_ID_LIST& globalIndexCLList() const
      {
         return _globalIdxCLList ;
      }

   protected :
      virtual INT32 _checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;

      virtual INT32 _recheckInternal( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w ) ;

   protected :
      INT32 _version ;
      BOOLEAN _rmTaskAndIdx ;
      CAT_PAIR_CLNAME_ID_LIST _globalIdxCLList ;
   } ;

   /*
    * _catCtxUnlinkMainCLTask define
    */
   class _catCtxUnlinkMainCLTask : public _catCtxDataTask
   {
   public :
      _catCtxUnlinkMainCLTask ( const std::string &mainCLName,
                                const std::string &subCLName ) ;

      virtual ~_catCtxUnlinkMainCLTask () {}

   protected :
      virtual INT32 _checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w ) ;
   protected :
      std::string _subCLName ;
   } ;

   /*
    * _catCtxUnlinkSubCLTask define
    */
   class _catCtxUnlinkSubCLTask : public _catCtxDataTask
   {
   public :
      _catCtxUnlinkSubCLTask ( const std::string &mainCLName,
                               const std::string &subCLName ) ;

      virtual ~_catCtxUnlinkSubCLTask () {}

   protected :
      virtual INT32 _checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w ) ;

   protected :
      BOOLEAN _needUnlink ;
      std::string _mainCLName ;
   } ;

   /*
    * _catCtxDelCLsFromCSTask define
    */
   class _catCtxDelCLsFromCSTask : public _catCtxDataTask
   {
   public :
      _catCtxDelCLsFromCSTask () ;

      virtual ~_catCtxDelCLsFromCSTask () {}

      INT32 deleteCL ( const std::string &clFullName ) ;

   protected :
      virtual INT32 _checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w ) ;

   protected :
      typedef std::map< std::string, std::vector<std::string> > COLLECTION_MAP ;
      COLLECTION_MAP _deleteCLMap ;
   } ;

   /*
    * _catCtxUnlinkCSTask define
    */
   class _catCtxUnlinkCSTask : public _catCtxDataTask
   {
   public :
      _catCtxUnlinkCSTask ( const std::string &csName ) ;

      virtual ~_catCtxUnlinkCSTask () {}

      INT32 unlinkCS ( const std::string &mainCLName ) ;

      INT32 unlinkCS ( const std::set<std::string> &mainCLLst ) ;

   protected :
      virtual INT32 _checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w ) ;

   protected :
      std::set<std::string> _mainCLLst ;
   } ;

}

#endif //CATCONTEXTBASETASK_HPP_
