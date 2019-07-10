/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilESFetcher.cpp

   Descriptive Name = Elasticsearch fetcher.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilESFetcher.hpp"
#include "pd.hpp"
#include "pmdEDU.hpp"

using namespace engine ;

#define ES_SCROLL_FIX_SIZE          10000

namespace seadapter
{
   _utilESFetcher::_utilESFetcher( const CHAR *index, const CHAR *type )
   {
      SDB_ASSERT( index, "Index is NULL" ) ;
      SDB_ASSERT( type, "type is NULL" ) ;
      _clt = NULL ;
      _index = std::string( index ) ;
      _type = std::string( type ) ;
      _size = 0 ;
      _more = TRUE ;
   }

   _utilESFetcher::~_utilESFetcher()
   {
   }

   INT32 _utilESFetcher::setClt( utilESClt *esClt )
   {
      INT32 rc = SDB_OK ;

      if ( !esClt )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "esClt is NULL" ) ;
         goto error ;
      }
      _clt = esClt ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESFetcher::setCondition( const BSONObj &condObj )
   {
      INT32 rc = SDB_OK ;
      try
      {
         _query = condObj.copy() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilESFetcher::setSize( INT64 size )
   {
      _size = size ;
   }

   void _utilESFetcher::setFilterPath( const CHAR *filterPath )
   {
      SDB_ASSERT( filterPath, "filter path is NULL" ) ;
      _filterPath = filterPath ;
   }

   _utilESPageFetcher::_utilESPageFetcher( const CHAR *index, const CHAR *type )
   : _utilESFetcher( index, type )
   {
   }

   _utilESPageFetcher::~_utilESPageFetcher()
   {
   }

   void _utilESPageFetcher::setFrom( INT64 from )
   {
      _from = from ;
   }

   BOOLEAN _utilESPageFetcher::more()
   {
      return _more ;
   }

   INT32 _utilESPageFetcher::fetch( utilCommObjBuff &result )
   {
      INT32 rc = SDB_OK ;

      if ( _more )
      {
         rc = _clt->getDocument( _index.c_str(), _type.c_str(),
                                 _query.toString( FALSE, TRUE ).c_str(),
                                 result, FALSE ) ;
         if ( rc )
         {
            const CHAR *errMsg = _clt->getLastErrMsg() ;
            if ( errMsg )
            {
               PD_LOG_MSG( PDERROR, "Get document of index[ %s ] and "
                           "type[ %s ] failed[ %d ], query string: %s. "
                           "Error message: %s",
                           _index.c_str(), _type.c_str(), rc,
                           _query.toString( FALSE, TRUE ).c_str(),
                           _clt->getLastErrMsg() ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Get document of index[ %s ] and "
                           "type[ %s ] failed[ %d ], query string: %s.",
                           _index.c_str(), _type.c_str(), rc,
                           _query.toString( FALSE, TRUE ).c_str() ) ;
            }
            goto error ;
         }
         _more = FALSE ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _utilESScrollFetcher::_utilESScrollFetcher( const CHAR *index,
                                               const CHAR *type )
   : _utilESFetcher( index, type )
   {
   }

   _utilESScrollFetcher::~_utilESScrollFetcher()
   {
      if ( !_scrollID.empty() )
      {
         _clt->clearScroll( _scrollID ) ;
      }
   }

   BOOLEAN _utilESScrollFetcher::more()
   {
      return _more ;
   }

   INT32 _utilESScrollFetcher::fetch( utilCommObjBuff &result )
   {
      INT32 rc = SDB_OK ;

      if ( _scrollID.empty() )
      {
         rc = _clt->initScroll( _scrollID, _index.c_str(), _type.c_str(),
                                _query.toString( FALSE, TRUE ), result,
                                ES_SCROLL_FIX_SIZE, _filterPath.c_str() ) ;
         if ( rc )
         {
            const CHAR *errMsg = _clt->getLastErrMsg() ;
            if ( errMsg )
            {
               PD_LOG_MSG( PDERROR, "Initialize scroll for index[ %s ] and "
                           "type[ %s ] failed[ %d ], query string: %s. "
                           "Error message: %s",
                           _index.c_str(), _type.c_str(), rc,
                           _query.toString( FALSE, TRUE ).c_str(),
                           _clt->getLastErrMsg() ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Initialize scroll for index[ %s ] and "
                           "type[ %s ] failed[ %d ], query string: %s.",
                           _index.c_str(), _type.c_str(), rc,
                           _query.toString( FALSE, TRUE ).c_str() ) ;
            }
            goto error ;
         }
      }
      else
      {
         rc = _clt->scrollNext( _scrollID, result, _filterPath.c_str() ) ;
         if ( rc )
         {
            const CHAR *errMsg = _clt->getLastErrMsg() ;
            if ( errMsg )
            {
               PD_LOG_MSG( PDERROR, "Scroll with id[ %s ] for index[ %s ] and "
                           "type[ %s ] failed[ %d ]. Error message: %s",
                           _scrollID.c_str(), _index.c_str(), _type.c_str(),
                           rc, _clt->getLastErrMsg() ) ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Scroll with id[ %s ] for index[ %s ] and "
                           "type[ %s ] failed[ %d ].",
                           _scrollID.c_str(), _index.c_str(),
                           _type.c_str(), rc ) ;
            }
            goto error ;
         }
      }

      if ( 0 == result.getObjNum() )
      {
         _more = FALSE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

