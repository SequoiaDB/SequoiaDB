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
#include "monDump.hpp"
#include "rtnFetchBase.hpp"

using namespace bson ;

namespace engine
{

   class _rtnListInner : public _rtnMonInnerBase
   {
      protected:
         _rtnListInner(CHAR* name,
                       RTN_COMMAND_TYPE type,
                       INT32 fetchType,
                       UINT32 infoMask )
           : _rtnMonInnerBase(name, type, fetchType, infoMask) {}
         virtual ~_rtnListInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const = 0;
         virtual BOOLEAN _isDetail() const { return FALSE ; }

      private:
   } ;

   class _rtnList : public _rtnMonBase
   {
      protected:
         _rtnList(const CHAR* name,
                  const CHAR* intrName,
                  RTN_COMMAND_TYPE type,
                  INT32 fetchType,
                  UINT32 infoMask )
           : _rtnMonBase(name, intrName, type, fetchType, infoMask) {}
         virtual ~_rtnList () {}

      protected:
         virtual BOOLEAN _isCurrent() const = 0;
         virtual BOOLEAN _isDetail() const { return FALSE ; }
   } ;

   /*
      _rtnListCollections define
   */
   class _rtnListCollections : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollections () :
            _rtnList( NAME_LIST_COLLECTIONS,
                      CMD_NAME_LIST_COLLECTION_INTR,
                      CMD_LIST_COLLECTIONS,
                      RTN_FETCH_COLLECTION,
                      0 )
         {}

         virtual ~_rtnListCollections () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;

