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
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{

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

         OSS_INLINE void setSkip ( INT64 skip )
         {
            _skip = skip > 0 ? skip : 0 ;
         }

         OSS_INLINE INT64 getSkip () const
         {
            return _skip ;
         }

         OSS_INLINE void setLimit ( INT64 limit )
         {
            _limit = limit ;
         }

         OSS_INLINE INT64 getLimit () const
         {
            return _limit ;
         }

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

      public :
         BSONObj        _selector ;
         INT64          _skip ;
         INT64          _limit ;
         INT32          _flag ;
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
         INT32 toQueryMsg ( CHAR **ppMsg, INT32 &buffSize,
                            IExecutor *cb = NULL ) const ;

         virtual INT32 getOwned() ;

         string toString () const ;

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

         void setCLFullName ( const CHAR *clFullName ) ;

         OSS_INLINE const CHAR * getCLFullName () const
         {
            return _fullName ;
         }

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

      public :
         BSONObj        _query ;
         BSONObj        _orderBy ;
         BSONObj        _hint ;
         const CHAR *   _fullName ;
         CHAR *         _fullNameBuf ;
         const CHAR *   _mainCLName ;
         CHAR *         _mainCLNameBuf ;
   } ;

   typedef class _rtnQueryOptions rtnQueryOptions ;

}

#endif
