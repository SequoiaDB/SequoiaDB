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

   Source File Name = rtnQueryOptions.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/05/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_QUERYOPTIONS_HPP_
#define RTN_QUERYOPTIONS_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "sdbInterface.hpp"
#include "../bson/bson.hpp"
#include "msg.hpp"
#include "ossMemPool.hpp"
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{

   // indicates the query is for count
   #define RTN_INTERNAL_QUERY_COUNT_FLAG ( 0x00000001 )

   /*
      _rtnReturnOptions define
    */
   class _rtnReturnOptions : public SDBObject
   {
      public :
         _rtnReturnOptions () ;

         _rtnReturnOptions ( const BSONObj &selector,
                             INT64 skip,
                             INT64 limit,
                             INT32 flag ) ;

         _rtnReturnOptions ( const CHAR *selector,
                             INT64 skip,
                             INT64 limit,
                             INT32 flag ) ;

         _rtnReturnOptions ( const _rtnReturnOptions &options ) ;

         virtual ~_rtnReturnOptions () ;

         _rtnReturnOptions & operator = ( const _rtnReturnOptions &options ) ;

         virtual void reset () ;

         virtual INT32 getOwned () ;

         /// Selector
         OSS_INLINE void setSelector ( const BSONObj &selector )
         {
            _selector = selector ;
         }

         OSS_INLINE const BSONObj & getSelector () const
         {
            return _selector ;
         }

         OSS_INLINE BSONObj * getSelectorPtr ()
         {
            return &_selector ;
         }

         OSS_INLINE UINT32 getSelectorFieldNum () const
         {
            return (UINT32)_selector.nFields() ;
         }

         OSS_INLINE BOOLEAN isSelectorEmpty () const
         {
            return _selector.isEmpty() ;
         }

         OSS_INLINE UINT32 getSelectorHash () const
         {
            return ossHash( _selector.objdata(), _selector.objsize() ) ;
         }

         /// Skip
         OSS_INLINE void setSkip ( INT64 skip )
         {
            _skip = skip > 0 ? skip : 0 ;
         }

         OSS_INLINE INT64 getSkip () const
         {
            return _skip ;
         }

         /// Limit
         OSS_INLINE void setLimit ( INT64 limit )
         {
            _limit = limit ;
         }

         OSS_INLINE INT64 getLimit () const
         {
            return _limit ;
         }

         /// Flag
         OSS_INLINE void resetFlag ( INT32 flag )
         {
            _flag = flag ;
         }

         OSS_INLINE void setFlag ( INT32 flag )
         {
            OSS_BIT_SET( _flag, flag ) ;
         }

         OSS_INLINE void clearFlag ( INT32 flag )
         {
            OSS_BIT_CLEAR( _flag, flag ) ;
         }

         OSS_INLINE INT32 getFlag () const
         {
            return _flag ;
         }

         OSS_INLINE BOOLEAN testFlag ( INT32 flag ) const
         {
            return OSS_BIT_TEST( _flag, flag ) ? TRUE : FALSE ;
         }

         OSS_INLINE void resetInternalFlag( INT32 internalFlag )
         {
            _internalFlag = internalFlag ;
         }

         OSS_INLINE void setInternalFlag( INT32 internalFlag )
         {
            OSS_BIT_SET( _internalFlag, internalFlag ) ;
         }

         OSS_INLINE void clearInternalFlag( INT32 internalFlag )
         {
            OSS_BIT_CLEAR( _internalFlag, internalFlag ) ;
         }

         OSS_INLINE INT32 getInternalFlag() const
         {
            return _internalFlag ;
         }

         OSS_INLINE BOOLEAN testInternalFlag( INT32 internalFlag ) const
         {
            return OSS_BIT_TEST( _internalFlag, internalFlag ) ? TRUE : FALSE ;
         }

      public :
         BSONObj        _selector ;
         INT64          _skip ;
         INT64          _limit ;
         INT32          _flag ;
         INT32          _internalFlag ;
   } ;

   typedef class _rtnReturnOptions rtnReturnOptions ;

   /*
      _rtnQueryOptions define
    */
   class _rtnQueryOptions : public _rtnReturnOptions
   {
      public :
         _rtnQueryOptions () ;

         _rtnQueryOptions ( const CHAR *query,
                            const CHAR *selector,
                            const CHAR *orderBy,
                            const CHAR *hint,
                            const CHAR *fullName,
                            SINT64 skip,
                            SINT64 limit,
                            INT32 flag ) ;

         _rtnQueryOptions ( const BSONObj &query,
                            const BSONObj &selector,
                            const BSONObj &orderBy,
                            const BSONObj &hint,
                            const CHAR *fullName,
                            SINT64 skip,
                            SINT64 limit,
                            INT32 flag ) ;

         _rtnQueryOptions ( const _rtnQueryOptions &o ) ;

         virtual ~_rtnQueryOptions () ;

         _rtnQueryOptions & operator = ( const _rtnQueryOptions &o ) ;

         INT32 fromQueryMsg ( CHAR *pMsg ) ;
         INT32 fromInsertMsg( CHAR *pMSg ) ;
         INT32 fromDeleteMsg( CHAR *pMsg ) ;
         INT32 fromUpdateMsg( CHAR *pMsg ) ;
         INT32 toQueryMsg ( CHAR **ppMsg, INT32 &buffSize,
                            IExecutor *cb = NULL ) const ;

         virtual void reset() ;
         virtual INT32 getOwned() ;

         string toString () const ;

         /// Query
         OSS_INLINE void setQuery ( const BSONObj & query )
         {
            _query = query ;
         }

         OSS_INLINE const BSONObj & getQuery () const
         {
            return _query ;
         }

         OSS_INLINE BSONObj * getQueryPtr ()
         {
            return &_query ;
         }

         OSS_INLINE UINT32 getQueryFieldNum () const
         {
            return (UINT32)_query.nFields() ;
         }

         OSS_INLINE UINT32 getQueryHash () const
         {
            return ossHash( _query.objdata(), _query.objsize() ) ;
         }

         OSS_INLINE BOOLEAN isQueryEmpty () const
         {
            return _query.isEmpty() ;
         }

         OSS_INLINE UINT32 getQuerySize() const
         {
            return (UINT32)( _query.objsize() ) ;
         }

         /// OrderBy
         OSS_INLINE void setOrderBy ( const BSONObj & orderBy )
         {
            _orderBy = orderBy ;
         }

         OSS_INLINE const BSONObj & getOrderBy () const
         {
            return _orderBy ;
         }

         OSS_INLINE BSONObj * getOrderByPtr ()
         {
            return &_orderBy ;
         }

         OSS_INLINE const BSONObj * getOrderByPtr () const
         {
            return &_orderBy ;
         }

         OSS_INLINE UINT32 getOrderByFieldNum () const
         {
            return (UINT32)_orderBy.nFields() ;
         }

         OSS_INLINE BOOLEAN isOrderByEmpty () const
         {
            return _orderBy.isEmpty() ;
         }

         OSS_INLINE UINT32 getOrderByHash () const
         {
            return ossHash( _orderBy.objdata(), _orderBy.objsize() ) ;
         }

         OSS_INLINE UINT32 getOrderBySize() const
         {
            return _orderBy.objsize() ;
         }

         /// Hint
         OSS_INLINE void setHint ( const BSONObj & hint )
         {
            _hint = hint ;
         }

         OSS_INLINE const BSONObj & getHint () const
         {
            return _hint ;
         }

         OSS_INLINE BSONObj * getHintPtr ()
         {
            return &_hint ;
         }

         OSS_INLINE const BSONObj * getHintPtr () const
         {
            return &_hint ;
         }

         OSS_INLINE UINT32 getHintFieldNum () const
         {
            return (UINT32)_hint.nFields() ;
         }

         OSS_INLINE BOOLEAN isHintEmpty () const
         {
            return _hint.isEmpty() ;
         }

         OSS_INLINE UINT32 getHintHash () const
         {
            return ossHash( _hint.objdata(), _hint.objsize() ) ;
         }

         OSS_INLINE UINT32 getHintSize() const
         {
            return _hint.objsize() ;
         }

         OSS_INLINE BOOLEAN hasRangeInHint() const
         {
            return isHintEmpty() ? FALSE
                                 : _hint.hasField( FIELD_NAME_RANGE ) ;
         }

         // insertor
         OSS_INLINE void setInsertor( const BSONObj &insertor )
         {
            // reuse selector as insertor
            _selector = insertor ;
         }

         OSS_INLINE const BSONObj &getInsertor() const
         {
            // reuse selector as insertor
            return _selector ;
         }

         OSS_INLINE void setInsertNum( INT32 insertNum )
         {
            // reuse skip as insert number
            _skip = insertNum ;
         }

         OSS_INLINE INT32 getInsertNum() const
         {
            // reuse skip as insert number
            return _skip ;
         }

         OSS_INLINE void setUpdator( const BSONObj &updator )
         {
            // reuse selector as updator
            _selector = updator ;
         }

         OSS_INLINE const BSONObj &getUpdator() const
         {
            // reuse selector as updator
            return _selector ;
         }

         /// Collection name
         void setCLFullName ( const CHAR *clFullName ) ;

         OSS_INLINE const CHAR * getCLFullName () const
         {
            return _fullName ;
         }

         /// Main-Collection name
         void setMainCLName ( const CHAR *mainCLName ) ;

         OSS_INLINE const CHAR * getMainCLName () const
         {
            return _mainCLName ;
         }

         void setMainCLQuery ( const CHAR *mainCLName, const CHAR *subCLName ) ;

         OSS_INLINE BOOLEAN canPrepareMore () const
         {
            return testFlag( FLG_QUERY_PREPARE_MORE )&&
                   !testFlag( FLG_QUERY_MODIFY ) ;
         }

         // WARNING: only used in explain
         BOOLEAN isQueryAndModify() const ;

      public :
         BSONObj        _query ;
         BSONObj        _orderBy ;
         BSONObj        _hint ;
         const CHAR *   _fullName ;
         ossPoolString  _fullNameBuf ;
         const CHAR *   _mainCLName ;
         ossPoolString  _mainCLNameBuf ;
   } ;

   typedef class _rtnQueryOptions rtnQueryOptions ;

   /*
      _rtnExplainOptions
    */
   class _rtnExplainOptions : public SDBObject
   {
   public:
      _rtnExplainOptions()
      : _showMask( 0 ),
        _needDetail( FALSE ),
        _needEstimate( FALSE ),
        _needRun( FALSE ),
        _needSearch( FALSE ),
        _needEvaluate( FALSE ),
        _needExpand( FALSE ),
        _needFlatten( FALSE ),
        _needAbbrev( FALSE ),
        _genBuilderOption( FALSE ),
        _builderOption()
      {
      }

      _rtnExplainOptions( const _rtnExplainOptions &options )
      : _showMask( options._showMask ),
        _needDetail( options._needDetail ),
        _needEstimate( options._needEstimate ),
        _needRun( options._needRun ),
        _needSearch( options._needSearch ),
        _needEvaluate( options._needEvaluate ),
        _needExpand( options._needExpand ),
        _needFlatten( options._needFlatten ),
        _needAbbrev( options._needAbbrev ),
        _genBuilderOption( FALSE ),
        _builderOption()
      {
      }

      ~_rtnExplainOptions()
      {
      }

      _rtnExplainOptions &operator =( const _rtnExplainOptions &options )
      {
         _showMask = options._showMask ;
         _needDetail = options._needDetail ;
         _needEstimate = options._needEstimate ;
         _needRun = options._needRun ;
         _needSearch = options._needSearch ;
         _needEvaluate = options._needEvaluate ;
         _needExpand = options._needExpand ;
         _needFlatten = options._needFlatten ;
         _needAbbrev = options._needAbbrev ;
         _genBuilderOption = FALSE ;
         _builderOption.reset() ;
         return ( *this ) ;
      }

      OSS_INLINE UINT16 getShowMask() const
      {
         return _showMask ;
      }

      OSS_INLINE void setShowMask( UINT16 mask )
      {
         _showMask = mask ;
      }

      OSS_INLINE BOOLEAN isNeedDetail() const
      {
         return _needDetail ;
      }

      OSS_INLINE void setNeedDetail( BOOLEAN needDetail )
      {
         _needDetail = needDetail ;
      }

      OSS_INLINE BOOLEAN isNeedEstimate() const
      {
         return _needEstimate ;
      }

      OSS_INLINE void setNeedEstimate( BOOLEAN needEstimate )
      {
         _needEstimate = needEstimate ;
      }

      OSS_INLINE BOOLEAN isNeedRun() const
      {
         return _needRun ;
      }

      OSS_INLINE void setNeedRun( BOOLEAN needRun )
      {
         _needRun = needRun ;
      }

      OSS_INLINE BOOLEAN isNeedSearch() const
      {
         return _needSearch ;
      }

      OSS_INLINE void setNeedSearch( BOOLEAN needSearch )
      {
         _needSearch = needSearch ;
      }

      OSS_INLINE BOOLEAN isNeedEvaluate() const
      {
         return _needEvaluate ;
      }

      OSS_INLINE void setNeedEvaluate( BOOLEAN needEvaluate )
      {
         _needEvaluate = needEvaluate ;
      }

      OSS_INLINE BOOLEAN isNeedExpand() const
      {
         return _needExpand ;
      }

      OSS_INLINE void setNeedExpand( BOOLEAN needExpand )
      {
         _needExpand = needExpand ;
      }

      OSS_INLINE BOOLEAN isNeedFlatten() const
      {
         return _needFlatten ;
      }

      OSS_INLINE void setNeedFlatten( BOOLEAN needFlatten )
      {
         _needFlatten = needFlatten ;
      }

      OSS_INLINE BOOLEAN isNeedAbbrev() const
      {
         return _needAbbrev ;
      }

      OSS_INLINE void setNeedAbbrev( BOOLEAN needAbbrev )
      {
         _needAbbrev = needAbbrev ;
      }

      OSS_INLINE const BSONObjBuilderOption &getBuilderOption() const
      {
         if ( !_genBuilderOption )
         {
            _builderOption.isClientReadable = false ;
            _builderOption.isAbbrevMode = _needAbbrev ;
         }
         return _builderOption ;
      }

   protected:
      // mask of explain information to be shown ( e.g. input, output, etc )
      UINT16      _showMask ;
      // indicates whether to show details of explain
      BOOLEAN     _needDetail ;
      // indicates whether to show estimate details of explain
      BOOLEAN     _needEstimate ;
      // indicates whether to run the explained access plan
      BOOLEAN     _needRun ;
      // indicates whether to keep search paths in explain result
      BOOLEAN     _needSearch ;
      // indicates whether to show evaluation ( cost based optimization )
      // of access plan
      BOOLEAN     _needEvaluate ;
      // indicates whether to expand all details of explain
      BOOLEAN     _needExpand ;
      // indicates whether to flatten details of explain into multiple rows
      BOOLEAN     _needFlatten ;
      // indicates whether to output explain in abbreviation mode
      BOOLEAN     _needAbbrev ;

      mutable BOOLEAN _genBuilderOption ;
      mutable BSONObjBuilderOption _builderOption ;
   } ;

   typedef class _rtnExplainOptions rtnExplainOptions ;

}

#endif