         virtual INT32 _checkPrivileges() const ;
   };

   /*
      _rtnListCollectionsInner define
   */
   class _rtnListCollectionsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollectionsInner () :
            _rtnListInner( CMD_NAME_LIST_COLLECTION_INTR,
                           CMD_LIST_COLLECTIONS,
                           RTN_FETCH_COLLECTION,
                           0 )
         {}
         virtual ~_rtnListCollectionsInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   /*
      _rtnListCollectionspaces define
   */
   class _rtnListCollectionspaces : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollectionspaces () :
            _rtnList( NAME_LIST_COLLECTIONSPACES,
                      CMD_NAME_LIST_SPACE_INTR,
                      CMD_LIST_COLLECTIONSPACES,
                      RTN_FETCH_COLLECTIONSPACE,
                      0 )
         {}

         virtual ~_rtnListCollectionspaces () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   /*
      _rtnListCollectionspacesInner define
   */
   class _rtnListCollectionspacesInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListCollectionspacesInner ()
            : _rtnListInner( CMD_NAME_LIST_SPACE_INTR,
                             CMD_LIST_COLLECTIONSPACES,
                             RTN_FETCH_COLLECTIONSPACE,
                             0 )
         {}
         virtual ~_rtnListCollectionspacesInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListIndexes define
   */
   class _rtnListIndexes : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnListIndexes()
            : _rtnList( NAME_LIST_INDEXES,
                        CMD_NAME_LIST_INDEXES_INTR,
                        CMD_LIST_INDEXES,
                        RTN_FETCH_INDEX,
                        0 )
         {}
         virtual ~_rtnListIndexes () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   class _rtnListIndexesInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListIndexesInner()
            : _rtnListInner( CMD_NAME_LIST_INDEXES_INTR,
                             CMD_LIST_INDEXES,
                             RTN_FETCH_INDEX,
                             0 )
         {}
         virtual ~_rtnListIndexesInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListContexts define
   */
   class _rtnListContexts : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContexts ()
            : _rtnList( NAME_LIST_CONTEXTS,
                        CMD_NAME_LIST_CONTEXT_INTR,
                        CMD_LIST_CONTEXTS,
                        RTN_FETCH_CONTEXT,
                        MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnListContexts () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListContextsInner define
   */
   class _rtnListContextsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContextsInner ()
            : _rtnListInner( CMD_NAME_LIST_CONTEXT_INTR,
                             CMD_LIST_CONTEXTS,
                             RTN_FETCH_CONTEXT,
                             MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnListContextsInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListContextsCurrent define
   */
   class _rtnListContextsCurrent : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContextsCurrent ()
            : _rtnList( NAME_LIST_CONTEXTS_CURRENT,
                        CMD_NAME_LIST_CONTEXTCUR_INTR,
                        CMD_LIST_CONTEXTS_CURRENT,
                        RTN_FETCH_CONTEXT,
                        MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListContextsCurrent () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   /*
      _rtnListContextsCurrentInner define
   */
   class _rtnListContextsCurrentInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListContextsCurrentInner ()
            : _rtnListInner( CMD_NAME_LIST_CONTEXTCUR_INTR,
                             CMD_LIST_CONTEXTS_CURRENT,
                             RTN_FETCH_CONTEXT,
                             MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListContextsCurrentInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListSessions define
   */
   class _rtnListSessions : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessions ()
            : _rtnList( NAME_LIST_SESSIONS,
                        CMD_NAME_LIST_SESSION_INTR,
                        CMD_LIST_SESSIONS,
                        RTN_FETCH_SESSION,
                        MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListSessions () {}


      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   /*
      _rtnListSessionsInner define
   */
   class _rtnListSessionsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessionsInner ()
            : _rtnListInner( CMD_NAME_LIST_SESSION_INTR,
                             CMD_LIST_SESSIONS,
                             RTN_FETCH_SESSION,
                             MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListSessionsInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListSessionsCurrent implement
   */
   class _rtnListSessionsCurrent : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessionsCurrent ()
            : _rtnList( NAME_LIST_SESSIONS_CURRENT,
                        CMD_NAME_LIST_SESSIONCUR_INTR,
                        CMD_LIST_SESSIONS_CURRENT,
                        RTN_FETCH_SESSION,
                        MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListSessionsCurrent () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   /*
      _rtnListSessionsCurrentInner define
   */
   class _rtnListSessionsCurrentInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSessionsCurrentInner ()
            : _rtnListInner( CMD_NAME_LIST_SESSIONCUR_INTR,
                             CMD_LIST_SESSIONS_CURRENT,
                             RTN_FETCH_SESSION,
                             MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListSessionsCurrentInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListStorageUnits define
   */
   class _rtnListStorageUnits : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListStorageUnits ()
            : _rtnList( NAME_LIST_STORAGEUNITS,
                        CMD_NAME_LIST_STORAGEUNIT_INTR,
                        CMD_LIST_STORAGEUNITS,
                        RTN_FETCH_STORAGEUNIT,
                        MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListStorageUnits () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListStorageUnitsInner define
   */
   class _rtnListStorageUnitsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListStorageUnitsInner ()
            : _rtnListInner( CMD_NAME_LIST_STORAGEUNIT_INTR,
                             CMD_LIST_STORAGEUNITS,
                             RTN_FETCH_STORAGEUNIT,
                             MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListStorageUnitsInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListBackups define
   */
   class _rtnListBackups : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER () ;

      public:
         _rtnListBackups ()
            : _rtnList( NAME_LIST_BACKUPS,
                        CMD_NAME_LIST_BACKUP_INTR,
                        CMD_LIST_BACKUPS,
                        RTN_FETCH_BACKUP,
                        MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListBackups () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   /*
      _rtnListBackupsInner define
   */
   class _rtnListBackupsInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListBackupsInner ()
            : _rtnListInner( CMD_NAME_LIST_BACKUP_INTR,
                             CMD_LIST_BACKUPS,
                             RTN_FETCH_BACKUP,
                             MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListBackupsInner () {}
      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   /*
      _rtnListTrans define
   */
   class _rtnListTrans : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER () ;

      public:
         _rtnListTrans ()
            : _rtnList( CMD_NAME_LIST_TRANSACTIONS,
                        CMD_NAME_LIST_TRANS_INTR,
                        CMD_LIST_TRANS,
                        RTN_FETCH_TRANS,
                        MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}
         virtual ~_rtnListTrans () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListTransInner define
   */
   class _rtnListTransInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListTransInner ()
            : _rtnListInner( CMD_NAME_LIST_TRANS_INTR,
                             CMD_LIST_TRANS,
                             RTN_FETCH_TRANS,
                             MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}
         virtual ~_rtnListTransInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListTransCurrent define
   */
   class _rtnListTransCurrent : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER () ;

      public:
         _rtnListTransCurrent ()
            : _rtnList( CMD_NAME_LIST_TRANSACTIONS_CUR,
                        CMD_NAME_LIST_TRANSCUR_INTR,
                        CMD_LIST_TRANS,
                        RTN_FETCH_TRANS,
                        MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}
         virtual ~_rtnListTransCurrent () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListTransCurrentInner define
   */
   class _rtnListTransCurrentInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListTransCurrentInner ()
            : _rtnListInner( CMD_NAME_LIST_TRANSCUR_INTR,
                             CMD_LIST_TRANS_CURRENT,
                             RTN_FETCH_TRANS,
                             MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}
         virtual ~_rtnListTransCurrentInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnListSvcTasks : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSvcTasks()
            : _rtnList( CMD_NAME_LIST_SVCTASKS,
                        CMD_NAME_LIST_SVCTASKS_INTR,
                        CMD_LIST_SVCTASKS,
                        RTN_FETCH_SVCTASKS,
                        MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListSvcTasks() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnListSvcTasksInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnListSvcTasksInner()
            : _rtnListInner( CMD_NAME_LIST_SVCTASKS_INTR,
                             CMD_LIST_SVCTASKS,
                             RTN_FETCH_SVCTASKS,
                             MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnListSvcTasksInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListRecycleBin define
    */
   class _rtnListRecycleBin : public _rtnList
   {
      DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnListRecycleBin()
      : _rtnList( CMD_NAME_LIST_RECYCLEBIN,
                  CMD_NAME_LIST_RECYCLEBIN_INTR,
                  CMD_LIST_RECYCLEBIN,
                  RTN_FETCH_RECYCLEBIN,
                  0 )
      {}

      virtual ~_rtnListRecycleBin() {}

   protected:
      virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnListRecycleBinInner define
    */
   class _rtnListRecycleBinInner : public _rtnListInner
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnListRecycleBinInner()
      : _rtnListInner( CMD_NAME_LIST_RECYCLEBIN_INTR,
                       CMD_LIST_RECYCLEBIN,
                       RTN_FETCH_RECYCLEBIN,
                       0 )
      {}

      virtual ~_rtnListRecycleBinInner() {}

   protected:
      virtual BOOLEAN _isCurrent() const ;
   } ;

}

#endif //RTN_COMMAND_LIST_HPP_
