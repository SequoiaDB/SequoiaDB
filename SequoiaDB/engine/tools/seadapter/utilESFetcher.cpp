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
#include "utilESCltMgr.hpp"

using namespace engine ;

#define ES_SCROLL_FIX_SIZE          10000

namespace seadapter
{
   _utilESFetcher::_utilESFetcher( const CHAR *index, const CHAR *type )
   {
      SDB_ASSERT( index, "Index is NULL" ) ;
      SDB_ASSERT( type, "type is NULL" ) ;
      _index = std::string( index ) ;
      _type = std::string( type ) ;
      _size = 0 ;
      _esCltMgr = utilGetESCltMgr() ;
   }

   _utilESFetcher::~_utilESFetcher()
   {
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
      _from = 0 ;
      _fetchDone =  FALSE ;
   }

   _utilESPageFetcher::~_utilESPageFetcher()
   {
   }

   void _utilESPageFetcher::setFrom( INT64 from )
   {
      _from = from ;
   }

   INT32 _utilESPageFetcher::fetch( utilCommObjBuff &result )
   {
      INT32 rc = SDB_OK ;
      UINT32 origNum = result.getObjNum() ;
      utilESClt *client = NULL ;
      rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      if ( !_fetchDone )
      {
         rc = client->getDocument( _index.c_str(), _type.c_str(),
                                   _query.toString( FALSE, TRUE ).c_str(),
                                   result, FALSE ) ;
         if ( rc )
         {
            const CHAR *errMsg = client->getLastErrMsg() ;
            if ( errMsg )
            {
               PD_LOG_MSG( PDERROR, "Get document of index[ %s ] and "
                           "type[ %s ] failed[ %d ], query string: %s. "
                           "Error message: %s",
                           _index.c_str(), _type.c_str(), rc,
                           _query.toString( FALSE, TRUE ).c_str(),
                           client->getLastErrMsg() ) ;
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
         _fetchDone = TRUE ;

         if ( result.getObjNum() - origNum == 0 )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
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
      // User may stop fetching data at any time. Be sure to release the scroll
      // resource on search engine.
      if ( !_scrollID.empty() )
      {
         INT32 rc = SDB_OK ;
         utilESClt *client = NULL ;
         rc = _esCltMgr->getClient( client ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get search engine client failed[%d]. Scroll [%s] "
                    "is not cleaned on search engine. It will be cleaned when "
                    "timeout", rc, _scrollID.c_str() ) ;
         }
         else
         {
            client->clearScroll( _scrollID ) ;
            _esCltMgr->releaseClient( client ) ;
         }
      }
   }

   INT32 _utilESScrollFetcher::fetch( utilCommObjBuff &result )
   {
      INT32 rc = SDB_OK ;
      UINT32 origNum = result.getObjNum() ;
      utilESClt *client = NULL ;
      rc = _esCltMgr->getClient( client ) ;
      PD_RC_CHECK( rc, PDERROR, "Get search engine client failed[%d]", rc ) ;

      if ( _scrollID.empty() )
      {
         // We just want the scroll id and document id. So use the filter path.
         rc = client->initScroll( _scrollID, _index.c_str(), _type.c_str(),
                                  _query.toString( FALSE, TRUE ), result,
                                  _esCltMgr->getScrollSize(),
                                  _filterPath.c_str() ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               const CHAR *errMsg = client->getLastErrMsg() ;
               if ( errMsg )
               {
                  PD_LOG_MSG( PDERROR, "Initialize scroll for index[ %s ] and "
                              "type[ %s ] failed[ %d ], query string: %s. "
                              "Error message: %s",
                              _index.c_str(), _type.c_str(), rc,
                              _query.toString( FALSE, TRUE ).c_str(),
                              client->getLastErrMsg() ) ;
               }
               else
               {
                  PD_LOG_MSG( PDERROR, "Initialize scroll for index[ %s ] and "
                              "type[ %s ] failed[ %d ], query string: %s.",
                              _index.c_str(), _type.c_str(), rc,
                              _query.toString( FALSE, TRUE ).c_str() ) ;
               }
            }
            goto error ;
         }
      }
      else
      {
         rc = client->scrollNext( _scrollID, result, _filterPath.c_str() ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               const CHAR *errMsg = client->getLastErrMsg() ;
               if ( errMsg )
               {
                  PD_LOG_MSG( PDERROR, "Scroll with id[ %s ] for index[ %s ] "
                              "and type[ %s ] failed[ %d ]. Error message: %s",
                              _scrollID.c_str(), _index.c_str(), _type.c_str(),
                              rc, client->getLastErrMsg() ) ;
               }
               else
               {
                  PD_LOG_MSG( PDERROR, "Scroll with id[ %s ] for index[ %s ] "
                              "and type[ %s ] failed[ %d ].",
                              _scrollID.c_str(), _index.c_str(),
                              _type.c_str(), rc ) ;
               }
            }
            goto error ;
         }
      }

      if ( result.getObjNum() - origNum == 0 )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      if ( client )
      {
         _esCltMgr->releaseClient( client ) ;
      }
      return rc ;
   error:
      if ( !_scrollID.empty() )
      {
         if ( client )
         {
            client->clearScroll( _scrollID ) ;
            _scrollID.clear() ;
         }
         else
         {
            PD_LOG( PDWARNING, "Scroll [%s] is not cleaned on search engine. "
                    "It will be cleaned when timeout", _scrollID.c_str() ) ;
         }
      }
      goto done ;
   }
}

