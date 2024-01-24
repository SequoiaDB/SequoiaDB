/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = utilAddress.cpp

   Descriptive Name = Address utility

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilAddress.hpp"
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

using std::vector ;
using std::string ;
namespace engine
{
   _utilAddrPair::_utilAddrPair()
   {
      ossMemset( _host, 0, OSS_MAX_HOSTNAME + 1 ) ;
      ossMemset( _service, 0, OSS_MAX_SERVICENAME + 1 ) ;
   }

   _utilAddrPair::~_utilAddrPair()
   {
   }

   INT32 _utilAddrPair::setHost( const CHAR *host )
   {
      INT32 rc = SDB_OK ;

      if ( !host || ossStrlen(host) > OSS_MAX_HOSTNAME )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossStrncpy( _host, host, OSS_MAX_HOSTNAME ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _utilAddrPair::getHost() const
   {
      return _host ;
   }

   INT32 _utilAddrPair::setService( const CHAR *service )
   {
      INT32 rc = SDB_OK ;

      if ( !service || ossStrlen(service) > OSS_MAX_SERVICENAME )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossStrncpy( _service, service, OSS_MAX_SERVICENAME ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _utilAddrPair::getService() const
   {
      return _service ;
   }

   BOOLEAN _utilAddrPair::operator==( const _utilAddrPair& r ) const
   {
      return ( 0 == ossStrcmp( _host, r._host ) &&
               0 == ossStrcmp( _service, r._service ) ) ;
   }

   INT32 _utilAddrContainer::append( const utilAddrPair &addr )
   {
      return _addresses.append( addr ) ;
   }

   BOOLEAN _utilAddrContainer::contains( const utilAddrPair& addr )
   {
      utilAddrPair item ;
      _utilArray<utilAddrPair>::iterator itr( _addresses ) ;
      while ( itr.next( item ) )
      {
         if ( item == addr )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   INT32 utilParseAddrList( const CHAR *addrStr,
                            utilAddrContainer &addrArray,
                            const CHAR *itemSeparator,
                            const CHAR *innerSeparator )
   {
      INT32 rc = SDB_OK ;

      if ( !addrStr || !itemSeparator || !innerSeparator ||
           itemSeparator[ 0 ] == 0 || innerSeparator[ 0 ] == 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( addrStr[ 0 ] == 0 )
      {
         goto done ;
      }

      try
      {
         std::vector<std::string> addrs ;
         boost::algorithm::split( addrs, addrStr,
                                  boost::algorithm::is_any_of( itemSeparator ) ) ;

         for ( vector<string>::iterator itr = addrs.begin() ;
               itr != addrs.end() ;
               ++itr )
         {
            vector<string> pair ;
            string tmp = *itr ;
            utilAddrPair addrItem ;
            boost::algorithm::trim( tmp ) ;
            boost::algorithm::split( pair, tmp,
                                     boost::algorithm::is_any_of(
                                           innerSeparator ) ) ;
            if ( pair.size() != 2 )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Address format is invalid: %s", tmp.c_str() ) ;
               goto error ;
            }
            if ( pair.at(0).size() == 0 || pair.at(1).size() == 0 )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Address can not be empty" ) ;
               goto error ;
            }

            rc = addrItem.setHost( pair.at(0).c_str() ) ;
            PD_RC_CHECK( rc, PDERROR, "Set host in item failed[%d]", rc ) ;
            rc = addrItem.setService( pair.at(1).c_str() ) ;
            PD_RC_CHECK( rc, PDERROR, "Set service name in item failed[%d]",
                         rc ) ;
            rc = addrArray.append( addrItem ) ;
            PD_RC_CHECK( rc, PDERROR, "Append address item into list "
                         "failed[%d]", rc ) ;
         }
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
      addrArray.clear() ;
      goto done ;
   }
}
