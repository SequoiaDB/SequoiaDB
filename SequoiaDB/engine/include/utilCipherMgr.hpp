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

   Source File Name = utilCipherMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILCIPHERMGR_H_
#define UTILCIPHERMGR_H_

#include "utilCipherFile.hpp"
#include <map>
#include <vector>

namespace passwd
{
   class _utilCipherMgr : public SDBObject
   {
   public:
      _utilCipherMgr() {}
      ~_utilCipherMgr() {}

      INT32   init( utilCipherFile *file ) ;
      INT32   addUser( const string &userFullName, const string &token,
                       const string &passwd ) ;
      INT32   removeUser( const string &userFullName, INT32 &retCode ) ;
      INT32   getPasswd( const string &filePath,
                         const string &userFullName,
                         const string &token,
                         string &passwd ) ;

   private:
      INT32   _parseLine( const string &line,
                          string &userFullName,
                          string &cipherText ) ;
      INT32   _findCipherText( const string &userFullName,
                               string &cipherText ) ;
      BOOLEAN _isValidHex( const CHAR *hexString ) ;

   private:
      utilCipherFile *_cipherfile ;
      std::map<std::string, std::string> _usersCipher ;
   } ;
   typedef _utilCipherMgr utilCipherMgr ;

}

#endif // UTIL_CIPHER_HPP_