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

   Source File Name = utilCipherFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILCIPHERFILE_H_
#define UTILCIPHERFILE_H_

#include "ossFile.hpp"

namespace passwd
{
   enum CIPHER_FILE_ROLE
   {
      R_ROLE = 0,
      W_ROLE
   } ;

   class _utilCipherFile
   {
   public:
      _utilCipherFile() : _isOpen( FALSE ) {}
      ~_utilCipherFile() ;

      INT32       init( string &filePath, UINT32 role ) ;
      INT32       read( CHAR **fileContent, INT64 &contentLen ) ;
      INT32       write( const string &fileContent ) ;
      const CHAR* getFilePath(){ return _filePath.c_str() ; }

   private:
      BOOLEAN _isOpen ;
      string  _filePath ;
      engine::ossFile _file ;
   } ;
   typedef _utilCipherFile utilCipherFile ;

   INT32  utilBuildDefaultCipherFilePath( string &cipherFilePath ) ;
   string utilGetUserShortNameFromUserFullName( const string &userFullName,
                                                string *clusterName = NULL ) ;
}

#endif
