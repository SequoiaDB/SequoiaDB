/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnExtDataHandler.hpp

   Descriptive Name = External data process handler for rtn.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_EXTDATAHANDLER_HPP__
#define RTN_EXTDATAHANDLER_HPP__

#include "pmdEDU.hpp"
#include "dpsLogWrapper.hpp"
#include "dmsExtDataHandler.hpp"
#include "rtnExtDataProcessor.hpp"

namespace engine
{
   enum _RTN_EXTCTX_TYPE
   {
      RTN_EXTCTX_TYPE_INSERT = 0,
      RTN_EXTCTX_TYPE_DELETE,
      RTN_EXTCTX_TYPE_UPDATE,
      RTN_EXTCTX_TYPE_TRUNCATE,
      RTN_EXTCTX_TYPE_DROPCS,
      RTN_EXTCTX_TYPE_DROPCL,
      RTN_EXTCTX_TYPE_DROPIDX
   } ;
   typedef enum _RTN_EXTCTX_TYPE RTN_EXTCTX_TYPE ;

   class _rtnExtContextBase : public SDBObject
   {
   public:
      _rtnExtContextBase() ;
      virtual ~_rtnExtContextBase() ;
      void setID( UINT32 id )
      {
         _id = id ;
      }

      UINT32 getID() const
      {
         return _id ;
      }

      void appendProcessor( rtnExtDataProcessor *processor ) ;
      void appendProcessor( rtnExtDataProcessor *processor,
                            const BSONObj &processData ) ;
      void appendProcessors( const vector< rtnExtDataProcessor * >& processorVec ) ;

      RTN_EXTCTX_TYPE getType() const { return _type ; }

      virtual INT32 done( rtnExtDataProcessorMgr *edpMgr, _pmdEDUCB *cb,
                          SDB_DPSCB *dpscb = NULL ) = 0 ;
      virtual INT32 abort( rtnExtDataProcessorMgr *edpMgr, _pmdEDUCB *cb,
                           SDB_DPSCB *dpscb = NULL ) = 0 ;

   protected:
      typedef vector< rtnExtDataProcessor * > EDP_VEC ;
      typedef EDP_VEC::iterator EDP_VEC_ITR ;
      typedef vector< BSONObj > OBJ_VEC ;
      typedef OBJ_VEC::iterator OBJ_VEC_ITR ;

      RTN_EXTCTX_TYPE   _type ;
      UINT32            _id ;
      EDP_VEC           _processors ;
      OBJ_VEC           _objects ;
   } ;
   typedef _rtnExtContextBase rtnExtContextBase ;

   class _rtnExtDataOprCtx : public _rtnExtContextBase
   {
   public:
      _rtnExtDataOprCtx() ;
      ~_rtnExtDataOprCtx() ;

      virtual INT32 done( rtnExtDataProcessorMgr *edpMgr, _pmdEDUCB *cb,
                          SDB_DPSCB *dpscb = NULL ) ;
      virtual INT32 abort( rtnExtDataProcessorMgr *edpMgr, _pmdEDUCB *cb,
                           SDB_DPSCB *dpscb = NULL ) ;
   } ;
   typedef _rtnExtDataOprCtx rtnExtDataOprCtx ;

   class _rtnExtDropOprCtx : public _rtnExtContextBase
   {
   public:
      _rtnExtDropOprCtx() ;
      ~_rtnExtDropOprCtx() ;

      virtual INT32 done( rtnExtDataProcessorMgr *edpMgr, _pmdEDUCB *cb,
                          SDB_DPSCB *dpscb = NULL ) ;
      virtual INT32 abort( rtnExtDataProcessorMgr *edpMgr, _pmdEDUCB *cb,
                           SDB_DPSCB *dpscb = NULL ) ;
      void setRemoveFiles( BOOLEAN remove ) { _removeFiles = remove ; }
   private:
      BOOLEAN _removeFiles ;
   } ;
   typedef _rtnExtDropOprCtx rtnExtDropOprCtx ;

