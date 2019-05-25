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
#include "dmsCB.hpp"
#include "dpsLogWrapper.hpp"
#include "mthSelector.hpp"
#include "rtnContextBuff.hpp"
#include "rtnQueryOptions.hpp"
#include "utilMap.hpp"
#include <string>

using namespace bson ;

namespace engine
{
   #define RTN_CONTEXT_GETNUM_ONCE              (1000)

   /*
      RTN_CONTEXT_TYPE define
   */
   enum RTN_CONTEXT_TYPE
   {
      RTN_CONTEXT_DATA     = 1,
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
      RTN_CONTEXT_EXPLAIN,
      RTN_CONTEXT_LOB,
      RTN_CONTEXT_LOB_FETCHER,
      RTN_CONTEXT_SHARD_OF_LOB,
      RTN_CONTEXT_LIST_LOB,
      RTN_CONTEXT_OM_TRANSFER,
      RTN_CONTEXT_TS,            // Context of text search


      RTN_CONTEXT_CAT_BEGIN,

      RTN_CONTEXT_CAT_REMOVE_GROUP,
      RTN_CONTEXT_CAT_ACTIVE_GROUP,
      RTN_CONTEXT_CAT_SHUTDOWN_GROUP,
      RTN_CONTEXT_CAT_CREATE_NODE,
      RTN_CONTEXT_CAT_REMOVE_NODE,
      RTN_CONTEXT_CAT_DROP_CS,
      RTN_CONTEXT_CAT_CREATE_CL,
      RTN_CONTEXT_CAT_DROP_CL,
      RTN_CONTEXT_CAT_ALTER_CL,
      RTN_CONTEXT_CAT_LINK_CL,
      RTN_CONTEXT_CAT_UNLINK_CL,
      RTN_CONTEXT_CAT_CREATE_IDX,
      RTN_CONTEXT_CAT_DROP_IDX,

      RTN_CONTEXT_CAT_END,
   } ;

   const CHAR *getContextTypeDesp( RTN_CONTEXT_TYPE type ) ;

   class _pmdEDUCB ;
   class _dmsStorageUnit ;
   class _SDB_DMSCB ;
   class _dmsMBContext ;

   class _optAccessPlanRuntime ;
   typedef class _optAccessPlanRuntime optAccessPlanRuntime ;

   /*
      _rtnPrefWatcher define
   */
   class _rtnPrefWatcher : public SDBObject
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

   class _rtnContextStoreBuf: public SDBObject
   {
   public:
      _rtnContextStoreBuf() ;
      ~_rtnContextStoreBuf() ;

   public:
      INT32    append( const BSONObj &obj ) ;
      INT32    appendObjs( const CHAR *objBuf,
                              INT32 len,
                              INT32 num,
                              BOOLEAN needAligned = TRUE ) ;
      INT32    get( INT32 maxNumToReturn,
                     rtnContextBuf& buf ) ;
      void     release() ;

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
   } ;
   typedef _rtnContextStoreBuf rtnContextStoreBuf ;

   /*
      _rtnContextBase define
   */
   class _rtnContextBase : public SDBObject
   {
      friend class _rtnContextParaData ;
      friend class _rtnExplainBase ;

      public:
         _rtnContextBase ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextBase () ;
         string   toString() ;

         INT64    contextID () const { return _contextID ; }
         UINT64   eduID () const { return _eduID ; }

         ossRWMutex*       dataLock () { return &_dataLock ; }

         _mthSelector & getSelector ()
         {
            return _selector ;
         }

         const _mthSelector & getSelector () const
         {
            return _selector ;
         }

         INT32    append( const BSONObj &result ) ;
         INT32    appendObjs( const CHAR *pObjBuff,
                              INT32 len,
                              INT32 num,
                              BOOLEAN needAligned = TRUE ) ;

         virtual INT32    getMore( INT32 maxNumToReturn,
                                   rtnContextBuf &buffObj,
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

         void           setWriteInfo( SDB_DPSCB *dpsCB, INT16 w ) ;
         SDB_DPSCB*     getDPSCB() { return _pDpsCB ; }
         INT16          getW() const { return _w ; }

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
         virtual std::string      name() const = 0 ;
         virtual RTN_CONTEXT_TYPE getType () const = 0 ;
         virtual _dmsStorageUnit* getSU () = 0 ;
         virtual BOOLEAN          isWrite() const { return FALSE ; }

         virtual optAccessPlanRuntime * getPlanRuntime ()
         {
            return NULL ;
         }

         virtual const optAccessPlanRuntime * getPlanRuntime () const
         {
            return NULL ;
         }

      public :
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
         }

