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

   Source File Name = rtnContextExplain.hpp

   Descriptive Name = RunTime Explain Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/15/2017  HGM Split from rtnContextData.hpp

   Last Changed =

*******************************************************************************/

#ifndef RTN_CONTEXT_EXPLAIN_HPP_
#define RTN_CONTEXT_EXPLAIN_HPP_

#include "rtnQueryOptions.hpp"
#include "rtnContext.hpp"
#include "rtnContextDataDispatcher.hpp"
#include "optAccessPlanRuntime.hpp"
#include "utilSet.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   #define RTN_MAX_EXPLAIN_BUFFER_SIZE ( 2097152 )

   /*
      _rtnExplainBase define
    */
   class _rtnExplainBase : public _rtnSubContextHolder
   {
      public :
         _rtnExplainBase ( optExplainPath * explainPath ) ;

         virtual ~_rtnExplainBase () ;

      protected :
         OSS_INLINE virtual BOOLEAN _needCollectSubExplains () const
         {
            return FALSE ;
         }

         OSS_INLINE virtual BOOLEAN _needReturnDataInRun () const
         {
            return FALSE ;
         }

         OSS_INLINE virtual BOOLEAN _needResetExplainOptions () const
         {
            return FALSE ;
         }

         OSS_INLINE virtual BOOLEAN _needResetTotalRecords () const
         {
            return FALSE ;
         }

         INT32 _openExplain ( const rtnQueryOptions & options,
                              pmdEDUCB * cb ) ;

         INT32 _prepareExplain ( rtnContext * explainContext,
                                 pmdEDUCB * cb ) ;

         virtual INT32 _openSubContext ( rtnQueryOptions & options,
                                         pmdEDUCB * cb,
                                         rtnContext ** ppContext ) = 0 ;

         OSS_INLINE virtual INT32 _finishSubContext ( rtnContext * context,
                                                      pmdEDUCB * cb )
         {
            return SDB_OK ;
         }

         virtual INT32 _prepareExplainPath ( rtnContext * context,
                                             pmdEDUCB * cb ) = 0 ;

         virtual INT32 _buildExplain ( rtnContext * explainContext,
                                       BOOLEAN & hasMore ) = 0 ;

         INT32 _extractExplainOptions ( const BSONObj & hint,
                                        BSONObj & explainOptions,
                                        BSONObj & realHint ) ;

         INT32 _parseExplainOptions ( const BSONObj & options ) ;

         virtual INT32 _parseLocationOption ( const BSONObj & explainOptions,
                                              BOOLEAN & hasOption ) ;

      private :
         INT32 _parseBoolOption ( const BSONObj & options,
                                  const CHAR * optionName,
                                  BOOLEAN & option,
                                  BOOLEAN & hasOption,
                                  BOOLEAN defaultValue ) ;

         INT32 _parseMaskOption ( const BSONObj & options,
                                  const CHAR * optionName,
                                  BOOLEAN & hasMask,
                                  UINT16 & mask ) ;

         INT32 _parseMask ( const CHAR * maskName,
                            UINT16 & mask ) ;

      protected :
         INT32 _buildBSONNodeInfo ( BSONObjBuilder & builder ) const ;

         INT32 _buildBSONQueryOptions ( BSONObjBuilder & builder,
                                        BOOLEAN needDetail ) const ;

      protected :
         rtnQueryOptions _queryOptions ;

         UINT16 _explainMask ;
         BOOLEAN _needDetail ;
         BOOLEAN _needEstimate ;
         BOOLEAN _needRun ;
         BOOLEAN _needSearch ;
         BOOLEAN _needEvaluate ;
         BOOLEAN _needExpand ;
         BOOLEAN _needFlatten ;

         BOOLEAN _explainStarted ;
         BOOLEAN _explainRunned ;
         BOOLEAN _explainPrepared ;
         BOOLEAN _explained ;

         optPlanAllocator     _planAllocator ;
         optExplainPath *     _explainPath ;
   } ;

   /*
      _rtnContextExplain define
    */
   class _rtnContextExplain : public _rtnContextBase,
                              public _rtnExplainBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()

   public :
      _rtnContextExplain ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_rtnContextExplain () ;

   public :
      OSS_INLINE virtual string name () const
      {
         return "EXPLAIN" ;
      }

      OSS_INLINE virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_EXPLAIN ;
      }

      OSS_INLINE virtual dmsStorageUnit * getSU ()
      {
         return NULL ;
      }

      INT32 open ( const rtnQueryOptions &options,
                   pmdEDUCB *cb ) ;

   protected :
      virtual INT32     _prepareData ( pmdEDUCB * cb ) ;
      virtual BOOLEAN   _canPrefetch () const { return FALSE ; }
      virtual void      _toString ( stringstream & ss ) ;

      OSS_INLINE BOOLEAN _needReturnDataInRun () const
      {
         return !_fromLocal ;
      }

      OSS_INLINE BOOLEAN _needResetExplainOptions () const
      {
         return TRUE ;
      }

      OSS_INLINE BOOLEAN _needResetTotalRecords () const
      {
         return !_fromLocal ;
      }

      INT32 _openSubContext ( rtnQueryOptions & options,
                              pmdEDUCB * cb,
                              rtnContext ** ppContext ) ;

      INT32 _prepareExplainPath ( rtnContext * context,
                                  pmdEDUCB * cb ) ;

      INT32 _buildExplain ( rtnContext * explainContext,
                            BOOLEAN & hasMore ) ;

   protected :
      BOOLEAN              _fromLocal ;
      optExplainScanPath   _explainScanPath ;
   } ;

   typedef class _rtnContextExplain rtnContextExplain ;

   /*
      _rtnExplainMainBase define
    */
   class _rtnExplainMainBase : public _rtnExplainBase,
                               public _IRtnCtxDataProcessor
   {
      public :
         _rtnExplainMainBase ( optExplainMergePathBase * explainMergePath ) ;

         virtual ~_rtnExplainMainBase () ;

         OSS_INLINE BOOLEAN _needCollectSubExplains () const
         {
            return TRUE ;
         }

         OSS_INLINE RTN_CTX_DATA_PROCESSOR_TYPE getProcessType () const
         {
            return RTN_CTX_EXPLAIN_PROCESSOR ;
         }

         OSS_INLINE BOOLEAN needCheckData () const
         {
            return TRUE ;
         }

         OSS_INLINE BOOLEAN needCheckSubContext () const
         {
            return TRUE ;
         }

         INT32 processData ( INT64 dataID, const CHAR * data,
                             INT32 dataSize, INT32 dataNum ) ;
         INT32 checkData ( INT64 dataID, const CHAR * data,
                           INT32 dataSize, INT32 dataNum ) ;
         INT32 checkSubContext ( INT64 dataID ) ;

      protected :
         virtual BOOLEAN _needParallelProcess () const = 0 ;

         INT32 _registerExplainProcessor ( rtnContext * context ) ;
         INT32 _unregisterExplainProcessor ( rtnContext * context ) ;

         INT32 _finishSubContext ( rtnContext * context,
                                   pmdEDUCB * cb ) ;

         virtual BOOLEAN _needChildExplain ( INT64 dataID,
                                             const BSONObj & childExplain ) = 0 ;

         INT32 _buildExplain ( rtnContext * explainContext,
                               BOOLEAN & hasMore ) ;

         virtual INT32 _buildMainExplain ( rtnContext * explainContext,
                                           BOOLEAN & hasMore ) ;
         virtual INT32 _buildSubExplains ( rtnContext * explainContext,
                                           BOOLEAN needSort,
                                           BOOLEAN & hasMore ) ;
         virtual INT32 _buildSimpleExplain ( rtnContext * explainContext,
                                             BOOLEAN & hasMore ) = 0 ;

         OSS_INLINE virtual UINT16 _getExplainInfoMask () const
         {
            return OPT_EXPINFO_MASK_ALL ;
         }

      protected :
         typedef _utilMap< INT64, ossTick > rtnExplainTimestampList ;
         typedef _utilSet< INT64 > rtnExplainIDList ;

      protected :
         ossTick                    _tempTimestamp ;
         rtnExplainTimestampList    _startTimestampList ;
         rtnExplainTimestampList    _waitTimestampList ;
         rtnExplainTimestampList    _endTimestampList ;
         rtnExplainIDList           _explainIDSet ;
         BOOLEAN                    _mainExplainOutputted ;
         optExplainMergePathBase *  _explainMergeBasePath ;
   } ;

}

#endif /* RTN_CONTEXT_EXPLAIN_HPP_ */
