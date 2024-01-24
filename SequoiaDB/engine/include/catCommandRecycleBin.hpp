/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = catCommandRecycleBin.hpp

   Descriptive Name = Catalogue commands for recycle bin

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for catalog
   commands.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_COMMAND_RECYCLEBIN_HPP__
#define CAT_COMMAND_RECYCLEBIN_HPP__

#include "catCMDBase.hpp"
#include "catRecycleBinManager.hpp"
#include "utilRecycleBinConf.hpp"

namespace engine
{

   /*
      _catCMDGetRecycleBinDetail define
    */
   class _catCMDGetRecycleBinDetail : public _catReadCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDGetRecycleBinDetail() ;
      virtual ~_catCMDGetRecycleBinDetail() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 )
      {
         return SDB_OK ;
      }

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *name() const
      {
         return CMD_NAME_GET_RECYCLEBIN_DETAIL ;
      }

   protected:
      catRecycleBinManager * _recycleBinMgr ;
   } ;

   typedef class _catCMDGetRecycleBinDetail catCMDGetRecycleBinDetail ;

   /*
      _catCMDAlterRecycleBin define
    */
   class _catCMDAlterRecycleBin : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDAlterRecycleBin() ;
      virtual ~_catCMDAlterRecycleBin() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *name() const
      {
         return CMD_NAME_ALTER_RECYCLEBIN ;
      }

   protected:
      catRecycleBinManager *  _recycleBinMgr ;
      // new recycle bin conf after alter
      utilRecycleBinConf      _newConf ;
   } ;

   typedef class _catCMDAlterRecycleBin catCMDAlterRecycleBin ;

   /*
      _catCMDGetRecycleBinCount define
    */
   class _catCMDGetRecycleBinCount : public _catReadCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDGetRecycleBinCount() ;
      virtual ~_catCMDGetRecycleBinCount() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *name() const
      {
         return CMD_NAME_GET_RECYCLEBIN_COUNT ;
      }

   protected:
      // cache of command options
      bson::BSONObj _queryObj ;
   } ;

   typedef class _catCMDGetRecycleBinCount catCMDGetRecycleBinCount ;

   /*
      _catCMDDropRecycleBinBase define
    */
   class _catCMDDropRecycleBinBase : public _catWriteCMDBase
   {
   public:
      _catCMDDropRecycleBinBase() ;
      virtual ~_catCMDDropRecycleBinBase() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *getProcessName() const
      {
         return _isDropAll() ? "" : _recycleItemName ;
      }

   protected:
      virtual BOOLEAN _isDropAll() const = 0 ;
      virtual INT32 _check( _pmdEDUCB *cb ) = 0 ;
      virtual INT32 _execute( _pmdEDUCB *cb,
                              INT16 w,
                              rtnContextBuf &ctxBuf ) = 0 ;

   protected:
      catRecycleBinManager *  _recycleBinMgr ;

      // cache of command options
      bson::BSONObj _queryObj ;
      bson::BSONObj _hintObj ;
      const CHAR *  _recycleItemName ;
      BOOLEAN       _isEnforced ;
      BOOLEAN       _isRecursive ;
      BOOLEAN       _isIgnoreLock ;
   } ;

   typedef class _catCMDDropRecycleBinBase catCMDDropRecycleBinBase ;

   /*
      _catCMDDropRecycleBinItem define
    */
   class _catCMDDropRecycleBinItem : public _catCMDDropRecycleBinBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDDropRecycleBinItem() {}
      virtual ~_catCMDDropRecycleBinItem() {}

      virtual const CHAR *name() const
      {
         return CMD_NAME_DROP_RECYCLEBIN_ITEM ;
      }

   protected:
      virtual BOOLEAN _isDropAll() const
      {
         return FALSE ;
      }

      virtual INT32 _check( _pmdEDUCB *cb ) ;
      virtual INT32 _execute( _pmdEDUCB *cb,
                              INT16 w,
                              rtnContextBuf &ctxBuf ) ;

      INT32 _checkRecycledCLInCS( utilRecycleItem &item,
                                  _pmdEDUCB *cb ) ;
      INT32 _checkCLInRecycledCS( utilRecycleItem &item,
                                  _pmdEDUCB *cb ) ;
      INT32 _checkSubCLInRecycledCS( utilRecycleItem &item,
                                     _pmdEDUCB *cb ) ;

   protected:
      utilRecycleItem _recycleItem ;
      CAT_GROUP_SET _groupIDSet ;
   } ;

   typedef class _catCMDDropRecycleBinItem catCMDDropRecycleBinItem ;

   /*
      _catCMDDropRecycleBinAll define
    */
   class _catCMDDropRecycleBinAll : public _catCMDDropRecycleBinBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDDropRecycleBinAll() {}
      virtual ~_catCMDDropRecycleBinAll() {}

      virtual const CHAR *name() const
      {
         return CMD_NAME_DROP_RECYCLEBIN_ALL ;
      }

   protected:
      virtual BOOLEAN _isDropAll() const
      {
         return TRUE ;
      }

      virtual INT32 _check( _pmdEDUCB *cb ) ;
      virtual INT32 _execute( _pmdEDUCB *cb,
                              INT16 w,
                              rtnContextBuf &ctxBuf ) ;
   } ;

   typedef class _catCMDDropRecycleBinAll catCMDDropRecycleBinAll ;

   /*
      _catCMDReturnRecycleBinBase define
    */
   class _catCMDReturnRecycleBinBase : public _catWriteCMDBase
   {
   public:
      _catCMDReturnRecycleBinBase() ;
      virtual ~_catCMDReturnRecycleBinBase() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      virtual const CHAR *getProcessName() const
      {
         return _recycleItemName ;
      }

   protected:
      virtual BOOLEAN _isReturnToName() const = 0 ;

   protected:
      // cache of command options
      bson::BSONObj _queryObj ;
      const CHAR * _recycleItemName ;
   } ;

   typedef class _catCMDReturnRecycleBinBase catCMDReturnRecycleBinBase ;

   /*
      _catCMDReturnRecycleBinItem define
    */
   class _catCMDReturnRecycleBinItem : public _catCMDReturnRecycleBinBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDReturnRecycleBinItem() {}
      virtual ~_catCMDReturnRecycleBinItem() {}

      virtual const CHAR *name() const
      {
         return CMD_NAME_RETURN_RECYCLEBIN_ITEM ;
      }

   protected:
      virtual BOOLEAN _isReturnToName() const
      {
         return FALSE ;
      }
   } ;

   typedef class _catCMDReturnRecycleBinItem catCMDReturnRecycleBinItem ;

   /*
      _catCMDReturnRecycleBinItemToName define
    */
   class _catCMDReturnRecycleBinItemToName : public _catCMDReturnRecycleBinBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _catCMDReturnRecycleBinItemToName() {}
      virtual ~_catCMDReturnRecycleBinItemToName() {}

      virtual const CHAR *name() const
      {
         return CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME ;
      }

   protected:
      virtual BOOLEAN _isReturnToName() const
      {
         return TRUE ;
      }
   } ;

   typedef class _catCMDReturnRecycleBinItem catCMDReturnRecycleBinItem ;

}

#endif // CAT_COMMAND_RECYCLEBIN_HPP__
