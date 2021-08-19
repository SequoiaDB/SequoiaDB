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

   Source File Name = ossPath.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OSSPATH_HPP__
#define OSSPATH_HPP__

#include <string>
#include <map>
#include <vector>
#include "oss.h"

using namespace std ;

INT32 ossEnumFiles( const string &dirPath,
                    multimap<string, string> &mapFiles,
                    const CHAR *filter = NULL,
                    UINT32 deep = 1 ) ;

INT32 ossEnumFiles2( const string &dirPath,
                     multimap<string, string> &mapFiles,
                     const CHAR *filter = NULL,
                     OSS_MATCH_TYPE type = OSS_MATCH_ALL,
                     UINT32 deep = 1 ) ;

INT32 ossEnumSubDirs( const string &dirPath,
                      vector< string > &subDirs,
                      UINT32 deep = 1 ) ;


#endif // OSSPATH_HPP__

