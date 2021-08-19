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

   Source File Name = sptWords.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/4/2017    TZB  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPTWORDS_HPP__
#define SPTWORDS_HPP__

#include "core.hpp"

#include <stdio.h>
#include <vector>
#include <string>

#if defined ( _WINDOWS )
#include <stdint.h>
#endif

using std::vector ;
using std::string ;

namespace engine {

#if defined ( _WINDOWS )
   INT32 sptGBKToUTF8( const std::string& strGBK, std::string &strUTF8 ) ;
   INT32 sptUTF8ToGBK( const std::string &strUTF8, std::string &strGBK ) ;
#endif
   void sdbSplitWords( const string &text, 
                       INT32 wordNum, vector<string> &output ) ;
}
#endif // SPTWORDS_HPP__
