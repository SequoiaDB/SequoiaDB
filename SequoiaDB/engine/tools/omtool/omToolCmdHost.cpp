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

   Source File Name = omToolCmdHost.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "omToolCmdHost.hpp"
#include "utilStr.hpp"
#include "ossIO.hpp"
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "boost/filesystem.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

namespace fs = boost::filesystem ;

namespace omTool
{
   enum HOST_LINE_TYPE
   {
      LINE_HOST = 1,
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

   INT32 parseHostsFile( VEC_HOST_ITEM &vecItems, string &err ) ;
   INT32 writeHostsFile( VEC_HOST_ITEM &vecItems, string &err ) ;
   INT32 extractHosts( const CHAR *buf, VEC_HOST_ITEM &vecItems ) ;

   IMPLEMENT_OMTOOL_CMD_AUTO_REGISTER( omToolAddHost ) ;

   omToolAddHost::omToolAddHost()
   {
   }

   omToolAddHost::~omToolAddHost()
   {
   }

   INT32 omToolAddHost::doCommand()
   {
      INT32 rc = SDB_OK ;
      string hostName = _options->hostname() ;
      string ip       = _options->ip() ;
      string err ;
      VEC_HOST_ITEM vecItems ;
      VEC_HOST_ITEM::iterator it ;

      if ( !engine::isValidIPV4( ip.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         cout << "error ip format: " << ip << endl ;
         goto error ;
      }

      if ( hostName.empty() )
      {
         rc = SDB_INVALIDARG ;
         cout << "invalid hostname" << endl ;
         goto error ;
      }
      
      rc = parseHostsFile( vecItems, err ) ;
      if ( rc )
      {
         cout << err << endl ;
         goto error ;
      }

      {
         BOOLEAN needAdd = TRUE ;

         it = vecItems.begin() ;

         while ( it != vecItems.end() )
         {
            hostItem &item = *it ;

            if( LINE_HOST == item._lineType && hostName == item._host )
            {
               item._ip = ip ;
               needAdd  = FALSE ;
            }

            ++it ;
         }

         if ( needAdd )
         {
            hostItem info ;

            info._lineType = LINE_HOST ;
            info._host     = hostName ;
            info._ip       = ip ;

            vecItems.push_back( info ) ;
         }
      }

      // write
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

   IMPLEMENT_OMTOOL_CMD_AUTO_REGISTER( omToolDelHost ) ;

   omToolDelHost::omToolDelHost()
   {
   }

   omToolDelHost::~omToolDelHost()
   {
   }

   INT32 omToolDelHost::doCommand()
   {
      INT32 rc = SDB_OK ;
      string hostName = _options->hostname() ;
      string err ;
      VEC_HOST_ITEM vecItems ;
      VEC_HOST_ITEM::iterator iter ;

      if ( hostName.empty() )
      {
         rc = SDB_INVALIDARG ;
         cout << "invalid hostname" << endl ;
         goto error ;
      }

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

      // write
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

      // read file
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

      // remove last empty
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
      const CHAR *pFilePath = NULL ;
      std::string tmpFile ;
      OSSFILE file ;
      stringstream ss ;
      fs::perms permissions = fs::no_perms ;

      // 1. first create the file if not exist
      pFilePath = HOSTS_FILE ;
      if ( SDB_OK != ossAccess( pFilePath ) )
      {
         rc = ossOpen ( pFilePath, OSS_READWRITE|OSS_SHAREWRITE|OSS_REPLACE,
                        OSS_RU|OSS_WU|OSS_RG|OSS_RO, file ) ;
         if ( rc )
         {
            ss << "open file[" << pFilePath << "] failed: " << rc ;
            goto error ;
         }
         ossClose( file ) ;
      }
      // get the permissions for hosts file
      try
      {
         fs::path hostsFilePath ( pFilePath ) ;
         fs::file_status fileStatus = fs::status( hostsFilePath ) ;
         if ( fileStatus.type() == fs::file_not_found )
         {
            rc = SDB_FNE ;
            ss << "get file[" << pFilePath << "]'s status failed, rc = " << rc ;
            goto error ;
         }
         else if ( fileStatus.type() == fs::type_unknown )
         {
            rc = SDB_IO ;
            ss << "get file[" << pFilePath 
               << "]'s status failed, the attributes cannot be determind" ;
            goto error ;
         }
         permissions = fileStatus.permissions() ;
      }
      catch ( fs::filesystem_error& e )
      {
         if ( e.code() == boost::system::errc::permission_denied ||
              e.code() == boost::system::errc::operation_not_permitted )
         {
            rc = SDB_PERM ;
            ss << "no permission to access file[" << pFilePath << "], rc = " 
               << rc ;
         }
         else
         {
            rc = SDB_IO ;
            ss << "get file[" << pFilePath << "]'s permission failed, errno: "
               << e.code().value() << ", rc = " << rc ;
         }
         goto error ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         ss << "get file[" << pFilePath << "]'s permission failed, error: "
            << e.what() << ", rc = " << rc ;
         goto error ;
      }

      // 2. remove the tmp file
      tmpFile = tmpFile + HOSTS_FILE + ".tmp" ;
      pFilePath = tmpFile.c_str() ;
      if ( SDB_OK == ossAccess( pFilePath ) )
      {
         ossDelete( pFilePath ) ;
      }

      // 3. Create the tmp file and change it's permissions
      rc = ossOpen ( pFilePath, OSS_READWRITE|OSS_SHAREWRITE|OSS_REPLACE,
                     OSS_RU|OSS_WU|OSS_RG|OSS_RO, file ) ;
      if ( rc )
      {
         ss << "open file[" << pFilePath << "] failed: " << rc ;
         goto error ;
      }
      /// set permission
      try
      {      
         fs::path tmpHostsFilePath ( pFilePath ) ;
         fs::permissions( tmpHostsFilePath, permissions ) ;
      }
      catch ( fs::filesystem_error& e )
      {
         if ( e.code() == boost::system::errc::permission_denied ||
              e.code() == boost::system::errc::operation_not_permitted )
         {
            rc = SDB_PERM ;
            ss << "no permission to access file[" << pFilePath << "], rc = " 
               << rc ;
         }
         else
         {
            rc = SDB_IO ;
            ss << "set file[" << pFilePath << "]'s permission failed, error: "
               << e.what() << ", rc = " << rc ;
         }
         goto error ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         ss << "set file[" << pFilePath << "]'s permission failed, error: "
            << e.what() << ", rc = " << rc ;
         goto error ;
      }

      // 3. write data
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
               ss << "write context[" << text << "] to file[" << pFilePath
                  << "] failed: " << rc ;
               goto error ;
            }
         }
      }

      ossClose( file ) ;

      // 4. backup the file
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

      // 5. commit the file
      rc = ossRenamePath( pFilePath, HOSTS_FILE ) ;
      if ( SDB_OK != rc )
      {
         ss << "commit file:" << pFilePath << " to file:" << HOSTS_FILE << 
               " failed" ;
         goto error ;
      }

   done:
      return rc ;
   error:
      err = ss.str() ;
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
            // unreconigze format, ignore
            item._ip = *itr ;
            vecItems.push_back( item ) ;
            continue ;
         }

         if ( !engine::isValidIPV4( columns.at( 0 ).c_str() ) )
         {
            // unreconigze format, ignore
            item._ip = *itr ;
            vecItems.push_back( item ) ;
            continue ;
         }

         /// xxx.xxx.xxx.xxx xxxx
         /// xxx.xxx.xxx.xxx xxxx.xxxx xxxx
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

}