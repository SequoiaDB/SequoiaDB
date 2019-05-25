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

   Source File Name = rtnCommandList.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_COMMAND_LIST_HPP_
#define RTN_COMMAND_LIST_HPP_

#include "rtnCommandMon.hpp"

using namespace bson ;

namespace engine
{

   class _rtnListInner : public _rtnMonInnerBase
   {
      protected:
         _rtnListInner () {}
         virtual ~_rtnListInner () {}

      protected:
         virtual BOOLEAN _isDetail() const { return FALSE ; }
   } ;

   class _rtnList : public _rtnMonBase
   {
      protected:
         _rtnList () {}
         virtual ~_rtnList () {}

      protected:
         virtual BOOLEAN _isDetail() const { return FALSE ; }

   } ;

   /*
      _rtnListCollections define
   */
   class _rtnListCollections : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollections () ;
         virtual ~_rtnListCollections () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   /*
      _rtnListCollectionsInner define
   */
   class _rtnListCollectionsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollectionsInner () ;
         virtual ~_rtnListCollectionsInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   };

   /*
      _rtnListCollectionspaces define
   */
   class _rtnListCollectionspaces : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollectionspaces () ;
         virtual ~_rtnListCollectionspaces () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   /*
      _rtnListCollectionspacesInner define
   */
   class _rtnListCollectionspacesInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollectionspacesInner () ;
         virtual ~_rtnListCollectionspacesInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   /*
      _rtnListContexts define
   */
   class _rtnListContexts : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContexts () ;
         virtual ~_rtnListContexts () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   } ;

   /*
      _rtnListContextsInner define
   */
   class _rtnListContextsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContextsInner () ;
         virtual ~_rtnListContextsInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   /*
      _rtnListContextsCurrent define
   */
   class _rtnListContextsCurrent : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContextsCurrent () ;
         virtual ~_rtnListContextsCurrent () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   /*
      _rtnListContextsCurrentInner define
   */
   class _rtnListContextsCurrentInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContextsCurrentInner () ;
         virtual ~_rtnListContextsCurrentInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   /*
      _rtnListSessions define
   */
   class _rtnListSessions : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessions () ;
         virtual ~_rtnListSessions () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   /*
      _rtnListSessionsInner define
   */
   class _rtnListSessionsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessionsInner () ;
         virtual ~_rtnListSessionsInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   /*
      _rtnListSessionsCurrent implement
   */
   class _rtnListSessionsCurrent : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessionsCurrent () ;
         virtual ~_rtnListSessionsCurrent () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   /*
      _rtnListSessionsCurrentInner define
   */
   class _rtnListSessionsCurrentInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessionsCurrentInner () ;
         virtual ~_rtnListSessionsCurrentInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   /*
      _rtnListStorageUnits define
   */
   class _rtnListStorageUnits : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListStorageUnits () ;
         virtual ~_rtnListStorageUnits () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   } ;

   /*
      _rtnListStorageUnitsInner define
   */
   class _rtnListStorageUnitsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListStorageUnitsInner () ;
         virtual ~_rtnListStorageUnitsInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   /*
      _rtnListBackups define
   */
   class _rtnListBackups : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER () ;

      public:
         _rtnListBackups () ;
         virtual ~_rtnListBackups () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
         virtual BSONObj _getOptObj() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   } ;

   /*
      _rtnListBackupsInner define
   */
   class _rtnListBackupsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListBackupsInner () ;
         virtual ~_rtnListBackupsInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   /*
      _rtnListTrans define
   */
   class _rtnListTrans : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER () ;

      public:
         _rtnListTrans () ;
         virtual ~_rtnListTrans () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   } ;

   /*
      _rtnListTransInner define
   */
   class _rtnListTransInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListTransInner () ;
         virtual ~_rtnListTransInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   /*
      _rtnListTransCurrent define
   */
   class _rtnListTransCurrent : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER () ;

      public:
         _rtnListTransCurrent () ;
         virtual ~_rtnListTransCurrent () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   } ;

   /*
      _rtnListTransCurrentInner define
   */
   class _rtnListTransCurrentInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListTransCurrentInner () ;
         virtual ~_rtnListTransCurrentInner () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

}

#endif //RTN_COMMAND_LIST_HPP_

