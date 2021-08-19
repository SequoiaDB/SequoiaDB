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

   Source File Name = rtnExtContext.hpp

   Descriptive Name = External data process context

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/08/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_EXTCONTEXT_HPP__
#define RTN_EXTCONTEXT_HPP__

#include "oss.hpp"
#include "dmsExtDataHandler.hpp"
#include "rtnExtOprData.hpp"
#include "rtnExtDataProcessor.hpp"
#include "utilConcurrentMap.hpp"

namespace engine
{
   // Context status. We need this because in one operation, we may invoke
   // different callback functions of different types. The typical scenario is
   // insertion. If data is inserted, and index insertion fails due to
   // duplicated key, deleteRecord will be called. Then callback fuction for
   // delete will be created. In that case, no record should be inserted into
   // capped collection. The total operation should be cancelled.
   enum _rtnExtContextStat
   {
      EXT_CTX_STAT_NORMAL = 0,
      EXT_CTX_STAT_ABORTING
   };
   typedef _rtnExtContextStat rtnExtContextStat ;

   // The life circle of this context is in each operation. It holds all the
   // text index processors of one collection.
   class _rtnExtContextBase : public utilPooledObject
   {
   public:
      _rtnExtContextBase( DMS_EXTOPR_TYPE type ) ;
      virtual ~_rtnExtContextBase() ;

      void setID( UINT32 id )
      {
         _id = id ;
      }

      UINT32 getID() const
      {
         return _id ;
      }

      DMS_EXTOPR_TYPE getType() const
      {
         return _type ;
      }

      void setStat( rtnExtContextStat stat )
      {
         _stat = stat ;
      }

      rtnExtContextStat getStat() const
      {
         return _stat ;
      }

      virtual BOOLEAN canCache() const
      {
         return FALSE ;
      }

      virtual void reset( DMS_EXTOPR_TYPE type ) ;

