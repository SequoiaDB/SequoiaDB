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
      void           appendData ( MsgOpReply *pReply ) ;
      void           clearData () ;
      MsgRouteID     getRouteID() ;
      const CHAR*    front () ;
      INT32          pop() ;
      INT32          popN( INT32 num ) ;
      INT32          popAll() ;
      INT32          recordNum() ;
      INT32          remainLength() ;
      INT32          truncate ( INT32 num ) ;
      INT32          getOrderKey( rtnOrderKey &orderKey ) ;

      OSS_INLINE INT64 getDataID () const
      {
         return (INT64)( _routeID.value ) ;
      }

   private:
      _coordSubContext () ;
      _coordSubContext ( const _coordSubContext& ) ;
      void operator=( const _coordSubContext& ) ;

   private:
      MsgRouteID           _routeID ;
      INT32                _curOffset ;
      MsgOpReply*          _pData ;
      INT32                _recordNum ;
   } ;
   typedef _coordSubContext coordSubContext ;

   typedef _utilMap< UINT64, coordSubContext*, 20 >         EMPTY_CONTEXT_MAP ;
   typedef _utilMap< UINT64, MsgRouteID, 20 >               PREPARE_NODES_MAP ;

   /*
      _rtnContextCoord define
   */
   class _rtnContextCoord : public _rtnContextMain
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
      public:
         _rtnContextCoord ( INT64 contextID, UINT64 eduID,
                            BOOLEAN preRead = TRUE ) ;
         virtual ~_rtnContextCoord () ;

         virtual INT32 addSubContext ( MsgOpReply *pReply, BOOLEAN &takeOver ) ;

         virtual void addSubDone( _pmdEDUCB *cb ) ;

         virtual INT32 open ( const rtnQueryOptions & options,
                              BOOLEAN preRead = TRUE ) ;

         INT32    reopen () ;

         void     killSubContexts( _pmdEDUCB *cb ) ;

         /*
            Unit: millisec
         */
         INT64    getWaitTime() const ;

         virtual void optimizeReturnOptions ( MsgOpQuery * pQueryMsg,
                                              UINT32 targetGroupNum ) ;

         virtual void     getErrorInfo( INT32 rc,
                                        pmdEDUCB *cb,
                                        rtnContextBuf &buffObj ) ;

         virtual UINT32   getCachedRecordNum() ;

      public:
         virtual std::string      name() const ;
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

      private:
         INT32    _appendSubData ( CHAR *pData ) ;

         void     _delPrepareContext( const MsgRouteID &routeID ) ;

         INT32    _send2EmptyNodes( _pmdEDUCB *cb ) ;
         INT32    _getPrepareNodesData( _pmdEDUCB *cb, BOOLEAN waitAll ) ;

         INT32    _reOrderSubContext() ;
         INT32    _prepareSubCtxData( _pmdEDUCB *cb ) ;

      private:
         EMPTY_CONTEXT_MAP          _emptyContextMap ;
         EMPTY_CONTEXT_MAP          _prepareContextMap ;

         rtnOrderKey                _emptyKey ;
         BOOLEAN                    _preRead ;

         BOOLEAN                    _needReOrder ;
         ROUTE_RC_MAP               _nokRC ;

         _pmdRemoteSessionSite      *_pSite ;
         _pmdRemoteSession          *_pSession ;
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
      return requireOrder() &&
             ( _orderedContextMap.size() + _emptyContextMap.size() +
               _prepareContextMap.size() > 1 ) ;
   }

   /*
      _rtnContextCoordExplain define
    */
   class _rtnContextCoordExplain : public _rtnContextCoord,
                                   public _rtnExplainMainBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()

      public :
         _rtnContextCoordExplain ( INT64 contextID, UINT64 eduID,
                                   BOOLEAN preRead = TRUE ) ;

         virtual ~_rtnContextCoordExplain () ;

         std::string name () const ;

         RTN_CONTEXT_TYPE getType () const ;

         INT32 open ( const rtnQueryOptions & options,
                      BOOLEAN preRead = TRUE ) ;

         void optimizeReturnOptions ( MsgOpQuery * pQueryMsg,
                                      UINT32 targetGroupNum ) ;

         INT32 addSubContext ( MsgOpReply *pReply, BOOLEAN &takeOver ) ;

         void addSubDone ( pmdEDUCB *cb ) ;

         OSS_INLINE BOOLEAN _needParallelProcess () const
         {
            return TRUE ;
         }

      protected :
         INT32 _prepareData ( pmdEDUCB *cb ) ;

         INT32 _openSubContext ( rtnQueryOptions & options,
                                 pmdEDUCB * cb,
                                 rtnContext ** ppContext ) ;

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

      protected :
         SET_ROUTEID             _locationFilter ;
         optExplainCoordPath     _explainCoordPath ;
         UINT64                  _endSessionTimeout ;
   } ;

   typedef class _rtnContextCoordExplain rtnContextCoordExplain ;

}

#endif //COORD_CONTEXT_HPP__
