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

   Source File Name = catCMDBase.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains base class of catalog
   command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2019/10/02  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CATCMDBASE_HPP_
#define CATCMDBASE_HPP_

#include "rtnContextBuff.hpp"
#include "clsTask.hpp"
#include "pmdEDU.hpp"
#include "utilMap.hpp"

namespace engine
{
   #define CAT_DECLARE_CMD_AUTO_REGISTER() \
      public: \
         static _catCMDBase *newThis () ; \

   #define CAT_IMPLEMENT_CMD_AUTO_REGISTER(theClass) \
      _catCMDBase *theClass::newThis () \
      { \
         return SDB_OSS_NEW theClass() ;\
      } \
      _catCmdAssit theClass##Assit ( theClass::newThis ) ; \

   class _catCMDBase : public SDBObject
   {
   public:
      _catCMDBase() {} ;
      virtual ~_catCMDBase() {} ;

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 ) = 0 ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          rtnContextBuf &ctxBuf,
                          INT64 &contextID ) = 0 ;

      virtual const CHAR* name() const = 0 ;

      virtual const CHAR *getProcessName() const
      {
         return "" ;
      }

      // If this command can only be executed on primary node, we need check
      // primary
      virtual BOOLEAN needCheckPrimary() const = 0 ;

      // If this command can't be executed when DC is readonly, we need check
      // DC status
      virtual BOOLEAN needCheckDCStatus() const = 0 ;

      // doit() may generate clsTask to monitor progress. After the task is
      // finished, we may need to update metadata in postDoit().
      virtual INT32 postDoit( const clsTask *pTask, _pmdEDUCB *cb )
      {
         return SDB_OK ;
      }
   } ;
   typedef _catCMDBase catCMDBase ;

   typedef _catCMDBase* (*CAT_CMD_NEW_FUNC)() ;

   class _catCmdAssit : public SDBObject
   {
      public:
         _catCmdAssit ( CAT_CMD_NEW_FUNC pFunc ) ;
         ~_catCmdAssit () {} ;
   };

   class _catCmdBuilder : public SDBObject
   {
      typedef _utilStringMap< CAT_CMD_NEW_FUNC, 1 > MAP_COMMAND ;
      typedef MAP_COMMAND::iterator                 MAP_COMMAND_IT ;
      friend class _catCmdAssit ;

      public:
         _catCmdBuilder () ;
         ~_catCmdBuilder () ;
      public:
         INT32 create ( const CHAR *name, _catCMDBase *&pCommand ) ;
         void release ( _catCMDBase *&pCommand ) ;

      protected:
         INT32 _register ( const CHAR *name, CAT_CMD_NEW_FUNC pFunc ) ;

      private:
         MAP_COMMAND _mapCommand ;

   };

   class _catWriteCMDBase : public _catCMDBase
   {
   public:
      _catWriteCMDBase() {}
      virtual ~_catWriteCMDBase() {}

      // If this command can only be executed on primary node, we need check
      // primary
      virtual BOOLEAN needCheckPrimary() const
      {
         return TRUE ;
      }

      // If this command can't be executed when DC is readonly, we need check
      // DC status
      virtual BOOLEAN needCheckDCStatus() const
      {
         return TRUE ;
      }
   } ;
   typedef _catWriteCMDBase catWriteCMDBase ;

   class _catReadCMDBase : public _catCMDBase
   {
   public:
      _catReadCMDBase() {}
      virtual ~_catReadCMDBase() {}

      // If this command can only be executed on primary node, we need check
      // primary
      virtual BOOLEAN needCheckPrimary() const
      {
         return FALSE ;
      }

      // If this command can't be executed when DC is readonly, we need check
      // DC status
      virtual BOOLEAN needCheckDCStatus() const
      {
         return FALSE ;
      }
   } ;
   typedef _catReadCMDBase catReadCMDBase ;

   _catCmdBuilder * getCatCmdBuilder () ;

}

#endif // CATCMDBASE_HPP_

