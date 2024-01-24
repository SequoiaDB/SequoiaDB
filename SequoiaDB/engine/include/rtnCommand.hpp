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

   Source File Name = rtnCommand.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_COMMAND_HPP_
#define RTN_COMMAND_HPP_

#include "rtnCommandDef.hpp"
#include <string>
#include <vector>
#include "../bson/bson.h"
#include "dms.hpp"
#include "migLoad.hpp"
#include "rtnAlterJob.hpp"
#include "rtnContextBuff.hpp"
#include "rtnQueryOptions.hpp"
#include "rtnSessionProperty.hpp"
#include "utilResult.hpp"
#include "utilGlobalID.hpp"
#include "utilRecycleBinConf.hpp"
#include "utilRecycleItem.hpp"
#include "utilRecycleReturnInfo.hpp"

using namespace bson ;

namespace engine
{

#define DECLARE_CMD_AUTO_REGISTER() \
   public: \
      static _rtnCommand *newThis () ; \

#define IMPLEMENT_CMD_AUTO_REGISTER(theClass) \
   _rtnCommand *theClass::newThis () \
   { \
      return SDB_OSS_NEW theClass() ;\
   } \
   _rtnCmdAssit theClass##Assit ( theClass::newThis ) ; \

   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;
   class _rtnCommand : public utilPooledObject
   {
      public:
         _rtnCommand () ;
         virtual ~_rtnCommand () ;

         void  setFromService( INT32 fromService ) ;
         INT32 getFromService() const { return _fromService ; }

         BOOLEAN                 hasBuff() const ;
         const rtnContextBuf&    getBuff() const ;

         virtual INT32 spaceNode () ;
         virtual INT32 spaceService () ;
         virtual utilResult* getResult() { return NULL ; }

