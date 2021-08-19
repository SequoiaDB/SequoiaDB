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

   Source File Name = utilCipherMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossErr.h"
#include "ossUtil.hpp"
#include "utilCipher.hpp"
#include "utilCipherMgr.hpp"
#include "msgDef.hpp"
#include "openssl/des.h"
#include "openssl/sha.h"
#include <iostream>
#include <sstream>

#define UTIL_CLEARTEXT_MAX_LENGTH   1024

namespace passwd
{
   INT32 _utilCipherMgr::_parseLine( const string &line,
                                     string &userFullName,
                                     string &cipherText )
   {
      INT32 rc = SDB_OK ;

      string::size_type offset = line.find( ":" ) ;
      if ( string::npos == offset || 0 == offset )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "line [%s] is in wrong foramt. ", line.c_str() ) ;
         goto error ;
      }

      userFullName = line.substr( 0, offset ) ;
      cipherText = line.substr( offset + 1, line.length() - offset - 1 ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCipherMgr::_findCipherText( const string &userFullName,
                                          string &cipherText )
   {
      INT32                           rc = SDB_OK ;
      map<string, string>::iterator   itor ;
      map<string, string>::iterator   found ;
      string                          userShortName ;
      string                          clusterName ;
      BOOLEAN                         fullMatched  = FALSE ;
      UINT32                          partMatchNum = 0 ;

      userShortName = utilGetUserShortNameFromUserFullName( userFullName,
                                                            &clusterName ) ;

      for ( itor = _usersCipher.begin(); itor != _usersCipher.end(); itor++ )
      {
         string tmpUser ;
         string tmpCluster ;

         tmpUser = utilGetUserShortNameFromUserFullName( itor->first,
                                                         &tmpCluster ) ;

         if ( userShortName == tmpUser && clusterName == tmpCluster )
         {
            fullMatched  = TRUE ;
            partMatchNum = 0 ;
            cipherText = itor->second ;
            break ;
         }
         else if ( clusterName.empty() )
         {
            if ( userShortName == tmpUser )
            {
               cipherText = itor->second ;
               ++partMatchNum ;
            }
         }
      }

      if ( partMatchNum > 1 )
      {
         PD_LOG ( PDERROR, "Ambiguous user name, try providing cluster name." ) ;
         std::cerr << "Ambiguous user name, try providing cluster name."
                   << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !fullMatched && 0 == partMatchNum )
      {
         PD_LOG ( PDWARNING, "No corresponding user information." ) ;
         std::cerr << "No corresponding user information." << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _utilCipherMgr::_isValidHex( const CHAR* hexString )
   {
      UINT32 len = ossStrlen( hexString ) ;

      for( UINT32 i = 0; i < len; ++i )
      {
         CHAR tmp = hexString[i] ;
         if ( ( tmp < '0' && tmp > '9' ) && ( tmp < 'a' && tmp > 'f' ) &&
              ( tmp < 'A' && tmp > 'F' ) )
         {
            return FALSE ;
         }
      }
      return TRUE ;
   }

   INT32 _utilCipherMgr::init( utilCipherFile *file )
   {
      INT32  rc          = SDB_OK ;
      INT64  begin       = 0 ;
      INT64  lineLen     = 0 ;
      INT64  contentLen  = 0 ;
      CHAR*  fileContent = NULL ;
      string userFullName ;
      string cipherText ;

      _cipherfile = file ;
      rc = _cipherfile->read( &fileContent, contentLen ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to read cipher file[%s], rc: %d",
                  _cipherfile->getFilePath(), rc ) ;
         goto error ;
      }

      while( begin < contentLen )
      {
         const CHAR* find = ossStrstr( fileContent + begin, OSS_NEWLINE ) ;
         if ( NULL != find )
         {
            lineLen = find - ( fileContent + begin ) ;
            string line( fileContent + begin, lineLen ) ;
            begin += lineLen + ossStrlen( OSS_NEWLINE ) ;

            if ( SDB_OK == _parseLine( line, userFullName, cipherText ) )
            {
               _usersCipher[userFullName] = cipherText ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Invalid cipher file[%s]",
                     _cipherfile->getFilePath() ) ;
            goto error ;
         }
      }

   done:
      if( fileContent )
      {
         SDB_OSS_FREE( fileContent ) ;
         fileContent = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCipherMgr::addUser( const string &userFullName,
                                  const string &token,
                                  const string &passwd )
   {
      INT32  rc = SDB_OK ;
      string fileContent ;
      map<string,string>::iterator it ;
      CHAR cipherText[ UTIL_CLEARTEXT_MAX_LENGTH + 1 ] = { '\0' } ;

      if ( userFullName.empty() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "The username can't be empty, rc: %d", rc ) ;
         goto error ;
      }

      if ( SDB_MAX_USERNAME_LENGTH < userFullName.size() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "The username is too long. Its maximum "
                  "length is %d, rc: %d", SDB_MAX_USERNAME_LENGTH, rc ) ;
         goto error ;
      }

      if ( SDB_MAX_PASSWORD_LENGTH < passwd.size() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "The password is too long. Its maximum "
                  "length is %d, rc: %d", SDB_MAX_PASSWORD_LENGTH, rc ) ;
         goto error ;
      }

      if ( SDB_MAX_TOKEN_LENGTH < token.size() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "The token is too long. Its maximum "
                  "length is %d, rc: %d", SDB_MAX_TOKEN_LENGTH, rc ) ;
         goto error ;
      }

      if ( !passwd.empty() )
      {
         rc = utilCipherEncrypt( passwd.c_str(), token.c_str(),
                                 cipherText, UTIL_CLEARTEXT_MAX_LENGTH + 1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to encrypt password, rc: %d", rc ) ;
            goto error ;
         }
         _usersCipher[userFullName] = cipherText ;
      }
      else
      {
         _usersCipher[userFullName] = "\0" ;
      }

      it = _usersCipher.begin() ;
      for ( ; it != _usersCipher.end(); it++ )
      {
         fileContent += ( it->first + ":" + it->second + OSS_NEWLINE ) ;
      }

      rc = _cipherfile->write( fileContent ) ;
      if( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to write cipher text to the cipher file,"
                  " rc: ", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCipherMgr::removeUser( const string &userFullName,
                                     INT32 &retCode )
   {
      INT32 rc = SDB_OK ;

      map<string, string>::iterator it = _usersCipher.find( userFullName ) ;
      if ( it != _usersCipher.end() )
      {
         string fileContent ;

         _usersCipher.erase( it ) ;

         for( it = _usersCipher.begin(); it != _usersCipher.end(); it++ )
         {
            fileContent += ( it->first + ":" + it->second + OSS_NEWLINE ) ;
         }

         rc = _cipherfile->write( fileContent ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else
      {
         PD_LOG ( PDWARNING, "No user[%s] password in file[%s]",
                  userFullName.c_str(), _cipherfile->getFilePath() ) ;
         std::cout << "No user[" << userFullName.c_str()
                   << "] password in file["
                   << _cipherfile->getFilePath() << "]"
                   << std::endl ;
         retCode = SDB_INVALIDARG ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilCipherMgr::getPasswd( const string &filePath,
                                    const string &userFullName,
                                    const string &token,
                                    string &passwd )
   {
      INT32  rc = SDB_OK ;
      string cipherText ;
      CHAR   clearText[SDB_MAX_PASSWORD_LENGTH + 1] = { '\0' } ;

      rc = _findCipherText( userFullName, cipherText ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( !_isValidHex( cipherText.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Invalid cipher file[%s]. The cipher text in the "
                  "file must be a hexadecimal character, rc: %d",
                  filePath.c_str(), rc ) ;
         goto error ;
      }

      rc = utilCipherDecrypt( cipherText.c_str(), token.c_str(), clearText ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to decrypt user[%s]'s password",
                  userFullName.c_str() ) ;
         goto error ;
      }
      passwd = clearText ;

   done:
      return rc ;
   error:
      goto done ;
   }

}
