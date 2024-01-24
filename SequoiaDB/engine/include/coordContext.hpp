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

   Source File Name = coordContext.hpp

   Descriptive Name = Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_CONTEXT_HPP__
#define COORD_CONTEXT_HPP__

#include "rtnContext.hpp"
#include "rtnContextMain.hpp"
#include "rtnContextExplain.hpp"
#include "coordDef.hpp"
#include "coordRemoteHandle.hpp"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{

   class _pmdRemoteSessionSite ;
   class _pmdRemoteSession ;

   /*
      coordSubContext define
   */
   class _coordSubContext : public _rtnSubContext
   {
   public:
      _coordSubContext ( const BSONObj& orderBy,
                         _ixmIndexKeyGen* keyGen,
                         INT64 contextID,
                         MsgRouteID routeID ) ;

      virtual ~_coordSubContext () ;

   public:
      void           appendData ( const pmdEDUEvent &event ) ;
      void           clearData () ;
      MsgRouteID     getRouteID() ;
      const CHAR*    front () ;
      INT32          pop() ;
      INT32          popN( INT32 num ) ;
      INT32          popAll() ;
      INT32          pushFront( const BSONObj &obj ) ;
      INT32          recordNum() ;
      INT32          remainLength() ;
      INT32          truncate ( INT32 num ) ;
      INT32          genOrderKey() ;

      OSS_INLINE INT64 getDataID () const
      {
         return (INT64)( _routeID.value ) ;
      }

   private:
      // disallow copy and assign
      _coordSubContext () ;
      _coordSubContext ( const _coordSubContext& ) ;
      void operator=( const _coordSubContext& ) ;

   private:
      MsgRouteID           _routeID ;
      INT32                _curOffset ;
      MsgOpReply*          _pData ;
      pmdEDUEvent          _event ;
      INT32                _recordNum ;
   } ;
   typedef _coordSubContext coordSubContext ;

   typedef ossPoolMap< UINT64, coordSubContext*>         EMPTY_CONTEXT_MAP ;
   typedef ossPoolMap< UINT64, MsgRouteID>               PREPARE_NODES_MAP ;

   /*
      _rtnContextCoord define
   */
   class _rtnContextCoord : public _rtnContextMain
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextCoord )
      public:
         _rtnContextCoord ( INT64 contextID, UINT64 eduID,
                            BOOLEAN preRead = TRUE ) ;
         virtual ~_rtnContextCoord () ;

         virtual INT32 addSubContext ( const pmdEDUEvent &event,
                                       BOOLEAN &takeOver ) ;

         virtual void addSubDone( _pmdEDUCB *cb ) ;

         virtual INT32 open ( const rtnQueryOptions & options,
                              BOOLEAN preRead = TRUE ) ;

         INT32    reopen () ;
         void     setModify( BOOLEAN modify ) ;

         /*
            Unit: millisec
         */
         INT64    getWaitTime() const ;

         virtual void optimizeReturnOptions ( MsgOpQuery * pQueryMsg,
                                              UINT32 targetGroupNum ) ;

         virtual void      getErrorInfo( INT32 rc,
                                         pmdEDUCB *cb,
                                         rtnContextBuf &buffObj ) ;

         virtual UINT32    getCachedRecordNum() ;

         virtual BOOLEAN   isWrite() const { return _isModify ; }
         virtual BOOLEAN   needRollback() const { return _isModify ; }
         virtual const CHAR *getProcessName() const
         {
            return ( NULL != _options.getCLFullName() ) ?
                   ( _options.getCLFullName() ) :
                   ( "" ) ;
         }

      public:
         virtual const CHAR*      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;

         OSS_INLINE BOOLEAN requireOrder () const ;

      protected:
         virtual void _toString( stringstream &ss ) ;

         void     _setPreRead ( BOOLEAN preRead ) { _preRead = preRead ; }
         BOOLEAN  _enabledPreRead () const { return _preRead ; }

      protected:
         OSS_INLINE BOOLEAN _requireExplicitSorting () const ;
         INT32   _createSubContext ( MsgRouteID routeID, SINT64 contextID ) ;
         INT32   _prepareAllSubCtxDataByOrder( _pmdEDUCB *cb ) ;
         INT32   _getNonEmptyNormalSubCtx( _pmdEDUCB *cb, rtnSubContext*& subCtx ) ;
         INT32   _saveEmptyOrderedSubCtx( rtnSubContext* subCtx ) ;
         INT32   _saveEmptyNormalSubCtx( rtnSubContext* subCtx ) ;
         INT32   _saveNonEmptyNormalSubCtx( rtnSubContext* subCtx ) ;
         INT32   _doAfterPrepareData( _pmdEDUCB *cb ) ;

         virtual INT32   _processCacheData( _pmdEDUCB *cb ) ;

         virtual INT32   _prepareSubCtxsAdvance( LST_SUB_CTX_PTR &lstCtx ) ;

         virtual INT32   _doSubCtxsAdvance( LST_SUB_CTX_PTR &lstCtx,
                                            const BSONObj &arg,
                                            _pmdEDUCB *cb ) ;
         virtual void    _preReleaseSubContext( rtnSubContext *subCtx ) ;

      private:
         INT32    _appendSubData ( const pmdEDUEvent &event,
                                   BOOLEAN &isTakeOver ) ;

         void     _delPrepareContext( const MsgRouteID &routeID,
                                      BOOLEAN setInvalidContext = FALSE ) ;

         INT32    _send2EmptyNodes( _pmdEDUCB *cb ) ;
         INT32    _getPrepareNodesData( _pmdEDUCB *cb, BOOLEAN waitAll ) ;

         INT32    _reOrderSubContext() ;
         INT32    _prepareSubCtxData( _pmdEDUCB *cb ) ;

         // release sub contexts with killing remote contexts
         void     _killSubContexts( _pmdEDUCB *cb ) ;
         // release sub contexts without killing remote contexts
         void     _destroySubContexts() ;

      private:
         EMPTY_CONTEXT_MAP          _emptyContextMap ;
         EMPTY_CONTEXT_MAP          _prepareContextMap ;

         BOOLEAN                    _preRead ;

         BOOLEAN                    _needReOrder ;
         /// error info
         ROUTE_RC_MAP               _nokRC ;

         _coordNoSessionInitHandler _handler ;
         _pmdRemoteSession          *_pSession ;

         BOOLEAN                    _isModify ;
   } ;
   typedef _rtnContextCoord rtnContextCoord ;

   /*
      _rtnContextCoord OSS_INLINE functions
   */
   OSS_INLINE BOOLEAN _rtnContextCoord::requireOrder () const
   {
      return !_options.isOrderByEmpty() ;
   }

   OSS_INLINE BOOLEAN _rtnContextCoord::_requireExplicitSorting () const
   {
      // 1. order is required ( sort is not empty )
      // 2. has more than one sub-context
      return requireOrder() &&
             ( _orderedContexts.size() + _emptyContextMap.size() +
               _prepareContextMap.size() > 1 ) ;
   }

   /*
      _rtnContextCoordExplain define
    */
   class _rtnContextCoordExplain : public _rtnContextCoord,
                                   public _rtnExplainMainBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextCoordExplain )

      public :
         _rtnContextCoordExplain ( INT64 contextID, UINT64 eduID,
                                   BOOLEAN preRead = TRUE ) ;

         virtual ~_rtnContextCoordExplain () ;

         const CHAR* name () const ;

         RTN_CONTEXT_TYPE getType () const ;

         INT32 open ( const rtnQueryOptions & options,
                      BOOLEAN preRead = TRUE ) ;

         void optimizeReturnOptions ( MsgOpQuery * pQueryMsg,
                                      UINT32 targetGroupNum ) ;

         INT32 addSubContext ( const pmdEDUEvent &event, BOOLEAN &takeOver ) ;

         void addSubDone ( pmdEDUCB *cb ) ;

         OSS_INLINE BOOLEAN _needParallelProcess () const
         {
            return TRUE ;
         }

      protected :
         INT32 _prepareData ( pmdEDUCB *cb ) ;

         INT32 _openSubContext ( rtnQueryOptions & options,
                                 pmdEDUCB * cb,
                                 rtnContextPtr *ppContext ) ;

         INT32 _prepareExplainPath ( rtnContext * context,
                                     pmdEDUCB * cb ) ;

         INT32 _parseLocationOption ( const BSONObj & explainOptions,
                                      BOOLEAN & hasOption ) ;

         BOOLEAN _needChildExplain ( INT64 dataID,
                                     const BSONObj & childExplain  ) ;

         INT32 _buildSimpleExplain ( rtnContext * explainContext,
                                     BOOLEAN & hasMore ) ;

         OSS_INLINE virtual UINT16 _getExplainInfoMask () const
         {
            return ( OPT_EXPINFO_MASK_RETURN_NUM |
                     OPT_EXPINFO_MASK_ELAPSED_TIME |
                     OPT_EXPINFO_MASK_USERCPU |
                     OPT_EXPINFO_MASK_SYSCPU ) ;
         }

         virtual optExplainMergePathBase* getExplainMergePath()
         {
            return &_explainCoordPath ;
         }

      protected :
         SET_ROUTEID             _locationFilter ;
         optExplainCoordPath     _explainCoordPath ;
   } ;

   typedef class _rtnContextCoordExplain rtnContextCoordExplain ;

}

#endif //COORD_CONTEXT_HPP__
