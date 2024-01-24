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

   Source File Name = rtnContext.hpp

   Descriptive Name = RunTime Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNCONTEXT_HPP_
#define RTNCONTEXT_HPP_

#include "ossMem.hpp"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "monCB.hpp"
#include "ossAtomic.hpp"
#include "ossMemPool.hpp"
#include "dpsLogWrapper.hpp"
#include "mthSelector.hpp"
#include "rtnContextDef.hpp"
#include "rtnContextBuff.hpp"
#include "rtnQueryOptions.hpp"
#include "rtnResultSetFilter.hpp"
#include "utilPooledObject.hpp"
#include "utilPooledAutoPtr.hpp"
#include "monClass.hpp"
#include <string>

using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;
   class _dmsStorageUnit ;
   class _dmsMBContext ;
   class _optAccessPlanRuntime ;

   /*
      RTN_CONTEXT_TYPE define
   */
   enum RTN_CONTEXT_TYPE
   {
      RTN_CONTEXT_DATA     = 0,
      RTN_CONTEXT_DUMP,
      RTN_CONTEXT_COORD,
      RTN_CONTEXT_COORD_EXP,
      RTN_CONTEXT_QGM,
      RTN_CONTEXT_TEMP,
      RTN_CONTEXT_SP,
      RTN_CONTEXT_PARADATA,
      RTN_CONTEXT_MAINCL,
      RTN_CONTEXT_MAINCL_EXP,
      RTN_CONTEXT_SORT,
      RTN_CONTEXT_QGMSORT,
      RTN_CONTEXT_DELCS,
      RTN_CONTEXT_DELCL,
      RTN_CONTEXT_DELMAINCL,
      RTN_CONTEXT_RENAMECS,
      RTN_CONTEXT_RENAMECL,
      RTN_CONTEXT_RENAMEMAINCL,
      RTN_CONTEXT_EXPLAIN,
      RTN_CONTEXT_LOB,
      RTN_CONTEXT_LOB_FETCHER,
      RTN_CONTEXT_SHARD_OF_LOB,
      RTN_CONTEXT_LIST_LOB,
      RTN_CONTEXT_OM_TRANSFER,
      RTN_CONTEXT_TS,            // Context of text search
      RTN_CONTEXT_TRUNCATECL,
      RTN_CONTEXT_TRUNCATEMAINCL,
      RTN_CONTEXT_RETURNCL,
      RTN_CONTEXT_RETURNCS,
      RTN_CONTEXT_RETURNMAINCL,

      /// Alter contexts
      RTN_CONTEXT_ALTERCS,
      RTN_CONTEXT_ALTERCL,
      RTN_CONTEXT_ALTERMAINCL,

      /// Catalog contexts

      RTN_CONTEXT_CAT_BEGIN,

      /// Group related
      RTN_CONTEXT_CAT_REMOVE_GROUP,
      RTN_CONTEXT_CAT_ACTIVE_GROUP,
      RTN_CONTEXT_CAT_SHUTDOWN_GROUP,
      RTN_CONTEXT_CAT_ALTER_GROUP,
      /// Node related
      RTN_CONTEXT_CAT_CREATE_NODE,
      RTN_CONTEXT_CAT_REMOVE_NODE,
      /// CollectionSpace related
      RTN_CONTEXT_CAT_DROP_CS,
      RTN_CONTEXT_CAT_ALTER_CS,
      RTN_CONTEXT_CAT_RENAME_CS,
      /// Collection related
      RTN_CONTEXT_CAT_CREATE_CL,
      RTN_CONTEXT_CAT_DROP_CL,
      RTN_CONTEXT_CAT_ALTER_CL,
      RTN_CONTEXT_CAT_LINK_CL,
      RTN_CONTEXT_CAT_UNLINK_CL,
      RTN_CONTEXT_CAT_RENAME_CL,
      RTN_CONTEXT_CAT_TRUNCATE_CL,
      /// Index related
      RTN_CONTEXT_CAT_CREATE_IDX,
      RTN_CONTEXT_CAT_DROP_IDX,
      /// recycle bin related
      RTN_CONTEXT_CAT_RETURN_RECYCLEBIN,

      /// The last
      RTN_CONTEXT_CAT_END
   } ;

   const CHAR *getContextTypeDesp( RTN_CONTEXT_TYPE type ) ;

   /*
      _rtnPrefWatcher define
   */
   class _rtnPrefWatcher : public _utilPooledObject
   {
      public:
         _rtnPrefWatcher () :_prefNum(0), _needWait(FALSE) {}
         ~_rtnPrefWatcher () {}
         void     reset ()
         {
            _needWait = _prefNum > 0 ? TRUE : FALSE ;
            _prefEvent.reset() ;
         }
         void     ntyBegin ()
         {
            ++_prefNum ;
            _needWait = TRUE ;
         }
         void     ntyEnd ()
         {
            --_prefNum ;
            _prefEvent.signalAll() ;
         }
         INT32    waitDone( INT64 millisec = -1 )
         {
            if ( !_needWait && _prefNum <= 0 )
            {
               return 0 ;
            }
            INT32 rc = _prefEvent.wait( millisec, NULL ) ;
            if ( SDB_OK == rc )
            {
               return 1 ;
            }
            return rc ;
         }

      private:
         UINT32         _prefNum ;
         BOOLEAN        _needWait ;
         ossEvent       _prefEvent ;
   } ;
   typedef _rtnPrefWatcher rtnPrefWatcher ;

   class _rtnContextValidator: public _utilPooledObject
   {
   public:
      virtual  INT32 validate( const BSONObj &record ) = 0 ;
      virtual ~_rtnContextValidator() { }
   } ;

   class _rtnContextStoreBuf: public _utilPooledObject
   {
   public:
      _rtnContextStoreBuf() ;
      ~_rtnContextStoreBuf() ;

   public:
      INT32    append( const BSONObj &obj,
                       const BSONObj *orgObj = NULL ) ;
      INT32    pushFront( const BSONObj &obj ) ;
      INT32    pushFronts( const CHAR *objBuf,
                           INT32 len,
                           INT32 num ) ;
      INT32    appendObjs( const CHAR *objBuf,
                           INT32 len,
                           INT32 num,
                           BOOLEAN needAligned = TRUE ) ;
      INT32    get( INT32 maxNumToReturn,
                    rtnContextBuf& buf,
                    BOOLEAN onlyPeek = FALSE ) ;

      // only for object(aligned)
      INT32    pop( UINT32 num = 1 ) ;

      void     release() ;

      void     setContextValidator( _rtnContextValidator *contextValidator ) ;

   public:
      OSS_INLINE void      enableCountMode() { _countOnly = TRUE ; }
      OSS_INLINE BOOLEAN   isCountMode() const { return _countOnly ; }
      OSS_INLINE INT32     bufferSize() const { return _bufferSize ; }
      OSS_INLINE INT64     numRecords() const { return _numRecords ; }
      OSS_INLINE INT32     readOffset() const { return _readOffset ; }
      OSS_INLINE INT32     writeOffset() const { return _writeOffset ; }
      OSS_INLINE INT32     freeSize() const
      {
         return _bufferSize - ossAlign4((UINT32)_writeOffset) ;
      }
      OSS_INLINE BOOLEAN   hasMem() const
      {
         return (NULL != _buffer) ? TRUE : FALSE ;
      }
      OSS_INLINE INT32*    getContextFlag()
      {
         return hasMem() ? RTN_GET_CONTEXT_FLAG(_buffer) : NULL ;
      }
      OSS_INLINE INT32*    getRefCountPointer()
      {
         return hasMem() ? RTN_GET_REFERENCE(_buffer) : NULL ;
      }
      OSS_INLINE INT32     getRefCount() const
      {
         return hasMem() ? *RTN_GET_REFERENCE(_buffer) : 0 ;
      }
      OSS_INLINE BOOLEAN   isEmpty() const
      {
         return ( _numRecords == 0 ) ? TRUE : FALSE ;
      }
      OSS_INLINE void      empty()
      {
         _numRecords = 0 ;
         _readOffset = 0 ;
         _writeOffset = 0 ;
      }

   private:
      INT32    _ensureBufferSize( INT32 ensuredSize ) ;

   private:
      CHAR*    _buffer ;
      INT64    _numRecords ;
      INT32    _bufferSize ;
      INT32    _readOffset ;
      INT32    _writeOffset ;
      BOOLEAN  _countOnly ;
      _rtnContextValidator *_contextValidator ;
   } ;
   typedef _rtnContextStoreBuf rtnContextStoreBuf ;

   /*
      _rtnContextBase define
   */
   class _rtnContextBase : public _rtnContextValidator
   {
      friend class _rtnContextParaData ;
      friend class _rtnExplainBase ;
      friend class _SDB_RTNCB ;

      typedef boost::shared_ptr<ossRWMutex>     ctxMutexPtr ;

      public:
         _rtnContextBase ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextBase () ;
         string   toString() ;

         INT64    contextID () const { return _contextID ; }
         UINT64   eduID () const { return _eduID ; }

         void     setOpID( UINT64 opID )
         {
            _opID = opID ;
         }

         UINT64   getOpID() const
         {
            return _opID ;
         }

         ossRWMutex*       dataLock () { return &_dataLock ; }

         _mthSelector & getSelector ()
         {
            return _selector ;
         }

         const _mthSelector & getSelector () const
         {
            return _selector ;
         }

         INT32    append( const BSONObj &result,
                          const BSONObj *orgResult = NULL ) ;
         INT32    appendObjs( const CHAR *pObjBuff,
                              INT32 len,
                              INT32 num,
                              BOOLEAN needAligned = TRUE ) ;

         INT32    getMore( INT32 maxNumToReturn,
                           rtnContextBuf &buffObj,
                           _pmdEDUCB *cb ) ;

         INT32    advance( const BSONObj &arg,
                           const CHAR *pBackData ,
                           INT32 backDataSize,
                           _pmdEDUCB *cb ) ;

         INT32    locate( const BSONObj &arg,
                          _pmdEDUCB *cb ) ;

         virtual void     getErrorInfo( INT32 rc,
                                        _pmdEDUCB *cb,
                                        rtnContextBuf &buffObj )
         {}

         virtual UINT32   getCachedRecordNum() ;

         OSS_INLINE BOOLEAN  isEmpty () const ;

         INT64    numRecords () const { return _buffer.numRecords() ; }
         INT32    buffSize () const { return _buffer.bufferSize() ; }
         INT64    totalRecords () const { return _totalRecords ; }
         OSS_INLINE INT32 freeSize () const ;
         INT32    buffEndOffset () const { return _buffer.writeOffset() ; }

         BOOLEAN  isOpened () const { return _isOpened ; }
         BOOLEAN  eof () const { return _hitEnd ; }

         INT32    getReference() const ;

         void     enableCountMode()
         {
            _buffer.enableCountMode() ;
            _countOnly = TRUE ;
         }
         BOOLEAN  isCountMode() const { return _countOnly ; }

         /// write info( some context will write data, drop collection, etc..)
         void           setWriteInfo( SDB_DPSCB *dpsCB, INT16 w ) ;
         SDB_DPSCB*     getDPSCB() { return _pDpsCB ; }
         INT16          getW() const { return _w ; }

         void           setTransContext( BOOLEAN transCtx ) ;
         BOOLEAN        isTransContext() const ;

         // if affect global index or not when queryAndModify
         void           setIsAffectGIndex( BOOLEAN isAffect )
         {
            _isAffectGIndex = isAffect ;
         }

         BOOLEAN        isAffectGIndex() const { return _isAffectGIndex ; }

         const MsgGlobalID& getGlobalID() const { return _globalID ; }

      private:
         void _setGlobalID( const MsgGlobalID &globalID )
         {
            _globalID = globalID ;
         }

      // prefetch
      public:
         void     enablePrefetch ( _pmdEDUCB *cb,
                                   rtnPrefWatcher *pWatcher = NULL ) ;
         void     disablePrefetch ()
         {
            _prefetchID = 0 ;
            _pPrefWatcher = NULL ;
            _pMonAppCB = NULL ;
         }
         INT32    prefetchResult() const { return _prefetchRet ; }
         INT32    prefetch ( _pmdEDUCB *cb, UINT32 prefetchID ) ;
         void     waitForPrefetch() ;
         void     setPrepareMoreData( BOOLEAN canPrepare )
         {
            _canPrepareMore = canPrepare ;
         }

      public:
         virtual const CHAR*      name() const = 0 ;
         virtual RTN_CONTEXT_TYPE getType () const = 0 ;
         virtual _dmsStorageUnit* getSU () = 0 ;
         virtual BOOLEAN          isWrite() const { return FALSE ; }
         virtual BOOLEAN          needRollback() const { return FALSE ; }

         virtual UINT32 getSULogicalID() const
         {
            return DMS_INVALID_LOGICCSID ;
         }

         // name of processing object ( collection space or collection )
         virtual const CHAR *getProcessName() const
         {
            return "" ;
         }

         BOOLEAN needTimeout() const
         {
            return _needTimeout ;
         }

         void disableTimeout()
         {
            _needTimeout = FALSE ;
         }

         UINT64 getLastProcessTick() const
         {
            return _lastProcessTick ;
         }

         void updateLastProcessTick() ;

         virtual _optAccessPlanRuntime * getPlanRuntime ()
         {
            return NULL ;
         }

         virtual const _optAccessPlanRuntime * getPlanRuntime () const
         {
            return NULL ;
         }

         BOOLEAN needCloseOnEOF() const
         {
            return _needCloseOnEOF ;
         }

         void enableCloseOnEOF()
         {
            _needCloseOnEOF = TRUE ;
         }

      // Monitor
      public :
         void setMonQueryCB(monClassQuery *cb)
         {
            _monQueryCB = cb ;
         }

         monClassQuery* getMonQueryCB()
         {
            return _monQueryCB ;
         }

         monContextCB* getMonCB ()
         {
            return &_monCtxCB ;
         }

         const monContextCB* getMonCB () const
         {
            return &_monCtxCB ;
         }

         BOOLEAN enabledMonContext () const
         {
            return _enableMonContext ;
         }

         void setEnableMonContext ( BOOLEAN enableMonContext )
         {
            _enableMonContext = enableMonContext ;
         }

         BOOLEAN enabledQueryActivity () const
         {
            return _enableQueryActivity ;
         }

         void setEnableQueryActivity ( BOOLEAN enableQueryActivity )
         {
            _enableQueryActivity = enableQueryActivity ;
         }

         virtual void setQueryActivity ( BOOLEAN hitEnd )
         {
            // Do nothing
         }

         virtual void setResultSetFilter( rtnResultSetFilter *rsFilter,
                                          BOOLEAN appendMode = TRUE )
         {
         }

         virtual INT32 validate ( const BSONObj &record )
         {
            // Do nothing
            return 0;
         }

      protected:
         virtual INT32     _prepareData( _pmdEDUCB *cb ) = 0 ;
         virtual BOOLEAN   _canPrefetch () const { return FALSE ; }
         virtual void      _toString( stringstream &ss ) {}
         virtual INT32     _doAdvance( INT32 type,
                                       INT32 prefixNum,
                                       const BSONObj &keyVal,
                                       const BSONObj &orderby,
                                       const BSONObj &arg,
                                       BOOLEAN isLocate,
                                       _pmdEDUCB *cb )
         {
            return SDB_OPTION_NOT_SUPPORT ;
         }
         virtual INT32     _getAdvanceOrderby( BSONObj &orderby,
                                               BOOLEAN isRange = FALSE ) const
         {
            return SDB_OPTION_NOT_SUPPORT ;
         }

         virtual void      _onDataEmpty () ;
         BOOLEAN           _canPrepareMoreData() const { return _canPrepareMore ;}
         INT32             _prepareMoreData( _pmdEDUCB *cb ) ;
         INT32             _prepareDataMonitor ( _pmdEDUCB *cb ) ;
         INT32             _getBuffer( INT32 maxNumToReturn,
                                       rtnContextBuf& buf ) ;

         virtual INT32 _prepareDoAdvance ( _pmdEDUCB *cb )
         {
            return SDB_OPTION_NOT_SUPPORT;
         }

      protected:
         OSS_INLINE void _empty () ;
         OSS_INLINE void _close () { _isOpened = FALSE ; }
         UINT32   _getWaitPrefetchNum () { return _waitPrefetchNum.peek() ; }
         BOOLEAN  _isInPrefetching () const { return _isInPrefetch ; }

         void     _resetTotalRecords( INT64 totalRecords )
         {
            _totalRecords = totalRecords ;
         }

         INT32    _advance( const BSONObj &arg,
                            BOOLEAN isLocate,
                            _pmdEDUCB *cb,
                            const CHAR *pBackData = NULL,
                            INT32 backDataSize = 0 ) ;

         INT32    _advanceRecords( INT32 type,
                                   INT32 prefixNum,
                                   ixmIndexKeyGen &keyGen,
                                   const BSONObj &keyVal,
                                   const BSONObj &orderby,
                                   INT64 recordNum,
                                   _pmdEDUCB *cb,
                                   BOOLEAN &finished ) ;

         INT32    _advanceBackData( INT32 type,
                                    INT32 prefixNum,
                                    ixmIndexKeyGen &keyGen,
                                    const BSONObj &keyVal,
                                    const BSONObj &orderby,
                                    const CHAR *pBackData,
                                    INT32 backDataSize,
                                    BOOLEAN &finished ) ;

         INT32    _checkAdvance( INT32 type,
                                 ixmIndexKeyGen &keyGen,
                                 INT32 prefixNum,
                                 const BSONObj &keyVal,
                                 const BSONObj &curObj,
                                 const BSONObj &orderby,
                                 BOOLEAN &matched,
                                 BOOLEAN &isEqual ) ;

         INT32    _woNCompare( const BSONObj &l,
                               const BSONObj &r,
                               BOOLEAN compreFieldName,
                               UINT32 keyNum,
                               const BSONObj &keyPattern = BSONObj() ) ;

      protected:
         monContextCB            _monCtxCB ;
         monClassQuery          *_monQueryCB ;
         _mthSelector            _selector ;

         // status
         BOOLEAN                 _hitEnd ;
         BOOLEAN                 _isOpened ;

         SDB_DPSCB               *_pDpsCB ;
         INT16                   _w ;

         // Enable performance monitor
         BOOLEAN                 _enableMonContext ;
         BOOLEAN                 _enableQueryActivity ;

         // advance postion info
         BSONObj                 _advancePosition ;

      private:
         INT64                   _contextID ;
         UINT64                  _eduID ;
         UINT64                  _opID ;
         _rtnContextStoreBuf     _buffer ;
         INT64                   _totalRecords ;
         BOOLEAN                 _preHitEnd ;
         // mutex
         ossRWMutex              _dataLock ;
         ctxMutexPtr             _prefetchLock ;
         UINT32                  _prefetchID ;
         ossAtomic32             _waitPrefetchNum ;
         BOOLEAN                 _isInPrefetch ;
         INT32                   _prefetchRet ;
         rtnPrefWatcher          *_pPrefWatcher ;
         _monAppCB               *_pMonAppCB ;

         BOOLEAN                 _countOnly ;

         BOOLEAN                 _canPrepareMore ;
         INT32                   _prepareMoreDataLimit ;
         INT32                   _prepareMoreTimeLimit ;

         BOOLEAN                 _isTransCtx ;

         BOOLEAN                 _isAffectGIndex ;

         // last tick after open, get-more or advance
         UINT64                  _lastProcessTick ;
         // indicates whether to check timeout
         BOOLEAN                 _needTimeout ;
         // indicates whether to close when EOF
         BOOLEAN                 _needCloseOnEOF ;

         MsgGlobalID             _globalID ;
   } ;
   typedef _rtnContextBase rtnContextBase ;
   typedef _rtnContextBase rtnContext ;
   typedef utilThreadLocalPtr< rtnContext > rtnContextPtr ;

   typedef ossPoolSet< INT64 > RTN_CTX_ID_SET ;

   typedef struct _rtnCtxProcessInfo
   {
      _rtnCtxProcessInfo()
      {
         _opID = 0 ;
         _ctxID = -1 ;
         _eduID = PMD_INVALID_EDUID ;
      }

      UINT64         _opID ;
      INT64          _ctxID ;
      EDUID          _eduID ;
      ossPoolString  _processName ;
   } rtnCtxProcessInfo ;
   typedef ossPoolVector< rtnCtxProcessInfo > RTN_CTX_PROCESS_LIST ;

   /*
      _rtnContextBase OSS_INLINE functions
   */
   OSS_INLINE BOOLEAN _rtnContextBase::isEmpty () const
   {
      return _buffer.isEmpty() ;
   }
   OSS_INLINE void _rtnContextBase::_empty ()
   {
      _totalRecords = _totalRecords - _buffer.numRecords() ;
      _buffer.empty() ;
   }
   OSS_INLINE INT32 _rtnContextBase::freeSize () const
   {
      return _buffer.freeSize() ;
   }

   typedef rtnContextPtr (*RTN_CTX_NEW_FUNC)( INT64 contextId, EDUID eduId ) ;

   class _rtnContextAssit: public SDBObject
   {
   public:
      _rtnContextAssit( RTN_CONTEXT_TYPE type,
                        std::string name,
                        RTN_CTX_NEW_FUNC func ) ;
      ~_rtnContextAssit() ;
   } ;