      virtual INT32 done( _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;
      virtual INT32 abort( _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;
   protected:
      // Append processors to the context. If locked, they will be released in
      // _cleanup.
      void _appendProcessor( rtnExtDataProcessor *processor ) ;
      void _appendProcessors( const vector< rtnExtDataProcessor * >& processorVec ) ;

      BOOLEAN _processorLocked() const
      {
         return ( SHARED == _lockType || EXCLUSIVE == _lockType ) ;
      }

      virtual INT32 _onDone( _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL )
      {
         return SDB_OK ;
      }

      virtual INT32 _onAbort( _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL )
      {
         return SDB_OK ;
      }

   private:
      virtual void _cleanup() ;

   protected:
      typedef vector< rtnExtDataProcessor * > EDP_VEC ;
      typedef vector< rtnExtDataProcessor * >::iterator EDP_VEC_ITR ;

      DMS_EXTOPR_TYPE         _type ;
      UINT32                  _id ;
      rtnExtContextStat       _stat ;
      rtnExtDataProcessorMgr  *_processorMgr ;
      EDP_VEC                 _processors ;
      INT32                   _lockType ;
   } ;
   typedef _rtnExtContextBase rtnExtContextBase ;

   class _rtnExtCrtIdxCtx : public _rtnExtContextBase
   {
   public:
      _rtnExtCrtIdxCtx() ;
      ~_rtnExtCrtIdxCtx() ;

      INT32 open( rtnExtDataProcessorMgr *processorMgr,
                  dmsMBContext *mbContext, const CHAR *csName,
                  ixmIndexCB &indexCB, pmdEDUCB *cb,
                  SDB_DPSCB *dpscb = NULL ) ;

   private:
      INT32 _onAbort( pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;

   private:
      BOOLEAN _rebuildDone ;
   } ;
   typedef _rtnExtCrtIdxCtx rtnExtCrtIdxCtx ;

   class _rtnExtRebuildIdxCtx : public _rtnExtContextBase
   {
   public:
      _rtnExtRebuildIdxCtx() ;
      virtual ~_rtnExtRebuildIdxCtx() ;

      INT32 open( rtnExtDataProcessorMgr *processorMgr,
                  const CHAR *csName, const CHAR *clName,
                  const CHAR *idxName, const CHAR *extName,
                  const BSONObj &indexDef, pmdEDUCB *cb,
                  SDB_DPSCB *dpscb = NULL ) ;
   } ;
   typedef _rtnExtRebuildIdxCtx rtnExtRebuildIdxCtx ;

   // Base class for insert/delete/update operations.
   class _rtnExtDataOprCtx : public _rtnExtContextBase,
                             public _rtnExtOprData
   {
   public:
      _rtnExtDataOprCtx( DMS_EXTOPR_TYPE type ) ;
      virtual ~_rtnExtDataOprCtx() ;

      BOOLEAN canCache() const
      {
         return TRUE ;
      }

      virtual INT32 open( rtnExtDataProcessorMgr *processorMgr,
                          const CHAR *extName, pmdEDUCB *cb,
                          SDB_DPSCB *dpscb = NULL ) ;

   private:
      virtual INT32 _onDone( _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;
   } ;
   typedef _rtnExtDataOprCtx rtnExtDataOprCtx ;

   class _rtnExtDropCSCtx : public _rtnExtContextBase
   {
   public:
      _rtnExtDropCSCtx()
      : _rtnExtContextBase( DMS_EXTOPR_TYPE_DROPCS ),
        _removeFiles( FALSE ),
        _eduCB( NULL ),
        _dpsCB( NULL )
      {
      }

      ~_rtnExtDropCSCtx() ;

      virtual INT32 open( rtnExtDataProcessorMgr *processorMgr,
                          const CHAR *csName, pmdEDUCB *cb,
                          BOOLEAN removeFiles, SDB_DPSCB *dpscb = NULL ) ;

   private:
      INT32 _onDone( pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;
      INT32 _onAbort( pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;

   private:
      BOOLEAN _removeFiles ;
      pmdEDUCB *_eduCB ;
      SDB_DPSCB *_dpsCB ;
      vector<rtnExtDataProcessor *> _processorP1 ;
   } ;
   typedef _rtnExtDropCSCtx rtnExtDropCSCtx ;

   class _rtnExtDropCLCtx : public _rtnExtContextBase
   {
   public:
      _rtnExtDropCLCtx()
      : _rtnExtContextBase( DMS_EXTOPR_TYPE_DROPCL ),
        _eduCB( NULL ),
        _dpsCB( NULL )
      {
      }
      ~_rtnExtDropCLCtx() ;

      virtual INT32 open( rtnExtDataProcessorMgr *processorMgr,
                          const CHAR *csName, const CHAR *clName,
                          pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;

   private:
      INT32 _onDone( pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;
      INT32 _onAbort( pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;
   private:
      pmdEDUCB *_eduCB ;
      SDB_DPSCB *_dpsCB ;
      vector<rtnExtDataProcessor *> _processorP1 ;
   } ;
   typedef _rtnExtDropCLCtx rtnExtDropCLCtx ;

   class _rtnExtDropIdxCtx : public _rtnExtContextBase
   {
   public:
      _rtnExtDropIdxCtx()
            : _rtnExtContextBase( DMS_EXTOPR_TYPE_DROPIDX )
      {
      }
      ~_rtnExtDropIdxCtx() {}

      virtual INT32 open( rtnExtDataProcessorMgr *processorMgr,
                          const CHAR *extName, pmdEDUCB *cb,
                          SDB_DPSCB *dpscb = NULL ) ;
   private:
      INT32 _onDone( pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;
      INT32 _onAbort( pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;
   } ;
   typedef _rtnExtDropIdxCtx rtnExtDropIdxCtx ;

   class _rtnExtTruncateCtx : public _rtnExtContextBase
   {
   public:
      _rtnExtTruncateCtx() ;
      ~_rtnExtTruncateCtx() ;

      virtual INT32 open( rtnExtDataProcessorMgr *processorMgr,
                          const CHAR *csName, const CHAR *clName,
                          _pmdEDUCB *cb, BOOLEAN needChangeCLID,
                          SDB_DPSCB *dpscb = NULL ) ;

   private:
      virtual INT32 _onDone( _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;
   } ;
   typedef _rtnExtTruncateCtx rtnExtTruncateCtx ;

   class _rtnExtContextMgr : public SDBObject
   {
      typedef utilConcurrentMap<UINT32, rtnExtContextBase*> RTN_CTX_MAP ;
   public:
      _rtnExtContextMgr() ;
      ~_rtnExtContextMgr() ;

      rtnExtContextBase* findContext( UINT32 contextID ) ;
      INT32 createContext( DMS_EXTOPR_TYPE type, _pmdEDUCB *cb,
                           rtnExtContextBase** context ) ;
      INT32 delContext( UINT32 contextID, _pmdEDUCB *cb ) ;

   private:
      RTN_CTX_MAP       _contextMap ;
   } ;
   typedef _rtnExtContextMgr rtnExtContextMgr ;
}

#endif /* RTN_EXTCONTEXT_HPP__ */