   class _rtnExtTruncateCtx : public _rtnExtContextBase
   {
   public:
      _rtnExtTruncateCtx() ;
      ~_rtnExtTruncateCtx() ;

      virtual INT32 done( rtnExtDataProcessorMgr *edpMgr, _pmdEDUCB *cb,
                          SDB_DPSCB *dpscb = NULL ) ;
      virtual INT32 abort( rtnExtDataProcessorMgr *edpMgr, _pmdEDUCB *cb,
                           SDB_DPSCB *dpscb = NULL ) ;

      void setCLLIDs( UINT32 oldLID, UINT32 newLID ) ;
   private:
      UINT32 _oldCLLID ;
      UINT32 _newCLLID ;
   } ;
   typedef _rtnExtTruncateCtx rtnExtTruncateCtx ;

   class _rtnExtContextMgr : public SDBObject
   {
      typedef utilConcurrentMap<UINT32, rtnExtContextBase*> RTN_CTX_MAP ;
   public:
      _rtnExtContextMgr() ;
      ~_rtnExtContextMgr() ;

      rtnExtContextBase* findContext( UINT32 contextID ) ;
      INT32 createContext( RTN_EXTCTX_TYPE type, _pmdEDUCB *cb,
                           rtnExtContextBase** context ) ;
      INT32 delContext( UINT32 contextID, _pmdEDUCB *cb ) ;

   private:
      ossRWMutex        _mutex ;
      RTN_CTX_MAP       _contextMap ;
   } ;
   typedef _rtnExtContextMgr rtnExtContextMgr ;

   class _rtnExtDataHandler : public _IDmsExtDataHandler
   {
   public:
      _rtnExtDataHandler( rtnExtDataProcessorMgr *edpMgr ) ;
      virtual ~_rtnExtDataHandler() ;

   public:
      virtual INT32 onOpenTextIdx( const CHAR *csName, const CHAR *clName,
                                   const CHAR *idxName ) ;

      virtual INT32 onDelCS( const CHAR *csName, pmdEDUCB *cb,
                             BOOLEAN removeFiles, SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onDropAllIndexes( const CHAR *csName, const CHAR *clName,
                                      _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onDropTextIdx( const CHAR *csName, const CHAR *clName,
                                   const CHAR *idxName, _pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onRebuildTextIdx( const CHAR *csName, const CHAR *clName,
                                      const CHAR *idxName, _pmdEDUCB *cb,
                                      SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onInsert( const CHAR *csName, const CHAR *clName,
                              const CHAR *idxName, const ixmIndexCB &indexCB,
                              const BSONObj &object, _pmdEDUCB* cb,
                              SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onDelete( const CHAR *csName, const CHAR *clName,
                              const CHAR *idxName,
                              const ixmIndexCB &indexCB,
                              const BSONObj &object, _pmdEDUCB* cb,
                              SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onUpdate( const CHAR *csName, const CHAR *clName,
                              const CHAR *idxName, const ixmIndexCB &indexCB,
                              const BSONObj &orignalObj, const BSONObj &newObj,
                              _pmdEDUCB* cb, SDB_DPSCB *dpscb = NULL ) ;

      INT32 onTruncateCL( const CHAR *csName, const CHAR *clName,
                           _pmdEDUCB *cb, SDB_DPSCB *dpsCB = NULL ) ;

      virtual INT32 done( _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 abortOperation( _pmdEDUCB *cb ) ;

   private:
      INT32 _prepareCSAndCL( const CHAR *csName, const CHAR *clName,
                             pmdEDUCB *cb, SDB_DPSCB *dpsCB ) ;

   private:
      rtnExtDataProcessorMgr  *_edpMgr ;
      rtnExtContextMgr        _contextMgr ;
   } ;
   typedef _rtnExtDataHandler rtnExtDataHandler ;

   rtnExtDataHandler* rtnGetExtDataHandler() ;
}

#endif /* RTN_EXTDATAHANDLER_HPP__ */

