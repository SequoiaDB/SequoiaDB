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

*******************************************************************************/

#include "core.hpp"
#include "../bson/bson.h"
#include "../client/client.hpp"

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

using namespace std ;
using namespace bson ;
namespace po = boost::program_options;

#define ADD_PARAM_OPTIONS_BEGIN( desc )\
        desc.add_options()
#define ADD_PARAM_OPTIONS_END ;

#define COMMANDS_STRING( a, b ) (string(a) +string( b)).c_str()

#define COMMANDS_OPTIONS \
        ( COMMANDS_STRING("help", ",h"),                          "help" )\
        ( COMMANDS_STRING("addr", ",d"), boost::program_options::value<string>(), "<host:port>" )\
        ( COMMANDS_STRING("usrname", ",u"), boost::program_options::value<string>(), "<usrname>" )\
        ( COMMANDS_STRING("passwd", ",p"), boost::program_options::value<string>(), "<password>" )

#define JUDGE(rc, what) if (SDB_OK!=rc){std::cerr << what << endl ;goto error;}

BOOLEAN parseDst( const string &dst,
                  string &host,
                  UINT16 &port )
{
   vector<string> split ;
   boost::algorithm::split( split, dst,
                            boost::algorithm::is_any_of(":")) ;
   if ( 2 != split.size() )
   {
      std::cerr << "failed to parse dst" << std::endl ;
      return FALSE ;
   }
   host = split.at( 0 ) ;
   try
   {
      port = boost::lexical_cast<UINT16>( split.at( 1 ) ) ;
   }
   catch ( std::exception & e )
   {
      std::cerr << "failed to parse dst" << std::endl ;
      return FALSE ;
   }
   return TRUE ;
}


INT32 start( const CHAR *host,
             UINT16 port,
             const CHAR *usr,
             const CHAR *passwd )
{
   INT32 rc = SDB_OK ;
   sdbclient::sdb conn ;
   rc = conn.connect( host, port, usr, passwd ) ;
   CHAR line[500] ;
   BSONObj obj ;
   JUDGE(rc, "failed to connect to " << host << ":" << port << " rc:" << rc )
   while ( TRUE )
   {
      memset(line, 0, 500) ;
      cout << "sql>" ;
      cin.getline( line, sizeof(line) ) ;
      string sql =  boost::to_lower_copy(boost::trim_copy( string(line) ) );
      if ( "quit" == sql || "q" == sql )
      {
         cout << "bye !" << endl ;
         break ;
      }
      if ( string::npos != sql.find("select") ||
           string::npos != sql.find("list", 0, 4 ) )
      {
         sdbclient::sdbCursor cursor ;
         rc = conn.exec( string(line).c_str(), cursor ) ;
         if ( SDB_OK == rc )
         {
            while ( SDB_OK == cursor.next( obj ) )
            {
               cout << obj.toString() << endl ;
            }
         }
      }
      else if ( string::npos != sql.find("adduser"))
      {
          vector<string> params ;
          boost::algorithm::split( params, line,
                            boost::algorithm::is_any_of(" ")) ;
          if ( 3 != params.size() )
          {
             std::cerr << "create user input like:adduser usrname passwd"
                       << endl ;
             continue ;
          }
          rc = conn.createUsr( params.at(1).c_str(),
                               params.at(2).c_str() ) ;
      }
      else if ( string::npos != sql.find("removeuser"))
      {
         vector<string> params ;
         boost::algorithm::split( params, line,
                            boost::algorithm::is_any_of(" ")) ;
         if ( 3 != params.size() )
         {
            std::cerr << "remove user input like:removeuser usrname passwd"
                      << endl ;
            continue ;
         }
         rc = conn.removeUsr( params.at(1).c_str(),
                              params.at(2).c_str() ) ;
      }
      else
      {
         rc = conn.execUpdate( string(line).c_str() ) ;
      }
      cout << "rc>" << rc << endl ;
   }
done:
   conn.disconnect() ;
   return rc ;
error:
   goto done ;
}

INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   po::options_description desc ( "Command options" ) ;
   po::variables_map vm ;
   ADD_PARAM_OPTIONS_BEGIN( desc )
      COMMANDS_OPTIONS
   ADD_PARAM_OPTIONS_END
   try
   {
      po::store ( po::parse_command_line ( argc,
                                           argv, desc), vm ) ;
      po::notify ( vm ) ;
   }
   catch ( po::unknown_option &e )
   {
      std::cerr <<  "Unknown argument: "
                << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG;
      goto error;
   }
   catch ( po::invalid_option_value &e )
   {
      std::cerr <<  "Invalid argument: "
                << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG;
      goto error;
   }
   catch( po::error &e )
   {
      std::cerr << e.what () << std::endl ;
      rc = SDB_INVALIDARG;
      goto error;
   }

   if ( vm.count( "help" ) )
   {
      cout << desc << endl ;
      goto done ;
   }

   if ( !vm.count( "addr" ) )
   {
      std::cerr << "missing addr" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   {
   string host ;
   UINT16 port ;
   string usr ;
   string passwd ;
   if ( vm.count("usrname") )
   {
      usr = vm["usrname"].as<string>() ;
      if ( !vm.count("passwd"))
      {
         std::cerr << "usrname and passwd must exist" << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      passwd = vm["passwd"].as<string>() ;
   }
   if ( !parseDst( vm["addr"].as<string>(),
                   host, port) )
   {
      goto error ;
   }

   rc = start( host.c_str(), port,
               usr.c_str(), passwd.c_str() ) ;
   }
done:
   return rc;
error:
   goto done ;
}
