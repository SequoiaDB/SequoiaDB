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

   Source File Name = clsCommand.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/27/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_COMMAND_HPP_
#define CLS_COMMAND_HPP_

#include "rtnCommand.hpp"
#include "clsDef.hpp"
#include "utilResult.hpp"

using namespace bson ;

namespace engine
{

   class _rtnSplit : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSplit () ;
         ~_rtnSplit () ;

         virtual INT32 spaceService () ;

      public :
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
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      protected:
         CHAR _szCollection [ DMS_COLLECTION_SPACE_NAME_SZ +
                              DMS_COLLECTION_NAME_SZ + 2 ] ;
         CHAR _szTargetName [ OP_MAXNAMELENGTH + 1 ] ;
         CHAR _szSourceName [ OP_MAXNAMELENGTH + 1 ] ;
         FLOAT64 _percent  ;

         bson::BSONObj _splitKey ;
   } ;

   class _rtnCreateIndex : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCreateIndex () ;
         ~_rtnCreateIndex () ;

      public :
         virtual const CHAR*      name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN          writable () ;
         virtual const CHAR*      collectionFullName () ;
         virtual utilResult*      getResult() { return &_writeResult ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      private:
         INT32 _validateDef( const BSONObj &index ) ;
      protected:
         const CHAR             *_collectionName ;
         BSONObj                 _index ;
         const CHAR             *_indexName ;
         INT32                   _sortBufSize ;
         BOOLEAN                 _textIdx ;
         BOOLEAN                 _isGlobal ;
         utilWriteResult         _writeResult ;
         INT64                   _taskID ;
         BOOLEAN                 _isAsync ;
         BOOLEAN                 _isStandaloneIdx ;
   } ;

   class _rtnDropIndex : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnDropIndex () ;
         virtual ~_rtnDropIndex () ;

         virtual const CHAR*      name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN          writable () ;
         virtual const CHAR*      collectionFullName () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;
      protected:
         const CHAR          *_collectionName ;
         BSONObj              _index ;
         const CHAR          *_indexName ;
         INT64                _taskID ;
         BOOLEAN              _isAsync ;
         BOOLEAN              _isStandaloneIdx ;
   } ;

   class _rtnCopyIndex : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnCopyIndex () ;
         ~_rtnCopyIndex () ;

      public :
         virtual const CHAR *     name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual BOOLEAN          writable () ;
         virtual const CHAR *     collectionFullName () ;
         virtual INT32            spaceNode () ;
         virtual INT32            spaceService () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      protected:
         const CHAR* _collectionName ;
   } ;

   class _rtnCancelTask : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()
      public:
         _rtnCancelTask () { _taskID = 0 ; }
         ~_rtnCancelTask () {}
         virtual INT32 spaceNode () ;
         virtual BOOLEAN      writable () { return TRUE ; }
         virtual const CHAR * name () { return NAME_CANCEL_TASK ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_CANCEL_TASK ; }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

      protected:
         UINT64            _taskID ;

   } ;

   class _rtnLinkCollection : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnLinkCollection () ;
         ~_rtnLinkCollection () ;

         virtual INT32 spaceService () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;
         virtual const CHAR * collectionFullName () ;
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
         const CHAR           *_collectionName ;
         const CHAR           *_subCLName ;
   };

   class _rtnUnlinkCollection : public _rtnLinkCollection
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnUnlinkCollection () ;
         ~_rtnUnlinkCollection () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

   } ;

   class _rtnInvalidateCache : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()
   public:
      _rtnInvalidateCache() ;
      virtual ~_rtnInvalidateCache() ;

   public:
      virtual const CHAR *name() { return NAME_INVALIDATE_CACHE ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_INVALIDATE_CACHE ; }
      virtual INT32 spaceNode () ;
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;
   } ;

   class _rtnReelectBase : public _rtnCommand
   {
   public:
      _rtnReelectBase() ;
      virtual ~_rtnReelectBase() ;
      virtual INT32 spaceNode () ;
      virtual INT32 spaceService () ;

   protected:
      virtual INT32 _parseReelectArgs( const BSONObj &obj ) ;

   protected:
      BOOLEAN _isDestNotify ;
      INT32 _timeout ;
      UINT16 _nodeID ;
      CLS_REELECTION_LEVEL _level ;
   } ;

   class _rtnReelectGroup : public _rtnReelectBase
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnReelectGroup() ;
      virtual ~_rtnReelectGroup() ;

   public:
      virtual const CHAR * name () { return NAME_REELECT ; }
      virtual RTN_COMMAND_TYPE type () { return CMD_REELECT ; }
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;
   } ;

   class _rtnReelectLocation : public _rtnReelectBase
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnReelectLocation() ;
      virtual ~_rtnReelectLocation() ;

      virtual const CHAR * name () { return NAME_REELECT_LOCATION ; }
      virtual RTN_COMMAND_TYPE type () { return CMD_REELECT_LOCATION ; }
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

   private:
      const CHAR* _pLocation ;
   } ;

   class _rtnForceStepUp : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnForceStepUp()
      :_seconds( 120 )
      {}

      virtual ~_rtnForceStepUp() {}

   public:
      virtual const CHAR * name () { return NAME_FORCE_STEP_UP ; }
      virtual RTN_COMMAND_TYPE type () { return CMD_FORCE_STEP_UP ; }
      virtual INT32 spaceNode () ;
      virtual INT32 spaceService () ;
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

   private:
      UINT32 _seconds ;
   } ;

   class _clsAlterDC : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _clsAlterDC() ;
         virtual ~_clsAlterDC() ;

         virtual INT32 spaceService () ;

      public:
         virtual const CHAR * name () { return NAME_ALTER_DC ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ALTER_IMAGE ; }
         virtual BOOLEAN      writable () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

      private:
         const CHAR              *_pAction ;

   } ;

   /*
      _rtnAlterCommand define
    */
   class _rtnAlterCommand : public _rtnCommand,
                            public _rtnAlterJobHolder
   {
      public :
         _rtnAlterCommand () ;
         virtual ~_rtnAlterCommand () ;

      public :
         virtual BOOLEAN writable () { return TRUE ; }

         virtual INT32 init ( INT32 flags,
                              INT64 numToSkip,
                              INT64 numToReturn,
                              const CHAR * pMatcherBuff,
                              const CHAR * pSelectBuff,
                              const CHAR * pOrderByBuff,
                              const CHAR * pHintBuff ) ;

         virtual INT32 doit ( _pmdEDUCB * cb,
                              _SDB_DMSCB * dmsCB,
                              _SDB_RTNCB * rtnCB,
                              _dpsLogWrapper * dpsCB,
                              INT16 w = 1,
                              INT64 * pContextID = NULL ) ;

      protected :
         virtual RTN_ALTER_OBJECT_TYPE _getObjectType () const = 0 ;
         virtual AUDIT_OBJ_TYPE _getAuditType () const = 0 ;
         virtual INT32 _executeTask ( const CHAR * object,
                                      const rtnAlterTask * task,
                                      const rtnAlterInfo * alterInfo,
                                      const rtnAlterOptions * options,
                                      _pmdEDUCB * cb,
                                      _SDB_DMSCB * dmsCB,
                                      _SDB_RTNCB * rtnCB,
                                      _dpsLogWrapper * dpsCB,
                                      INT16 w ) = 0 ;
         virtual INT32 _openContext ( _pmdEDUCB * cb,
                                      _SDB_RTNCB * rtnCB,
                                      INT64 * pContextID = NULL ) = 0 ;
   } ;

   /*
      _rtnAlterCollectionSpace define
    */
   class _rtnAlterCollectionSpace : public _rtnAlterCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnAlterCollectionSpace () ;
         virtual ~_rtnAlterCollectionSpace () ;

      public :
         virtual const CHAR * name () { return NAME_ALTER_COLLECTION_SPACE ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ALTER_COLLECTIONSPACE ; }
         virtual const CHAR* spaceName() ;

      protected :
         virtual RTN_ALTER_OBJECT_TYPE _getObjectType () const
         {
            return RTN_ALTER_COLLECTION_SPACE ;
         }

         virtual AUDIT_OBJ_TYPE _getAuditType () const
         {
            return AUDIT_OBJ_CS ;
         }

         virtual INT32 _executeTask ( const CHAR * object,
                                      const rtnAlterTask * task,
                                      const rtnAlterInfo * alterInfo,
                                      const rtnAlterOptions * options,
                                      _pmdEDUCB * cb,
                                      _SDB_DMSCB * dmsCB,
                                      _SDB_RTNCB * rtnCB,
                                      _dpsLogWrapper * dpsCB,
                                      INT16 w ) ;

         virtual INT32 _openContext ( _pmdEDUCB * cb,
                                      _SDB_RTNCB * rtnCB,
                                      INT64 * pContextID = NULL ) ;
   } ;

   /*
      _rtnAlterCollection define
    */
   class _rtnAlterCollection : public _rtnAlterCommand
   {
         DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnAlterCollection () ;
         virtual ~_rtnAlterCollection () ;

         virtual utilResult* getResult() { return &_writeResult ; }

      public :
         virtual const CHAR * name () { return NAME_ALTER_COLLECTION ; }
         virtual RTN_COMMAND_TYPE type () { return CMD_ALTER_COLLECTION ; }
         virtual const CHAR * collectionFullName () ;

      protected :
         virtual RTN_ALTER_OBJECT_TYPE _getObjectType () const
         {
            return RTN_ALTER_COLLECTION ;
         }

         virtual AUDIT_OBJ_TYPE _getAuditType () const
         {
            return AUDIT_OBJ_CL ;
         }

         virtual INT32 _executeTask ( const CHAR * object,
                                      const rtnAlterTask * task,
                                      const rtnAlterInfo * alterInfo,
                                      const rtnAlterOptions * options,
                                      _pmdEDUCB * cb,
                                      _SDB_DMSCB * dmsCB,
                                      _SDB_RTNCB * rtnCB,
                                      _dpsLogWrapper * dpsCB,
                                      INT16 w ) ;

         virtual INT32 _openContext ( _pmdEDUCB * cb,
                                      _SDB_RTNCB * rtnCB,
                                      INT64 * pContextID = NULL ) ;

      protected:
         utilWriteResult            _writeResult ;

   } ;

   /* 
      _rtnAlterGroup define
    */
   class _rtnAlterGroup : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnAlterGroup () ;
         ~_rtnAlterGroup () ;

         virtual INT32 spaceNode () ;
         virtual INT32 spaceService () ;
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      public :
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;

      private:
         INT32 _parseOptions() ;

      private:
         UINT32                 _groupID ;
         const CHAR*            _pActionName ;
         BSONObj                _option ;
         BOOLEAN                _enforced ;
         clsGroupMode           _grpMode ;
   } ;

}


#endif //CLS_COMMAND_HPP_