      protected:
         void              _onDataEmpty () ;
         virtual INT32     _prepareData( _pmdEDUCB *cb ) = 0 ;
         virtual BOOLEAN   _canPrefetch () const { return FALSE ; }
         virtual void      _toString( stringstream &ss ) {}
         BOOLEAN           _canPrepareMoreData() const { return _canPrepareMore ;}
         INT32             _prepareMoreData( _pmdEDUCB *cb ) ;
         INT32             _prepareDataMonitor ( _pmdEDUCB *cb ) ;
         INT32             _getBuffer( INT32 maxNumToReturn,
                                       rtnContextBuf& buf ) ;

      protected:
         OSS_INLINE void _empty () ;
         OSS_INLINE void _close () { _isOpened = FALSE ; }
         UINT32   _getWaitPrefetchNum () { return _waitPrefetchNum.peek() ; }
         BOOLEAN  _isInPrefetching () const { return _isInPrefetch ; }

         void     _resetTotalRecords( INT64 totalRecords )
         {
            _totalRecords = totalRecords ;
         }

      protected:
         monContextCB            _monCtxCB ;
         _mthSelector            _selector ;

         BOOLEAN                 _hitEnd ;
         BOOLEAN                 _isOpened ;

         SDB_DPSCB               *_pDpsCB ;
         INT16                   _w ;

         BOOLEAN                 _enableMonContext ;
         BOOLEAN                 _enableQueryActivity ;

      private:
         INT64                   _contextID ;
         UINT64                  _eduID ;
         _rtnContextStoreBuf     _buffer ;
         INT64                   _totalRecords ;
         ossRWMutex              _dataLock ;
         ossRWMutex              _prefetchLock ;
         UINT32                  _prefetchID ;
         ossAtomic32             _waitPrefetchNum ;
         BOOLEAN                 _isInPrefetch ;
         INT32                   _prefetchRet ;
         rtnPrefWatcher          *_pPrefWatcher ;
         _monAppCB               *_pMonAppCB ;

         BOOLEAN                 _countOnly ;

         BOOLEAN                 _canPrepareMore ;
         INT32                   _prepareMoreDataLimit ;
   } ;
   typedef _rtnContextBase rtnContextBase ;
   typedef _rtnContextBase rtnContext ;

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

   typedef _rtnContextBase* (*RTN_CTX_NEW_FUNC)( INT64 contextId, EDUID eduId ) ;

   class _rtnContextAssit: public SDBObject
   {
   public:
      _rtnContextAssit( RTN_CONTEXT_TYPE type,
                             std::string name,
                             RTN_CTX_NEW_FUNC func ) ;
      ~_rtnContextAssit() ;
   } ;

#define DECLARE_RTN_CTX_AUTO_REGISTER() \
   public: \
      static _rtnContextBase *newThis ( INT64 contextId, EDUID eduId ) ;

#define RTN_CTX_AUTO_REGISTER(theClass, type, name ) \
   _rtnContextBase *theClass::newThis ( INT64 contextId, EDUID eduId ) \
   { \
      return SDB_OSS_NEW theClass( contextId, eduId ) ;\
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

      _rtnContextBase* create ( RTN_CONTEXT_TYPE type, INT64 contextId, EDUID eduId ) ;
      void             release ( _rtnContextBase* context ) ;
      const _rtnContextInfo* find( RTN_CONTEXT_TYPE type ) const ;

   private:
      INT32 _register( RTN_CONTEXT_TYPE type,
                        std::string name,
                        RTN_CTX_NEW_FUNC func ) ;
      INT32 _insert( _rtnContextInfo* contextInfo ) ;
      void _releaseContextInfos() ;

   private:
      std::map<RTN_CONTEXT_TYPE, _rtnContextInfo*> _contextInfoMap ;
      typedef std::pair<RTN_CONTEXT_TYPE, _rtnContextInfo*> pair_type ;
      typedef std::map<RTN_CONTEXT_TYPE, _rtnContextInfo*>::const_iterator ctx_info_iterator ;
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
         void _deleteSubContext () ;

         void _setSubContext ( rtnContext * subContext, pmdEDUCB * subCB ) ;

         OSS_INLINE rtnContext * _getSubContext ()
         {
            return _subContext ;
         }

         OSS_INLINE const rtnContext * _getSubContext () const
         {
            return _subContext ;
         }

         OSS_INLINE pmdEDUCB * _getSubContextCB ()
         {
            return _subCB ;
         }

      protected :
         pmdEDUCB *     _subCB ;
         rtnContext *   _subContext ;
         INT64          _subContextID ;
   } ;

   typedef class _rtnSubContextHolder rtnSubContextHolder ;

}

#endif //RTNCONTEXT_HPP_
