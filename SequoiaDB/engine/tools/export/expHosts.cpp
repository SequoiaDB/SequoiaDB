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

   Source File Name = impHosts.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#include "expHosts.hpp"
#include "pd.hpp"
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

namespace exprt
{
   static bool hostEqual( const Host& host1, const Host& host2 )
   {
      string hostname1 = host1.hostname ;
      string hostname2 = host2.hostname ;

      if ( "127.0.0.1" == hostname1 )
      {
         hostname1 = "localhost" ;
      }

      if ( "127.0.0.1" == hostname2 )
      {
         hostname2 = "localhost" ;
      }

      if ( hostname1 != hostname2 )
      {
         return false ;
      }

      return host1.svcname == host2.svcname ;
   }

   class HostComparer
   {
   public:
      bool operator()( const Host& host1, const Host& host2 )
      {
         string hostname1 = host1.hostname;
         string hostname2 = host2.hostname;

         if ( "127.0.0.1" == hostname1 )
         {
            hostname1 = "localhost" ;
         }

         if ( "127.0.0.1" == hostname2 )
         {
            hostname2 = "localhost" ;
         }

         if ( hostname1 < hostname2 )
         {
            return true ;
         }

         return host1.svcname < host2.svcname ;
      }
   } ;

   typedef boost::tokenizer<boost::char_separator<char> > CustomTokenizer ;
   INT32 Hosts::parse( const string& hostList, vector<Host>& hosts )
   {
      INT32 rc = SDB_OK ;
      boost::char_separator<char> hostSep( "," ) ;
      boost::char_separator<char> nameSep( ":" ) ;
      CustomTokenizer hostTok( hostList, hostSep ) ;
      CustomTokenizer::iterator it ;

      hosts.clear() ;

      for ( it = hostTok.begin(); it != hostTok.end(); ++it )
      {
         string host = *it ;

         host = boost::algorithm::trim_copy_if( host, boost::is_space() ) ;
         if ( host.empty() )
         {
            // ignore empty string or white space
            continue;
         }

         {
            CustomTokenizer nameTok( host, nameSep ) ;
            CustomTokenizer::iterator nameIt = nameTok.begin() ;
            string hostname ;
            string svcname ;

            // first is hostname
            if ( nameIt == nameTok.end() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid host [%s]", host.c_str() ) ;
               goto error ;
            }

            hostname = *nameIt ;
            hostname = boost::algorithm::trim_copy_if( hostname,
                                                       boost::is_space() ) ;
            if ( hostname.empty() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Empty hostname of host [%s]", host.c_str() ) ;
               goto error ;
            }

            // second is svcname
            *nameIt++ ;
            if ( nameIt == nameTok.end() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid host [%s]", host.c_str() ) ;
               goto error ;
            }

            svcname = *nameIt ;
            svcname = boost::algorithm::trim_copy_if( svcname,
                                                      boost::is_space() ) ;
            if ( svcname.empty() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Empty svcname of host [%s]", host.c_str() ) ;
               goto error ;
            }

            // error if still have string
            *nameIt++ ;
            if ( nameIt != nameTok.end() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid host [%s]", host.c_str() ) ;
               goto error ;
            }

            {
               Host h;

               h.hostname = hostname ;
               h.svcname = svcname ;
               hosts.push_back( h ) ;
            }
         }
      }

      if ( hosts.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "There is no host" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      hosts.clear() ;
      goto done ;
   }

   void Hosts::removeDuplicate( vector<Host>& hosts )
   {
      if ( hosts.empty() )
      {
         return ;
      }

      std::sort( hosts.begin(), hosts.end(), HostComparer() ) ;
      hosts.erase( std::unique( hosts.begin(), hosts.end(), hostEqual ),
                   hosts.end() ) ;
   }
}
