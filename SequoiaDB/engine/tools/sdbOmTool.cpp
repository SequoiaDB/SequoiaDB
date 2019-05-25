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

   Source File Name = sdbOmTool.cpp

   Descriptive Name = sdbOmTool Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbstop,
   which is used to stop SequoiaDB engine.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/27/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "ossUtil.hpp"
#include "ossProc.hpp"
#include "ossIO.hpp"
#include "ossMem.hpp"
#include "ossPath.hpp"
#include "utilNodeOpr.hpp"
#include "utilCommon.hpp"
#include "ossVer.h"
#include "utilParam.hpp"
#include "utilStr.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


using namespace std ;

namespace engine
{
   enum HOST_LINE_TYPE
   {
      LINE_HOST         = 1,
      LINE_UNKNONW,
   } ;

   struct hostItem
   {
      INT32    _lineType ;
      string   _ip ;
      string   _com ;
      string   _host ;

      hostItem()
      {
         _lineType = LINE_UNKNONW ;
      }

      string toString() const
      {
         if ( LINE_UNKNONW == _lineType )
         {
            return _ip ;
         }
         string space = "    " ;
         if ( _com.empty() )
         {
            return _ip + space + _host ;
         }

         return _ip + space + _com + space + _host ;
      }
   } ;

   typedef vector< hostItem >    VEC_HOST_ITEM ;

   /*
      OPTION DEFINE
   */
   #define OMTOOL_OPTION_HELP             "help"
   #define OMTOOL_OPTION_VERSION          "version"
   #define OMTOOL_OPTION_MODE             "mode"
   #define OMTOOL_OPTION_HOSTNAME         "hostname"
   #define OMTOOL_OPTION_IP               "ip"


   #define COMMANDS_ADD_PARAM_OPTIONS_BEGIN( desc )  desc.add_options()
   #define COMMANDS_ADD_PARAM_OPTIONS_END ;
   #define COMMANDS_STRING( a, b ) (string(a) +string( b)).c_str()

   #define COMMANDS_OPTIONS \
       ( COMMANDS_STRING( OMTOOL_OPTION_HELP, ",h" ), "help" ) \
       ( COMMANDS_STRING( OMTOOL_OPTION_VERSION, ",v" ), "version" ) \
       ( COMMANDS_STRING( OMTOOL_OPTION_MODE, ",m" ), po::value<string>(), "mode type: addhost/delhost" ) \
       ( COMMANDS_STRING( OMTOOL_OPTION_HOSTNAME, "" ), po::value<string>(), "hostname" ) \
       ( COMMANDS_STRING( OMTOOL_OPTION_IP, "" ), po::value<string>(), "ip" )


   #define OM_TOOL_MODE_ADD_STR        "addhost"
   #define OM_TOOL_MODE_DEL_STR        "delhost"

   void init ( po::options_description &desc )
   {
      COMMANDS_ADD_PARAM_OPTIONS_BEGIN ( desc )
         COMMANDS_OPTIONS
      COMMANDS_ADD_PARAM_OPTIONS_END
   }

   void displayArg ( po::options_description &desc )
   {
      std::cout << desc << std::endl ;
   }

