/*******************************************************************************

   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = expUtil.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef EXP_UTIL_HPP_
#define EXP_UTIL_HPP_

#include "oss.hpp"
#include <string>
#include <vector>

namespace exprt
{
   using namespace std ;
   UINT32 RC2ShellRC(INT32 rc) ;
   void cutStr( const string &str, vector<string>& subs, const string &cutBy ) ;
   void trimBoth ( string &str ) ;
   void trimLeft ( string &str ) ;
   void trimRight( string &str ) ;
}

#endif