#define DECLARE_RTN_CTX_AUTO_REGISTER(theClass) \
   public: \
      static rtnContextPtr newThis ( INT64 contextId, EDUID eduId ) ; \
      class sharePtr : public rtnContextPtr \
      { \
      public: \
         theClass* get() const { return (theClass *)( rtnContextPtr::get() ) ; } \
         theClass* operator->() { return get() ; } \
         const theClass* operator->() const { return get() ; } \
         operator const theClass* () { return get() ; } \
         operator theClass* () { return get() ; } \
      } ;

#define RTN_CTX_AUTO_REGISTER(theClass, type, name ) \
   rtnContextPtr theClass::newThis ( INT64 contextId, EDUID eduId ) \
   { \
      rtnContextPtr res ; \
      utilThreadLocalPtr< theClass > ptr = \
                  utilThreadLocalPtr< theClass >::allocRaw( ALLOC_TC ) ; \
      if ( NULL != ptr.get() && \
           NULL != new ( ptr.get() ) theClass( contextId, eduId ) ) \
      { \
         res = ptr ; \
         SDB_ASSERT( NULL != res.get(), "should be valid cast" ) ; \
      } \
      return res ; \
   } \
   _rtnContextAssit theClass##Assit ( type, std::string( name ), theClass::newThis ) ;

   struct _rtnContextInfo: public SDBObject
   {
      RTN_CONTEXT_TYPE  type ;
      std::string       name ;
      RTN_CTX_NEW_FUNC  newFunc ;
   } ;

   class _rtnContextBuilder: public SDBObject
   {
      friend class _rtnContextAssit ;

   public:
      _rtnContextBuilder() ;
      ~_rtnContextBuilder() ;

      rtnContextPtr  create( RTN_CONTEXT_TYPE type,
                             INT64 contextId,
                             EDUID eduId ) ;
      const _rtnContextInfo* find( RTN_CONTEXT_TYPE type ) const ;

   private:
      INT32 _register( RTN_CONTEXT_TYPE type,
                        std::string name,
                        RTN_CTX_NEW_FUNC func ) ;
      INT32 _insert( _rtnContextInfo* contextInfo ) ;
      void _releaseContextInfos() ;

   private:
      typedef ossPoolVector<_rtnContextInfo*> CONTEXT_INFO_VECTOR;

      CONTEXT_INFO_VECTOR _contextInfoVector ;
      typedef CONTEXT_INFO_VECTOR::const_iterator ctx_info_iterator ;
   } ;

   _rtnContextBuilder* sdbGetRTNContextBuilder() ;

   /*
      _rtnSubContextHolder define
    */
   class _rtnSubContextHolder
   {
      public :
         _rtnSubContextHolder () ;

         ~_rtnSubContextHolder () ;

      protected :
         void _setSubContext ( rtnContextPtr &subContext, _pmdEDUCB *subCB ) ;

         OSS_INLINE rtnContext * _getSubContext ()
         {
            return _subContext.get() ;
         }

         OSS_INLINE const rtnContext * _getSubContext () const
         {
            return _subContext.get() ;
         }

      private :
         void _deleteSubContext () ;

      protected :
         UINT32         _subSULogicalID ;
         _pmdEDUCB *    _subCB ;
         rtnContextPtr  _subContext ;
   } ;

   typedef class _rtnSubContextHolder rtnSubContextHolder ;

}

#endif //RTNCONTEXT_HPP_
