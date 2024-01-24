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

   Source File Name = utilPasswdTool.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILPASSWDTOOL_H_
#define UTILPASSWDTOOL_H_

#include "utilCipherMgr.hpp"

class _utilPasswordTool : public SDBObject
{
public:
   _utilPasswordTool() {}
   ~_utilPasswordTool() {}
   static BOOLEAN interactivePasswdInput( string &passwd ) ;
   INT32  getPasswdByCipherFile( const string &userFullName,
                                 const string &token,
                                 string &filePath,
                                 string &password ) ;

private:
   utilCipherMgr    _cipherMgr ;
   utilCipherFile   _cipherfile ;
} ;
typedef _utilPasswordTool utilPasswordTool ;

#endif