   INT32 resolveOTArgs ( po::options_description &desc,
                         INT32 argc, CHAR **argv, string &mode, 
                         string &hostName, string &ip )
   {
      INT32 rc = SDB_OK ;
      po::variables_map vm ;

      rc = utilReadCommandLine( argc, argv,  desc, vm, FALSE ) ;
      if ( rc )
      {
         std::cout << "Read command line failed: " << rc << endl ;
         goto error ;
      }

      if ( vm.count ( OMTOOL_OPTION_HELP ) )
      {
         displayArg ( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }

      if ( vm.count( OMTOOL_OPTION_VERSION ) )
      {
         ossPrintVersion( "om tool Version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }

      if ( vm.count ( OMTOOL_OPTION_MODE ) )
      {
         mode = vm[OMTOOL_OPTION_MODE].as<string>() ;
         if ( mode != OM_TOOL_MODE_ADD_STR && mode != OM_TOOL_MODE_DEL_STR )
         {
            std::cout << "mode invalid" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( mode == OM_TOOL_MODE_ADD_STR )
      {
         if ( !vm.count( OMTOOL_OPTION_HOSTNAME ) )
         {
            cout << "miss hostname in mode:" << mode << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         hostName = vm[ OMTOOL_OPTION_HOSTNAME ].as<string>() ;

         if ( !vm.count( OMTOOL_OPTION_IP ) )
         {
            cout << "miss ip in mode:" << mode << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         ip = vm[ OMTOOL_OPTION_IP ].as<string>() ;
      }
      else if ( mode == OM_TOOL_MODE_DEL_STR )
      {
         if ( !vm.count( OMTOOL_OPTION_HOSTNAME ) )
         {
            cout << "miss hostname in mode:" << mode << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         hostName = vm[ OMTOOL_OPTION_HOSTNAME ].as<string>() ;
      }
      else
      {
         cout << "unreconigzed mode:" << mode << endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 extractHosts( const CHAR *buf, VEC_HOST_ITEM &vecItems )
   {
      vector<string> splited ;
      boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      if ( splited.empty() )
      {
         goto done ;
      }

      for ( vector<string>::iterator itr = splited.begin() ;
            itr != splited.end() ; itr++ )
      {
         hostItem item ;
         if ( itr->empty() )
         {
            vecItems.push_back( item ) ;
            continue ;
         }

         boost::algorithm::trim( *itr ) ;
         vector<string> columns ;
         boost::algorithm::split( columns, *itr, boost::is_any_of("\t ") ) ;

         for ( vector<string>::iterator itr2 = columns.begin() ;
               itr2 != columns.end() ; )
         {
            if ( itr2->empty() )
            {
               itr2 = columns.erase( itr2 ) ;
            }
            else
            {
               ++itr2 ;
            }
         }

         if ( 2 != columns.size() && 3 != columns.size() )
         {
            item._ip = *itr ;
            vecItems.push_back( item ) ;
            continue ;
         }

         if ( !isValidIPV4( columns.at( 0 ).c_str() ) )
         {
            item._ip = *itr ;
            vecItems.push_back( item ) ;
            continue ;
         }

         item._ip = columns[ 0 ] ;
         if ( columns.size() == 3 )
         {
            item._com = columns[ 1 ] ;
            item._host = columns[ 2 ] ;
         }
         else
         {
            item._host = columns[ 1 ] ;
         }

         item._lineType = LINE_HOST ;
         vecItems.push_back( item ) ;
      }

   done:
      return SDB_OK ;
   }

#if defined( _LINUX )
   #define HOSTS_FILE      "/etc/hosts"
#else
   #define HOSTS_FILE      "C:\\Windows\\System32\\drivers\\etc\\hosts"
#endif // _LINUX

   INT32 parseHostsFile( VEC_HOST_ITEM &vecItems, string &err )
   {
      INT32 rc = SDB_OK ;
      OSSFILE file ;
      stringstream ss ;
      BOOLEAN isOpen = FALSE ;
      INT64 fileSize = 0 ;
      CHAR *pBuff = NULL ;
      INT64 hasRead = 0 ;

      rc = ossGetFileSizeByName( HOSTS_FILE, &fileSize ) ;
      if ( rc )
      {
         ss << "get file[" << HOSTS_FILE << "] size failed: " << rc ;
         goto error ;
      }
      pBuff = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
      if ( !pBuff )
      {
         ss << "alloc memory[" << fileSize << "] failed" ;
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( pBuff, 0, fileSize + 1 ) ;

      rc = ossOpen( HOSTS_FILE, OSS_READONLY|OSS_SHAREREAD, 0,
                    file ) ;
      if ( rc )
      {
         ss << "open file[" << HOSTS_FILE << "] failed: " << rc ;
         goto error ;
      }
      isOpen = TRUE ;

      rc = ossReadN( &file, fileSize, pBuff, hasRead ) ;
      if ( rc )
      {
         ss << "read file[" << HOSTS_FILE << "] failed: " << rc ;
         goto error ;
      }
      ossClose( file ) ;
      isOpen = FALSE ;

      rc = extractHosts( pBuff, vecItems ) ;
      if ( rc )
      {
         ss << "extract hosts failed: " << rc ;
         goto error ;
      }

      if ( vecItems.size() > 0 )
      {
         VEC_HOST_ITEM::iterator itr = vecItems.end() - 1 ;
         hostItem &info = *itr ;
         if ( info.toString().empty() )
         {
            vecItems.erase( itr ) ;
         }
      }

   done:
      if ( isOpen )
      {
         ossClose( file ) ;
      }
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      return rc ;
   error:
      err = ss.str() ;
      goto done ;
   }

   INT32 writeHostsFile( VEC_HOST_ITEM &vecItems, string &err )
   {
      INT32 rc = SDB_OK ;
      std::string tmpFile = HOSTS_FILE ;
      tmpFile += ".tmp" ;
      OSSFILE file ;
      stringstream ss ;

      if ( SDB_OK != ossAccess( HOSTS_FILE ) )
      {
         rc = ossOpen ( HOSTS_FILE, OSS_READWRITE|OSS_SHAREWRITE|OSS_REPLACE,
                     OSS_RU|OSS_WU|OSS_RG|OSS_RO, file ) ;
         if ( rc )
         {
            ss << "open file[" <<  HOSTS_FILE << "] failed: " << rc ;
            goto error ;
         }
      }

      if ( SDB_OK == ossAccess( tmpFile.c_str() ) )
      {
         ossDelete( tmpFile.c_str() ) ;
      }

      rc = ossOpen ( tmpFile.c_str(), OSS_READWRITE|OSS_SHAREWRITE|OSS_REPLACE,
                     OSS_RU|OSS_WU|OSS_RG|OSS_RO, file ) ;
      if ( rc )
      {
         ss << "open file[" <<  tmpFile.c_str() << "] failed: " << rc ;
         goto error ;
      }

      {
         VEC_HOST_ITEM::iterator it = vecItems.begin() ;
         UINT32 count = 0 ;
         while ( it != vecItems.end() )
         {
            ++count ;
            hostItem &item = *it ;
            ++it ;
            string text = item.toString() ;
            if ( !text.empty() || count < vecItems.size() )
            {
               text += OSS_NEWLINE ;
            }
            rc = ossWriteN( &file, text.c_str(), text.length() ) ;
            if ( rc )
            {
               ossClose( file ) ;
               ss << "write context[" << text << "] to file[" << tmpFile.c_str()
                  << "] failed: " << rc ;
               goto error ;
            }
         }
      }

      ossClose( file ) ;

      {
         string backupFile = string( HOSTS_FILE ) + ".bak" ;
         if ( SDB_OK == ossAccess( backupFile.c_str() ) )
         {
            ossDelete( backupFile.c_str() ) ;
         }

         rc = ossRenamePath( HOSTS_FILE, backupFile.c_str() ) ;
         if ( SDB_OK != rc )
         {
            ss << "backup file:" << HOSTS_FILE << " to file:" << backupFile 
               << " failed" ;
            goto error ;
         }
      }

      rc = ossRenamePath( tmpFile.c_str(), HOSTS_FILE ) ;
      if ( SDB_OK != rc )
      {
         ss << "commit file:" << tmpFile << " to file:" << HOSTS_FILE << 
            " failed" ;
         goto error ;
      }

   done:
      return rc ;
   error:
      err = ss.str() ;
      goto done ;
   }


   INT32 addHost( const string &hostName, const string &ip )
   {
      BOOLEAN needAdd ;
      VEC_HOST_ITEM::iterator it ;

      VEC_HOST_ITEM vecItems ;
      string err ;
      INT32 rc = SDB_OK ;

      if ( !isValidIPV4( ip.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         err = "error ip format:" + ip ;
         cout << err << endl ;
         goto error ;
      }
      
      rc = parseHostsFile( vecItems, err ) ;
      if ( SDB_OK != rc )
      {
         cout << err << endl ;
         goto error ;
      }

      it      = vecItems.begin() ;
      needAdd = TRUE ;
      while ( it != vecItems.end() )
      {
         hostItem &item = *it ;
         ++it ;
         if( item._lineType == LINE_HOST && hostName == item._host )
         {
            item._ip = ip ;
            needAdd  = FALSE ;
         }
      }

      if ( needAdd )
      {
         hostItem info ;
         info._lineType = LINE_HOST ;
         info._host     = hostName ;
         info._ip       = ip ;
         vecItems.push_back( info ) ;
      }

      rc = writeHostsFile( vecItems, err ) ;
      if ( rc )
      {
         cout << err << endl ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 delHost( const string &hostName )
   {
      VEC_HOST_ITEM::iterator iter ;
      VEC_HOST_ITEM vecItems ;
      string err ;
      INT32 rc = SDB_OK ;
      rc = parseHostsFile( vecItems, err ) ;
      if ( SDB_OK != rc )
      {
         cout << err << endl ;
         goto error ;
      }

      iter = vecItems.begin() ;
      while ( iter != vecItems.end() )
      {
         hostItem &item = *iter ;
         if( item._lineType == LINE_HOST && hostName == item._host )
         {
            iter = vecItems.erase( iter ) ;
         }
         else
         {
            iter++ ;
         }
      }

      rc = writeHostsFile( vecItems, err ) ;
      if ( rc )
      {
         cout << err << endl ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 doCommand( const string &mode, const string &hostName, 
                    const string &ip )
   {
      INT32 rc = SDB_OK ;
      if ( mode == OM_TOOL_MODE_ADD_STR )
      {
         rc = addHost( hostName, ip ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = delHost( hostName ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mainEntry ( INT32 argc, CHAR **argv )
   {
      INT32 rc  = SDB_OK ;
      string ip = "" ;
      string hostName = "" ;
      string mode = "" ;

      po::options_description desc ( "Command options" ) ;
      init ( desc ) ;

      rc = resolveOTArgs ( desc, argc, argv, mode, hostName, ip ) ;
      if( rc )
      {
         if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
         {
            rc = SDB_OK ;
            goto done ;
         }

         displayArg ( desc ) ;
         goto error ;
      }

      rc = doCommand( mode, hostName, ip ) ;

   done :
      return rc ;
   error :
      goto done ;
   }
}

INT32 main ( INT32 argc, CHAR **argv )
{
   return engine::mainEntry( argc, argv ) ;
}



