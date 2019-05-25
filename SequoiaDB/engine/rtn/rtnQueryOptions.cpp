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

#include "rtnQueryOptions.hpp"
#include "ossUtil.hpp"
#include "msgMessage.hpp"
#include <sstream>

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _rtnReturnOptions implement
    */
   _rtnReturnOptions::_rtnReturnOptions ()
   : _skip( 0 ),
     _limit( -1 ),
     _flag( 0 )
   {
   }

   _rtnReturnOptions::_rtnReturnOptions ( const BSONObj &selector,
                                          INT64 skip,
                                          INT64 limit,
                                          INT32 flag )
   : _selector( selector ),
     _skip( skip ),
     _limit( limit ),
     _flag( flag )
   {
   }

   _rtnReturnOptions::_rtnReturnOptions ( const CHAR *selector,
                                          INT64 skip,
                                          INT64 limit,
                                          INT32 flag )
   : _selector( selector ),
     _skip( skip ),
     _limit( limit ),
     _flag( flag )
   {
   }

   _rtnReturnOptions::_rtnReturnOptions ( const _rtnReturnOptions &options )
   : _selector( options._selector ),
     _skip( options._skip ),
     _limit( options._limit ),
     _flag( options._flag )
   {
   }

   _rtnReturnOptions::~_rtnReturnOptions ()
   {
   }

   _rtnReturnOptions & _rtnReturnOptions::operator = ( const _rtnReturnOptions &options )
   {
      _selector = options._selector ;
      _skip = options._skip ;
      _limit = options._limit ;
      _flag = options._flag ;
      return (*this) ;
   }

   void _rtnReturnOptions::reset ()
   {
      _selector = BSONObj() ;
      _skip = 0 ;
      _limit = -1 ;
      _flag = 0 ;
   }

   INT32 _rtnReturnOptions::getOwned ()
   {
      _selector = _selector.getOwned() ;
      return SDB_OK ;
   }

   /*
      _rtnQueryOptions implement
    */
   _rtnQueryOptions::_rtnQueryOptions ()
   : _rtnReturnOptions(),
     _fullName( NULL ),
     _fullNameBuf( NULL ),
     _mainCLName( NULL ),
     _mainCLNameBuf( NULL )
   {
   }

   _rtnQueryOptions::_rtnQueryOptions ( const CHAR *query,
                                        const CHAR *selector,
                                        const CHAR *orderBy,
                                        const CHAR *hint,
                                        const CHAR *fullName,
                                        SINT64 skip,
                                        SINT64 limit,
                                        INT32 flag )
   : _rtnReturnOptions( selector, skip, limit, flag ),
     _query( query ),
     _orderBy( orderBy ),
     _hint( hint ),
     _fullName( fullName ),
     _fullNameBuf( NULL ),
     _mainCLName( NULL ),
     _mainCLNameBuf( NULL )
   {
   }

   _rtnQueryOptions::_rtnQueryOptions ( const BSONObj &query,
                                        const BSONObj &selector,
                                        const BSONObj &orderBy,
                                        const BSONObj &hint,
                                        const CHAR *fullName,
                                        SINT64 skip,
                                        SINT64 limit,
                                        INT32 flag )
   : _rtnReturnOptions( selector, skip, limit, flag ),
     _query( query ),
     _orderBy( orderBy ),
     _hint( hint ),
     _fullName( fullName ),
     _fullNameBuf( NULL ),
     _mainCLName( NULL ),
     _mainCLNameBuf( NULL )
   {
   }

   _rtnQueryOptions::_rtnQueryOptions ( const _rtnQueryOptions &o )
   : _rtnReturnOptions( o ),
     _query( o._query ),
     _orderBy( o._orderBy ),
     _hint( o._hint ),
     _fullName( o._fullName ),
     _fullNameBuf( NULL ),
     _mainCLName( o._mainCLName ),
     _mainCLNameBuf( NULL )
   {
   }

   _rtnQueryOptions::~_rtnQueryOptions ()
   {
      SAFE_OSS_FREE( _fullNameBuf ) ;
      SAFE_OSS_FREE( _mainCLNameBuf ) ;
      _fullName = NULL ;
      _mainCLName = NULL ;
   }

   INT32 _rtnQueryOptions::getOwned ()
   {
      INT32 rc = SDB_OK ;

      _rtnReturnOptions::getOwned() ;

      SAFE_OSS_FREE( _fullNameBuf ) ;
      if ( NULL != _fullName )
      {
         _fullNameBuf = ossStrdup( _fullName ) ;
         if ( NULL == _fullNameBuf )
         {
            rc = SDB_OOM ;
            goto error ;
         }
      }
      _fullName = _fullNameBuf ;

      SAFE_OSS_FREE( _mainCLNameBuf ) ;
      if ( NULL != _mainCLName )
      {
         _mainCLNameBuf = ossStrdup( _mainCLName ) ;
         if ( NULL == _mainCLNameBuf )
         {
            rc = SDB_OOM ;
            goto error ;
         }
      }
      _mainCLName = _mainCLNameBuf ;

      _query = _query.getOwned() ;
      _orderBy = _orderBy.getOwned() ;
      _hint = _hint.getOwned() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnQueryOptions::setCLFullName ( const CHAR *clFullName )
   {
      SAFE_OSS_FREE( _fullNameBuf ) ;
      if ( NULL != clFullName && '\0' != clFullName[0] )
      {
         _fullName = clFullName ;
      }
      else
      {
         _fullName = NULL ;
      }
   }

   void _rtnQueryOptions::setMainCLName ( const CHAR *mainCLName )
   {
      SAFE_OSS_FREE( _mainCLNameBuf ) ;
      if ( NULL != mainCLName && '\0' != mainCLName[0] )
      {
         _mainCLName = mainCLName ;
      }
      else
      {
         _mainCLName = NULL ;
      }
   }

   void _rtnQueryOptions::setMainCLQuery ( const CHAR *mainCLName,
                                           const CHAR *subCLName )
   {
      SDB_ASSERT( NULL != mainCLName && '\0' != mainCLName[0],
                  "Invalid main-collection name" ) ;
      SDB_ASSERT( NULL != subCLName && '\0' != subCLName[0],
                  "Invalid sub-collection name" ) ;
      setMainCLName( mainCLName ) ;
      setCLFullName( subCLName ) ;
   }

   _rtnQueryOptions &_rtnQueryOptions::operator = ( const _rtnQueryOptions &o )
   {
      _rtnReturnOptions::operator =( o ) ;

      _query = o._query ;
      _orderBy = o._orderBy ;
      _hint = o._hint ;
      _fullName = o._fullName ;
      SAFE_OSS_FREE( _fullNameBuf ) ;
      _mainCLName = o._mainCLName ;
      SAFE_OSS_FREE( _mainCLNameBuf ) ;

      return *this ;
   }

   string _rtnQueryOptions::toString () const
   {
      stringstream ss ;
      if ( _fullName )
      {
         ss << "Name: " << _fullName ;
         ss << ", Query: " << _query.toString() ;
         ss << ", Selector: " << _selector.toString() ;
         ss << ", OrderBy: " << _orderBy.toString() ;
         ss << ", Hint: " << _hint.toString() ;
         ss << ", Skip: " << _skip ;
         ss << ", Limit: " << _limit ;
         ss << ", Flags: " << _flag ;
         if ( NULL != _mainCLName )
         {
            ss << ", MainCLName: " << _mainCLName ;
         }
      }
      return ss.str() ;
   }

   INT32 _rtnQueryOptions::fromQueryMsg ( CHAR *pMsg )
   {
      INT32 rc = SDB_OK ;
      CHAR *pQuery = NULL ;
      CHAR *pSelector = NULL ;
      CHAR *pOrderBy = NULL ;
      CHAR *pHint = NULL ;

      rc = msgExtractQuery( pMsg, &_flag, (CHAR**)&_fullName, &_skip, &_limit,
                            &pQuery, &pSelector, &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Extrace query msg failed, rc: %d", rc ) ;

      if ( NULL != _fullNameBuf )
      {
         SDB_OSS_FREE( _fullNameBuf ) ;
         _fullNameBuf = NULL ;
      }

      try
      {
         _query = BSONObj( pQuery ) ;
         _selector = BSONObj( pSelector ) ;
         _orderBy = BSONObj( pOrderBy ) ;
         _hint = BSONObj( pHint ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Extrace query msg occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnQueryOptions::toQueryMsg ( CHAR **ppMsg,
                                        INT32 &buffSize,
                                        IExecutor *cb ) const
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( ppMsg, "ppMsg can't be NULL" ) ;

      rc = msgBuildQueryMsg( ppMsg, &buffSize, _fullName, _flag, 0,
                             _skip, _limit, &_query, &_selector, &_orderBy,
                             &_hint, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build query msg failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