      public:
         virtual const CHAR * name () = 0 ;
         virtual RTN_COMMAND_TYPE type () = 0 ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;
         virtual const CHAR * spaceName() ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) = 0 ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) = 0 ;

      protected:
         INT32             _fromService ;
         rtnContextBuf     _buff ;

   };

   typedef _rtnCommand* (*CDM_NEW_FUNC)() ;

   class _rtnCmdAssit : public SDBObject
   {
      public:
         _rtnCmdAssit ( CDM_NEW_FUNC pFunc ) ;
         ~_rtnCmdAssit () ;
   };

   struct _cmdBuilderInfo : public SDBObject
   {
   public :
      std::string    cmdName ;
      UINT32         nameSize ;
      CDM_NEW_FUNC   createFunc ;

      _cmdBuilderInfo *sub ;
      _cmdBuilderInfo *next ;
   } ;

   class _rtnCmdBuilder : public SDBObject
   {
      friend class _rtnCmdAssit ;

      public:
         _rtnCmdBuilder () ;
         ~_rtnCmdBuilder () ;
      public:
         _rtnCommand *create ( const CHAR *command ) ;
         void         release ( _rtnCommand *pCommand ) ;

      protected:
         INT32 _register ( const CHAR * name, CDM_NEW_FUNC pFunc ) ;

         INT32        _insert ( _cmdBuilderInfo * pCmdInfo,
                                const CHAR * name, CDM_NEW_FUNC pFunc ) ;
         CDM_NEW_FUNC _find ( const CHAR * name ) ;

         void _releaseCmdInfo ( _cmdBuilderInfo *pCmdInfo ) ;

         UINT32 _near ( const CHAR *str1, const CHAR *str2 ) ;

      private:
         _cmdBuilderInfo                     *_pCmdInfoRoot ;

   };

   _rtnCmdBuilder * getRtnCmdBuilder () ;


   //Command list
   class _rtnCoordOnly : public _rtnCommand
   {
      protected:
         _rtnCoordOnly () {}
      public:
         virtual ~_rtnCoordOnly () {}
         virtual INT32 spaceNode () { return CMD_SPACE_NODE_COORD ; }
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff )
         { return SDB_RTN_COORD_ONLY ; }
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL )
         { return SDB_RTN_COORD_ONLY ; }
   };

   class _rtnCreateGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateGroup () {}
         virtual ~_rtnCreateGroup () {}
         virtual const CHAR * name () { return NAME_CREATE_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_GROUP ; }
   } ;

   class _rtnRemoveGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnRemoveGroup () {}
         virtual ~_rtnRemoveGroup () {}
         virtual const CHAR * name () { return NAME_REMOVE_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_GROUP ; }
   };

   class _rtnCreateNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateNode () {}
         virtual ~_rtnCreateNode () {}
         virtual const CHAR * name () { return NAME_CREATE_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_NODE ; }
   } ;

   class _rtnRemoveNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnRemoveNode () {}
         virtual ~_rtnRemoveNode () {}
         virtual const CHAR * name () { return NAME_REMOVE_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_NODE ; }
   };

   class _rtnUpdateNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnUpdateNode () {}
         virtual ~_rtnUpdateNode () {}
         virtual const CHAR * name () { return NAME_UPDATE_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_UPDATE_NODE ; }
   } ;

   class _rtnActiveGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnActiveGroup () {}
         virtual ~_rtnActiveGroup () {}
         virtual const CHAR * name () { return NAME_ACTIVE_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ACTIVE_GROUP ; }
   } ;

   class _rtnStartNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnStartNode () {}
         virtual ~_rtnStartNode () {}
         virtual const CHAR * name () { return NAME_START_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_START_NODE ; }
   };

   class _rtnShutdownNode : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnShutdownNode () {}
         virtual ~_rtnShutdownNode () {}
         virtual const CHAR * name () { return NAME_SHUTDOWN_NODE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SHUTDOWN_NODE ; }
   };

   class _rtnShutdownGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnShutdownGroup () {}
         virtual ~_rtnShutdownGroup () {}
         virtual const CHAR * name () { return NAME_SHUTDOWN_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SHUTDOWN_GROUP ; }
   };

   class _rtnGetConfig : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnGetConfig () {}
         virtual ~_rtnGetConfig () {}
         virtual const CHAR * name () { return NAME_GET_CONFIG ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GET_CONFIG ; }
   } ;

   class _rtnListGroups : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListGroups () {}
         virtual ~_rtnListGroups () {}
         virtual const CHAR * name () { return NAME_LIST_GROUPS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_GROUPS ; }
   };

   class _rtnListProcedures : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListProcedures () {}
         virtual ~_rtnListProcedures () {}
         virtual const CHAR * name () { return NAME_LIST_PROCEDURES ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_PROCEDURES ; }
   } ;

   class _rtnCreateProcedure : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateProcedure () {}
         virtual ~_rtnCreateProcedure () {}
         virtual const CHAR * name () { return NAME_CREATE_PROCEDURE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_PROCEDURE ; }
   } ;

   class _rtnRemoveProcedure : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnRemoveProcedure () {}
         virtual ~_rtnRemoveProcedure () {}
         virtual const CHAR * name () { return NAME_REMOVE_PROCEDURE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_PROCEDURE ; }
   } ;

   class _rtnListCSInDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListCSInDomain () {}
         virtual ~_rtnListCSInDomain () {}
         virtual const CHAR * name () { return NAME_LIST_CS_IN_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_CS_IN_DOMAIN ; }
   } ;

   class _rtnListCLInDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListCLInDomain () {}
         virtual ~_rtnListCLInDomain () {}
         virtual const CHAR * name () { return NAME_LIST_CL_IN_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_CL_IN_DOMAIN ; }
   } ;

   class _rtnCreateCataGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateCataGroup () {}
         virtual ~_rtnCreateCataGroup () {}
         virtual const CHAR * name () { return NAME_CREATE_CATAGROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_CATAGROUP ; }
   };

   class _rtnCreateDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateDomain () {}
         virtual ~_rtnCreateDomain () {}
         virtual const CHAR * name () { return NAME_CREATE_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_DOMAIN ; }
   } ;

   class _rtnDropDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnDropDomain () {}
         virtual ~_rtnDropDomain () {}
         virtual const CHAR * name () { return NAME_DROP_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_DROP_DOMAIN ; }
   } ;

   class _rtnAlterDomain : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnAlterDomain () {}
         virtual ~_rtnAlterDomain () {}
         virtual const CHAR * name () { return NAME_ALTER_DOMAIN ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ALTER_DOMAIN ; }
   } ;

   class _rtnAddDomainGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnAddDomainGroup () {}
         virtual ~_rtnAddDomainGroup () {}
         virtual const CHAR * name () { return NAME_ADD_DOMAIN_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ADD_DOMAIN_GROUP ; }
   };

   class _rtnRemoveDomainGroup : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnRemoveDomainGroup () {}
         virtual ~_rtnRemoveDomainGroup () {}
         virtual const CHAR * name () { return NAME_REMOVE_DOMAIN_GROUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_DOMAIN_GROUP ; }
   };

   class _rtnListDomains : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListDomains () {}
         virtual ~_rtnListDomains () {}
         virtual const CHAR * name () { return NAME_LIST_DOMAINS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_DOMAINS ; }
   };

   class _rtnSnapshotCata : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnSnapshotCata () {}
         virtual ~_rtnSnapshotCata () {}
         virtual const CHAR * name () { return NAME_SNAPSHOT_CATA ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SNAPSHOT_CATA ; }
   };

   class _rtnSnapshotCataIntr : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnSnapshotCataIntr () {}
         virtual ~_rtnSnapshotCataIntr () {}
         virtual const CHAR * name () { return CMD_NAME_SNAPSHOT_CATA_INTR ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SNAPSHOT_CATA ; }
   } ;

   class _rtnWaitTask : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnWaitTask () {}
         virtual ~_rtnWaitTask () {}
         virtual const CHAR * name () { return NAME_WAITTASK ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_WAITTASK ; }
   } ;

   class _rtnListTask : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListTask () {}
         virtual ~_rtnListTask () {}
         virtual const CHAR * name () { return NAME_LIST_TASKS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_TASKS ; }
   } ;

   class _rtnListUsers : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListUsers () {}
         virtual ~_rtnListUsers () {}
         virtual const CHAR * name () { return NAME_LIST_USERS ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_USERS ; }
   } ;

   class _rtnGetDCInfo : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnGetDCInfo () {}
         virtual ~_rtnGetDCInfo () {}
         virtual const CHAR * name () { return NAME_GET_DCINFO ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GET_DCINFO ; }
   } ;

   class _rtnSnapshotSequences : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnSnapshotSequences () {}
         virtual ~_rtnSnapshotSequences () {}
         virtual const CHAR * name () { return NAME_SNAP_SEQUENCES ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SNAPSHOT_SEQUENCES ; }
   };

   class _rtnSnapshotSequencesIntr : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnSnapshotSequencesIntr () {}
         virtual ~_rtnSnapshotSequencesIntr () {}
         virtual const CHAR * name () { return CMD_NAME_SNAPSHOT_SEQUENCES_INTR ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SNAPSHOT_SEQUENCES ; }
   } ;

   class _rtnListSequences : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListSequences () {}
         virtual ~_rtnListSequences () {}
         virtual const CHAR * name () { return NAME_LIST_SEQUENCES ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_SEQUENCES ; }
   } ;

   class _rtnCreateSequence : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCreateSequence () {}
         virtual ~_rtnCreateSequence () {}
         virtual const CHAR * name () { return NAME_CREATE_SEQUENCE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_SEQUENCE ; }
   } ;

   class _rtnDropSequence : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnDropSequence () {}
         virtual ~_rtnDropSequence () {}
         virtual const CHAR * name () { return NAME_DROP_SEQUENCE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_DROP_SEQUENCE ; }
   } ;

   class _rtnAlterSequence : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnAlterSequence () {}
         virtual ~_rtnAlterSequence () {}
         virtual const CHAR * name () { return NAME_ALTER_SEQUENCE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ALTER_SEQUENCE ; }
   } ;

   class _rtnListDataSources : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnListDataSources() {}
         virtual ~_rtnListDataSources() {}
         virtual const CHAR *name () { return NAME_LIST_DATASOURCES ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_DATASOURCES ; }
   } ;

   class _rtnGetRecycleBinDetail : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnGetRecycleBinDetail() {}
      virtual ~_rtnGetRecycleBinDetail() {}
      virtual const CHAR *name() { return NAME_GET_RECYCLEBIN_DETAIL ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_GET_RECYCLEBIN_DETAIL ; }
   } ;

   class _rtnBackup : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnBackup () ;
         virtual ~_rtnBackup () ;

         virtual BOOLEAN      writable () { return FALSE ; }
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      protected:
         const CHAR        *_matherBuff ;

         const CHAR        *_backupName ;
         const CHAR        *_path ;
         const CHAR        *_desp ;
         BOOLEAN           _ensureInc ;
         BOOLEAN           _rewrite ;

   };

   class _rtnCreateCollection : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCreateCollection () ;
         virtual ~_rtnCreateCollection () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

         void setCLUniqueID( utilCLUniqueID clUniqueID ) ;

      private:
         void _clean( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB ) ;
      protected:
         const CHAR              *_collectionName ;
         BSONObj                 _shardingKey ;
         UINT32                  _attributes ;
         utilCLUniqueID          _clUniqueID ;
         UTIL_COMPRESSOR_TYPE    _compressorType ;
         BSONObj                 _extOptions ; // Store options accorrding to attributes.
         BSONObj                 _idIdxDef ;
         BSONObj                 _shardIdxDef ;
   };

   class _rtnCreateCollectionspace : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

     public:
         _rtnCreateCollectionspace () ;
         virtual ~_rtnCreateCollectionspace () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR*  spaceName() ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

     protected:
         const CHAR                 *_spaceName ;
         utilCSUniqueID             _csUniqueID ;
         INT32                      _pageSize ;
         INT32                      _lobPageSize ;
         DMS_STORAGE_TYPE           _storageType ;
   };

   class _rtnDropCollection : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnDropCollection () ;
         virtual ~_rtnDropCollection () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

      protected:
         const CHAR           *_collectionName ;
         utilRecycleItem      _recycleItem ;
   };

   class _rtnDropCollectionspace : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnDropCollectionspace () ;
         virtual ~_rtnDropCollectionspace () ;

         virtual const CHAR *spaceName () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR           *_spaceName ;
         utilRecycleItem      _recycleItem ;
         BOOLEAN              _ensureEmpty ;
   };

   class _rtnGet : public _rtnCommand
   {
      protected:
         _rtnGet () ;
         virtual ~_rtnGet () ;
      public:
         virtual const CHAR * collectionFullName () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

         virtual void setMainCLName ( const CHAR * mainCL )
         {
            _options.setMainCLName( mainCL ) ;
         }

      protected:
         rtnQueryOptions      _options ;
         BOOLEAN              _hintExist ;
   } ;

   class _rtnGetCount : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetCount () ;
         virtual ~_rtnGetCount () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnGetDatablocks : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetDatablocks () ;
         virtual ~_rtnGetDatablocks () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnGetQueryMeta : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetQueryMeta () ;
         virtual ~_rtnGetQueryMeta () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

   } ;

   class _rtnGetCollectionDetail : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetCollectionDetail () ;
         virtual ~_rtnGetCollectionDetail () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnGetCollectionStat : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetCollectionStat () ;
         virtual ~_rtnGetCollectionStat () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnGetIndexStat : public _rtnGet
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetIndexStat () ;
         virtual ~_rtnGetIndexStat () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR * pMatcherBuff,
                              const CHAR * pSelectBuff,
                              const CHAR * pOrderByBuff,
                              const CHAR * pHintBuff ) ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   /*
      Only in standalone
   */
   class _rtnRenameCollection : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnRenameCollection () ;
         virtual ~_rtnRenameCollection () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN      writable () ;
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

      protected:
         const CHAR           *_clShortName ;
         const CHAR           *_newCLShortName ;
         const CHAR           *_csName ;
         std::string          _fullCollectionName ;
   } ;

   class _rtnRenameCollectionSpace : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnRenameCollectionSpace () ;
         virtual ~_rtnRenameCollectionSpace () ;

         virtual const CHAR * name () { return NAME_RENAME_COLLECTIONSPACE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_RENAME_COLLECTIONSPACE ; }
         virtual BOOLEAN      writable () { return TRUE ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR           *_oldName ;
         const CHAR           *_newName ;
   } ;

   class _rtnReorg : public _rtnCommand
   {
      protected:
         _rtnReorg () ;
         virtual ~_rtnReorg () ;
         virtual INT32 spaceNode () ;
      public:
         virtual const CHAR * collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR           *_collectionName ;
         const CHAR           *_hintBuffer ;

   };

   class _rtnReorgOffline : public _rtnReorg
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnReorgOffline () ;
         virtual ~_rtnReorgOffline () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnReorgOnline : public _rtnReorg
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnReorgOnline () ;
         virtual ~_rtnReorgOnline () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnReorgRecover : public _rtnReorg
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnReorgRecover () ;
         virtual ~_rtnReorgRecover () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
   };

   class _rtnShutdown : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnShutdown () ;
         virtual ~_rtnShutdown () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
   };

   class _rtnTest : public _rtnCommand
   {
      protected:
         _rtnTest () ;
         virtual ~_rtnTest () ;
      public:
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
      protected:
         const CHAR        *_objName ;
   };

   class _rtnTestCollection : public _rtnTest
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTestCollection () ;
         virtual ~_rtnTestCollection () ;

         virtual const CHAR * collectionFullName () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
   };

   class _rtnTestCollectionspace : public _rtnTest
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTestCollectionspace () ;
         virtual ~_rtnTestCollectionspace () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
   };

   class _rtnSetPDLevel : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSetPDLevel () ;
         virtual ~_rtnSetPDLevel () ;

      public:
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      protected:
         INT32             _pdLevel ;
   } ;

   class _rtnReloadConfig : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnReloadConfig() ;
         virtual ~_rtnReloadConfig() ;

      public:
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
   } ;

   class _configOprBase : public _rtnCommand
   {
      protected:
         _configOprBase() ;
         virtual ~_configOprBase() ;
         virtual INT32 _errorReport( BSONObj &returnObj ) ;
   } ;

   class _rtnUpdateConfig : public _configOprBase
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnUpdateConfig() ;
         virtual ~_rtnUpdateConfig() ;

      public:
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      private:
         BSONObj _newCfgObj ;
         BOOLEAN _isForce ;
   } ;

   class _rtnDeleteConfig : public _configOprBase
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnDeleteConfig() ;
         virtual ~_rtnDeleteConfig() ;

      public:
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

      private:
         virtual INT32 _fillAliasNameToDelConf() ;

      private:
         BSONObj _newCfgObj ;
         BOOLEAN _isForce ;
   } ;

   class _rtnTraceStart : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTraceStart () ;
         virtual ~_rtnTraceStart () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

      private:
         BOOLEAN _isFunctionNameValid( const CHAR* verifiedFuncName ) ;
         INT32   _initComponents( const BSONElement &componentsEle ) ;
         INT32   _initBreakpoints( const BSONElement &breakpointsEle ) ;
         INT32   _initTids( const BSONElement &tidsEle ) ;
         INT32   _initFunctionNames( const BSONElement &functionNamesEle ) ;
         INT32   _initThreadTypes( const BSONElement &threadTypesEle ) ;

      protected :
         UINT32 _mask ;
         std::vector<UINT32>  _tid ;
         std::vector<UINT64>  _breakpoint ;
         std::vector<UINT64>  _functionNameId ;
         std::vector<INT32>   _threadType ;
         UINT32 _size ;
   };

   class _rtnTraceResume : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTraceResume () ;
         virtual ~_rtnTraceResume () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
   };

   class _rtnTraceStop : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTraceStop () ;
         virtual ~_rtnTraceStop () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      protected :
         const CHAR *_pDumpFileName ;
   };

   class _rtnTraceStatus : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnTraceStatus () ;
         virtual ~_rtnTraceStatus () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      protected:
         INT64                _numToReturn ;
         INT64                _numToSkip ;
         const CHAR           *_matcherBuff ;
         const CHAR           *_selectBuff ;
         const CHAR           *_orderByBuff ;
         INT32                _flags ;
   };

   class _rtnLoad : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnLoad () ;
         virtual ~_rtnLoad () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      protected :
         setParameters _parameters ;
         CHAR     _fileName[ OSS_MAX_PATHSIZE + 1 ] ;
         CHAR     _csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ]   ;
         CHAR     _clName[ DMS_COLLECTION_NAME_SZ + 1 ]   ;
   };

   class _rtnExportConf : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnExportConf() ;
         virtual ~_rtnExportConf(){}

         virtual const CHAR * name () { return NAME_EXPORT_CONFIGURATION ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_EXPORT_CONFIG ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;

         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

      private:
         UINT32            _mask ;

   } ;

   class _rtnRemoveBackup : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER ()

      public:
         _rtnRemoveBackup () ;
         virtual ~_rtnRemoveBackup () {}

      public:
         virtual const CHAR * name () { return NAME_REMOVE_BACKUP ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REMOVE_BACKUP ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

      protected:
         const CHAR              *_path ;
         const CHAR              *_backupName ;
         const CHAR              *_matcherBuff ;

   } ;

   class _rtnForceSession : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnForceSession() ;
      virtual ~_rtnForceSession() ;

   public:
      virtual const CHAR * name () { return NAME_FORCE_SESSION ; }
      virtual RTN_COMMAND_TYPE type () { return CMD_FORCE_SESSION ; }
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL  ) ;

   private:
      EDUID _sessionID ;
   } ;
   typedef class _rtnForceSession rtnForceSession ;

   class _rtnListLob : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnListLob() ;
      virtual ~_rtnListLob() ;

   public:
      virtual const CHAR * name () { return NAME_LIST_LOBS ; }
      virtual RTN_COMMAND_TYPE type () { return CMD_LIST_LOB ; }
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL  ) ;
      virtual const CHAR *collectionFullName() ;

   private:
      INT64 _contextID ;
      bson::BSONObj _query ;
      bson::BSONObj _selector ;
      bson::BSONObj _orderBy ;
      bson::BSONObj _hint ;
      INT64 _skip ;
      INT64 _returnNum ;
      const CHAR *_fullName ;
   } ;

   /*
      _rtnLocalSessionProp define
   */
   class _rtnLocalSessionProp : public _rtnSessionProperty
   {
      public:
         _rtnLocalSessionProp() ;

         void           bindEDU( _pmdEDUCB *cb ) ;

      protected:
         virtual void   _toBson( BSONObjBuilder &builder ) const ;
         virtual INT32  _checkTransConf( const _dpsTransConfItem *pTransConf ) ;
         virtual void   _updateTransConf( const _dpsTransConfItem *pTransConf ) ;
         virtual void   _updateSource( const CHAR *pSource ) ;

      private:
         _pmdEDUCB      *_cb ;
   } ;
   typedef _rtnLocalSessionProp rtnLocalSessionProp ;

   class _rtnSetSessionAttr : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSetSessionAttr() { _pMatchBuff = NULL ; }
         virtual ~_rtnSetSessionAttr() {}

      public:
         virtual const CHAR * name () { return NAME_SET_SESSIONATTR ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_SET_SESSIONATTR ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

      private:
         const CHAR     *_pMatchBuff ;
   } ;

   class _rtnGetSessionAttr : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnGetSessionAttr () ;
         virtual ~_rtnGetSessionAttr () ;

      public:
         virtual const CHAR * name () { return NAME_GET_SESSIONATTR ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GET_SESSIONATTR ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR * pMatcherBuff,
                              const CHAR * pSelectBuff,
                              const CHAR * pOrderByBuff,
                              const CHAR * pHintBuff ) ;

         virtual INT32 doit ( _pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                              _SDB_RTNCB * rtnCB, _dpsLogWrapper * dpsCB,
                              INT16 w = 1, INT64 * pContextID = NULL ) ;
   } ;

   class _rtnTruncate : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnTruncate()
      : _collectionName( NULL )
      {
      }

      virtual ~_rtnTruncate() {}

   public:
      virtual const CHAR * name () { return NAME_TRUNCATE ; }
      virtual RTN_COMMAND_TYPE type () { return CMD_TRUNCATE ; }
      virtual BOOLEAN writable()
      {
         return TRUE ;
      }

      virtual const CHAR * collectionFullName()
      {
         return _collectionName ;
      }

      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

   private:
      const CHAR * _collectionName ;
      utilRecycleItem _recycleItem ;
   } ;

   class _rtnPop : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnPop()
      : _fullName( NULL ),
        _logicalID( 0 ),
        _direction( 1 )
      {
      }

      virtual ~_rtnPop() {}

   public:
      virtual const CHAR *name() { return NAME_POP ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_POP; }
      virtual BOOLEAN writable()
      {
         return TRUE ;
      }

      virtual const CHAR* collectionFullName()
      {
         return _fullName ;
      }

      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

   private:
      const CHAR *_fullName ;
      INT64 _logicalID ;
      INT8 _direction ;
   };

   class _rtnSyncDB : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnSyncDB() ;
      virtual ~_rtnSyncDB() ;

   public:
      virtual const CHAR * name () { return NAME_SYNC_DB ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_SYNC_DB ; }
      virtual BOOLEAN writable() { return FALSE ; }
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

   private:
      INT32          _syncType ;
      const CHAR     *_csName ;
      BOOLEAN        _block ;

   } ;

   class _rtnLoadCollectionSpace : public _rtnCommand
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnLoadCollectionSpace() ;
      virtual ~_rtnLoadCollectionSpace() ;

   public:
      virtual const CHAR * name () { return NAME_LOAD_COLLECTIONSPACE ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_LOAD_COLLECTIONSPACE ; }

      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

      virtual const CHAR* spaceName () { return _csName ; }
      INT32 setUniqueID( utilCSUniqueID csUniqueID,
                         const BSONObj& clInfoObj ) ;
      ossPoolVector<BSONObj>& getIndexVector() { return _idxInfoVector ; }

   protected:
      const CHAR                *_csName ;
      BOOLEAN                    _needChgID ;
      utilCSUniqueID             _csUniqueID ;
      BSONObj                    _clInfoObj ;
      ossPoolVector<BSONObj>     _idxInfoVector ;
   } ;

   class _rtnUnloadCollectionSpace : public _rtnLoadCollectionSpace
   {
   DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnUnloadCollectionSpace() ;
      virtual ~_rtnUnloadCollectionSpace() ;

   public:
      virtual const CHAR * name () { return NAME_UNLOAD_COLLECTIONSPACE ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_UNLOAD_COLLECTIONSPACE ; }

      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;
   } ;

   struct _rtnAnalyzeParam
   {
      _rtnAnalyzeParam ()
      {
         _mode = SDB_ANALYZE_MODE_SAMPLE ;
         _sampleByNum = TRUE ;
         _sampleRecords = SDB_ANALYZE_SAMPLE_DEF ;
         _samplePercent = 0.0 ;
         _needCheck = TRUE ;
      }

      _rtnAnalyzeParam ( const _rtnAnalyzeParam &param )
      {
         _mode = param._mode ;
         _sampleByNum = param._sampleByNum ;
         _sampleRecords = param._sampleRecords ;
         _samplePercent = param._samplePercent ;
         _needCheck = param._needCheck ;
      }

      INT32    _mode ;
      BOOLEAN  _sampleByNum ;
      UINT32   _sampleRecords ;
      double   _samplePercent ;
      BOOLEAN  _needCheck ;
   } ;

   typedef struct _rtnAnalyzeParam rtnAnalyzeParam ;

   class _rtnAnalyze : public _rtnCommand
   {

   DECLARE_CMD_AUTO_REGISTER()

   public :

      _rtnAnalyze () ;

      virtual ~_rtnAnalyze () ;

   public :

      virtual const CHAR * name () { return NAME_ANALYZE ; }

      virtual RTN_COMMAND_TYPE type () { return CMD_ANALYZE ; }

      virtual BOOLEAN writable ()
      {
         // Reload is read-only operation
         return ( _param._mode != SDB_ANALYZE_MODE_RELOAD &&
                  _param._mode != SDB_ANALYZE_MODE_CLEAR ) ;
      }

      virtual const CHAR * collectionFullName ()
      {
         return _clname ;
      }

      const CHAR * getIndexName () const
      {
         return _ixname ;
      }

      const rtnAnalyzeParam &getAnalyzeParam () const
      {
         return _param ;
      }

      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;

      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

   private:
      INT32 _checkPrivileges ( _pmdEDUCB *cb ) ;

   private :
      const CHAR *      _csname ;
      const CHAR *      _clname ;
      const CHAR *      _ixname ;
      rtnAnalyzeParam   _param ;
   } ;

   /*
      _rtnCMDGetRecycleBinCount define
    */
   class _rtnCMDGetRecycleBinCount : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _rtnCMDGetRecycleBinCount() ;
      virtual ~_rtnCMDGetRecycleBinCount() ;

      virtual const CHAR *name() ;
      virtual RTN_COMMAND_TYPE type() ;

      virtual INT32 init( INT32 flags,
                          INT64 numToSkip,
                          INT64 numToReturn,
                          const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff,
                          const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff ) ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          _SDB_DMSCB *dmsCB,
                          _SDB_RTNCB *rtnCB,
                          _dpsLogWrapper *dpsCB,
                          INT16 w = 1,
                          INT64 *pContextID = NULL ) ;

   protected:
      bson::BSONObj _queryObj ;
   } ;

   typedef class _rtnCMDGetRecycleBinCount rtnCMDGetRecycleBinCount ;

   /*
      _rtnCMDAlterRecycleBin define
    */
   class _rtnCMDAlterRecycleBin : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnCMDAlterRecycleBin() ;
      virtual ~_rtnCMDAlterRecycleBin() ;

      virtual const CHAR *name() ;
      virtual RTN_COMMAND_TYPE type() ;

      virtual BOOLEAN writable()
      {
         // need primary node
         return TRUE ;
      }

      virtual INT32 init( INT32 flags,
                          INT64 numToSkip,
                          INT64 numToReturn,
                          const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff,
                          const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff )
      {
         // do nothing
         return SDB_OK ;
      }

      virtual INT32 doit( _pmdEDUCB *cb,
                          _SDB_DMSCB *dmsCB,
                          _SDB_RTNCB *rtnCB,
                          _dpsLogWrapper *dpsCB,
                          INT16 w = 1,
                          INT64 *pContextID = NULL ) ;

   protected:
      utilRecycleBinConf _newConf ;
   } ;

   typedef class _rtnCMDAlterRecycleBin rtnCMDAlterRecycleBin ;

   /*
      _rtnCMDDropRecycleBinBase define
    */
   class _rtnCMDDropRecycleBinBase : public _rtnCommand
   {
   public:
      _rtnCMDDropRecycleBinBase() ;
      virtual ~_rtnCMDDropRecycleBinBase() ;

      virtual BOOLEAN writable()
      {
         return TRUE ;
      }

      virtual INT32 init( INT32 flags,
                          INT64 numToSkip,
                          INT64 numToReturn,
                          const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff,
                          const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff ) ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          _SDB_DMSCB *dmsCB,
                          _SDB_RTNCB *rtnCB,
                          _dpsLogWrapper *dpsCB,
                          INT16 w = 1,
                          INT64 *pContextID = NULL ) ;

   protected:
      virtual BOOLEAN _isDropAll() const = 0 ;

   protected:
      BOOLEAN              _isAsync ;
      const CHAR *         _recycleItemName ;
   } ;

   typedef class _rtnCMDDropRecycleBinBase rtnCMDDropRecycleBinBase ;

   /*
      _rtnCMDDropRecycleBinItem define
    */
   class _rtnCMDDropRecycleBinItem : public _rtnCMDDropRecycleBinBase
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnCMDDropRecycleBinItem() ;
      virtual ~_rtnCMDDropRecycleBinItem() ;

      virtual const CHAR *name() ;
      virtual RTN_COMMAND_TYPE type() ;

   protected:
      virtual BOOLEAN _isDropAll() const
      {
         return FALSE ;
      }
   } ;

   typedef class _rtnCMDDropRecycleBinItem rtnCMDDropRecycleBinItem ;

   /*
      _rtnCMDDropRecycleBinAll define
    */
   class _rtnCMDDropRecycleBinAll : public _rtnCMDDropRecycleBinBase
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnCMDDropRecycleBinAll() ;
      virtual ~_rtnCMDDropRecycleBinAll() ;

      virtual const CHAR *name() ;
      virtual RTN_COMMAND_TYPE type() ;

   protected:
      virtual BOOLEAN _isDropAll() const
      {
         return TRUE ;
      }
   } ;

   typedef class _rtnCMDDropRecycleBinAll rtnCMDDropRecycleBinAll ;

   /*
      _rtnCMDReturnRecycleBinBase define
    */
   class _rtnCMDReturnRecycleBinBase : public _rtnCommand
   {
   public:
      _rtnCMDReturnRecycleBinBase() ;
      virtual ~_rtnCMDReturnRecycleBinBase() ;

      virtual BOOLEAN writable()
      {
         return TRUE ;
      }

      virtual INT32 init( INT32 flags,
                          INT64 numToSkip,
                          INT64 numToReturn,
                          const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff,
                          const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff ) ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          _SDB_DMSCB *dmsCB,
                          _SDB_RTNCB *rtnCB,
                          _dpsLogWrapper *dpsCB,
                          INT16 w = 1,
                          INT64 *pContextID = NULL ) ;

   protected:
      utilRecycleItem         _recycleItem ;
      utilRecycleReturnInfo   _returnInfo ;
   } ;

   typedef class _rtnCMDReturnRecycleBinBase rtnCMDReturnRecycleBinBase ;

   /*
      _rtnCMDReturnRecycleBinItem define
    */
   class _rtnCMDReturnRecycleBinItem : public _rtnCMDReturnRecycleBinBase
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnCMDReturnRecycleBinItem() ;
      virtual ~_rtnCMDReturnRecycleBinItem() ;

      virtual const CHAR *name() ;
      virtual RTN_COMMAND_TYPE type() ;
   } ;

   typedef class _rtnCMDReturnRecycleBinItem rtnCMDReturnRecycleBinItem ;

   /*
      _rtnCMDReturnRecycleBinItemToName define
    */
   class _rtnCMDReturnRecycleBinItemToName : public _rtnCMDReturnRecycleBinBase
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnCMDReturnRecycleBinItemToName() ;
      virtual ~_rtnCMDReturnRecycleBinItemToName() ;

      virtual const CHAR *name() ;
      virtual RTN_COMMAND_TYPE type() ;
   } ;

   typedef class _rtnCMDReturnRecycleBinItemToName rtnCMDReturnRecycleBinItemToName ;

   /*
      _rtnCMDInvalidateUserCache define
   */
   class _rtnCMDInvalidateUserCache : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnCMDInvalidateUserCache() ;
      virtual ~_rtnCMDInvalidateUserCache() ;

      virtual BOOLEAN writable()
      {
         return FALSE ;
      }

      virtual INT32 init( INT32 flags,
                          INT64 numToSkip,
                          INT64 numToReturn,
                          const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff,
                          const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff ) ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          _SDB_DMSCB *dmsCB,
                          _SDB_RTNCB *rtnCB,
                          _dpsLogWrapper *dpsCB,
                          INT16 w = 1,
                          INT64 *pContextID = NULL ) ;

      virtual const CHAR *name() ;
      virtual RTN_COMMAND_TYPE type() ;

   private:
      ossPoolString _userName;
   } ;
   typedef class _rtnCMDInvalidateUserCache rtnCMDInvalidateUserCache ;

   /*
      _rtnCMDCreateRole define
   */
   class _rtnCMDCreateRole : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDCreateRole () {}
         virtual ~_rtnCMDCreateRole () {}
         virtual const CHAR * name () { return NAME_CREATE_ROLE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CREATE_ROLE ; }
   } ;
   typedef class _rtnCMDCreateRole rtnCMDCreateRole ;

   /*
      _rtnCMDDropRole define
   */
   class _rtnCMDDropRole : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDDropRole () {}
         virtual ~_rtnCMDDropRole () {}
         virtual const CHAR * name () { return NAME_DROP_ROLE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_DROP_ROLE ; }
   } ;
   typedef class _rtnCMDDropRole rtnCMDDropRole ;

   /*
      _rtnCMDGetRole define
   */
   class _rtnCMDGetRole : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDGetRole () {}
         virtual ~_rtnCMDGetRole () {}
         virtual const CHAR * name () { return NAME_GET_ROLE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GET_ROLE ; }
   } ;
   typedef class _rtnCMDGetRole rtnCMDGetRole ;

   /*
      _rtnCMDListRoles define
   */
   class _rtnCMDListRoles : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDListRoles () {}
         virtual ~_rtnCMDListRoles () {}
         virtual const CHAR * name () { return NAME_LIST_ROLES ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_LIST_ROLES ; }
   } ;
   typedef class _rtnCMDListRoles rtnCMDListRoles ;

   /*
      _rtnCMDUpdateRole define
   */
   class _rtnCMDUpdateRole : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDUpdateRole () {}
         virtual ~_rtnCMDUpdateRole () {}
         virtual const CHAR * name () { return NAME_UPDATE_ROLE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_UPDATE_ROLE ; }
   } ;
   typedef class _rtnCMDUpdateRole rtnCMDUpdateRole ;

   /*
      _rtnCMDGrantPrivilegesToRole define
   */
   class _rtnCMDGrantPrivilegesToRole : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDGrantPrivilegesToRole () {}
         virtual ~_rtnCMDGrantPrivilegesToRole () {}
         virtual const CHAR * name () { return NAME_GRANT_PRIVILEGES ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GRANT_PRIVILEGES ; }
   } ;
   typedef class _rtnCMDGrantPrivilegesToRole rtnCMDGrantPrivilegesToRole ;

   /*
      _rtnCMDRevokePrivilegesFromRole define
   */
   class _rtnCMDRevokePrivilegesFromRole : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDRevokePrivilegesFromRole () {}
         virtual ~_rtnCMDRevokePrivilegesFromRole () {}
         virtual const CHAR * name () { return NAME_REVOKE_PRIVILEGES ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REVOKE_PRIVILEGES ; }
   } ;
   typedef class _rtnCMDRevokePrivilegesFromRole rtnCMDRevokePrivilegesFromRole ;

   /*
      _rtnCMDGrantRolesToRole define
   */
   class _rtnCMDGrantRolesToRole : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDGrantRolesToRole () {}
         virtual ~_rtnCMDGrantRolesToRole () {}
         virtual const CHAR * name () { return NAME_GRANT_ROLES_TO_ROLE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GRANT_ROLES_TO_ROLE ; }
   } ;
   typedef class _rtnCMDGrantRolesToRole rtnCMDGrantRolesToRole ;

   /*
      _rtnCMDRevokeRolesFromRole define
   */
   class _rtnCMDRevokeRolesFromRole : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDRevokeRolesFromRole () {}
         virtual ~_rtnCMDRevokeRolesFromRole () {}
         virtual const CHAR * name () { return NAME_REVOKE_ROLES_FROM_ROLE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REVOKE_ROLES_FROM_ROLE ; }
   } ;
   typedef class _rtnCMDRevokeRolesFromRole rtnCMDRevokeRolesFromRole ;

   /*
      _rtnCMDGetUser define
   */
   class _rtnCMDGetUser : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDGetUser () {}
         virtual ~_rtnCMDGetUser () {}
         virtual const CHAR * name () { return NAME_GET_USER ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GET_USER ; }
   } ;
   typedef class _rtnCMDGetUser rtnCMDGetUser ;

   /*
      _rtnCMDGrantRolesToUser define
   */
   class _rtnCMDGrantRolesToUser : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDGrantRolesToUser () {}
         virtual ~_rtnCMDGrantRolesToUser () {}
         virtual const CHAR * name () { return NAME_GRANT_ROLES_TO_USER ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_GRANT_ROLES_TO_USER ; }
   } ;
   typedef class _rtnCMDGrantRolesToUser rtnCMDGrantRolesToUser ;

   /*
      _rtnCMDRevokeRolesFromUser define
   */
   class _rtnCMDRevokeRolesFromUser : public _rtnCoordOnly
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCMDRevokeRolesFromUser () {}
         virtual ~_rtnCMDRevokeRolesFromUser () {}
         virtual const CHAR * name () { return NAME_REVOKE_ROLES_FROM_USER ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_REVOKE_ROLES_FROM_USER ; }
   } ;
   typedef class _rtnCMDRevokeRolesFromUser rtnCMDRevokeRolesFromUser ;

   /*
      _rtnMemTrim define
   */
   class _rtnMemTrim : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnMemTrim() ;
         virtual ~_rtnMemTrim() ;

      public:
         virtual const CHAR * name () { return NAME_MEM_TRIM ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_MEM_TRIM ; }
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      private:
         UINT32      _mask ;
   } ;

}

const UINT32 pdGetTraceFunctionListNum();

#endif //RTN_COMMAND_HPP_

