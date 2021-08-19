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

   static BSONObj s_dummy ;

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
      _selector = s_dummy ;
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
     _mainCLName( NULL )
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
     _mainCLName( NULL )
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
     _mainCLName( NULL )
   {
   }

   _rtnQueryOptions::_rtnQueryOptions ( const _rtnQueryOptions &o )
   : _rtnReturnOptions( o ),
     _query( o._query ),
     _orderBy( o._orderBy ),
     _hint( o._hint ),
     _fullName( o._fullName ),
     _mainCLName( o._mainCLName )
   {
   }

   _rtnQueryOptions::~_rtnQueryOptions ()
   {
      _fullName = NULL ;
      _mainCLName = NULL ;
   }

   void _rtnQueryOptions::reset()
   {
      _rtnReturnOptions::reset() ;

      _query = s_dummy ;
      _orderBy = s_dummy ;
      _hint = s_dummy ;
      _mainCLNameBuf.clear() ;
      _mainCLName = NULL ;
      _fullNameBuf.clear() ;
      _fullName = NULL ;
   }

   INT32 _rtnQueryOptions::getOwned ()
   {
      _rtnReturnOptions::getOwned() ;

      _fullNameBuf.clear() ;
      if ( NULL != _fullName )
      {
         _fullNameBuf = _fullName ;
         _fullName = _fullNameBuf.c_str() ;
      }
      else
      {
         _fullName = NULL ;
      }

      _mainCLNameBuf.clear() ;
      if ( NULL != _mainCLName )
      {
         _mainCLNameBuf = _mainCLName ;
         _mainCLName = _mainCLNameBuf.c_str() ;
      }
      else
      {
         _mainCLName = NULL ;
      }

      _query = _query.getOwned() ;
      _orderBy = _orderBy.getOwned() ;
      _hint = _hint.getOwned() ;

      return SDB_OK ;
   }

   void _rtnQueryOptions::setCLFullName ( const CHAR *clFullName )
   {
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
      _fullNameBuf.clear() ;
      _mainCLName = o._mainCLName ;
      _mainCLNameBuf.clear() ;

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

      _fullNameBuf.clear() ;
      _mainCLNameBuf.clear() ;

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

