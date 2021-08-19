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

   Source File Name = expUtil.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#include "expUtil.hpp"
#include "utilCommon.hpp"

namespace exprt
{
   #define EXP_SPACE_STR   " \t"
   
   UINT32 RC2ShellRC(INT32 rc)
   {
      return engine::utilRC2ShellRC(rc);
   }

   void cutStr( const string &str, vector<string>& subs, const string &cutBy )
   {
      string::size_type prev = 0 ;
      string::size_type sep = string::npos ;
      
      sep = str.find( cutBy ) ;
      subs.push_back( string( str, 0, sep ) ) ;
      
      while ( string::npos != sep ) 
      {
         prev = sep + cutBy.size() ;
         sep = str.find( cutBy, prev ) ;
         subs.push_back( string( str, prev, sep-prev ) ) ;
      } 
   }

   void trimBoth( string &str ) 
   {
      trimLeft(str) ;
      trimRight(str) ;
   }
   void trimLeft ( string &str ) 
   {
      if ( !str.empty() )
      {
         string::size_type pos = str.find_first_not_of(EXP_SPACE_STR) ;
         str.erase( 0, pos ) ;
      }
   }
   void trimRight( string &str ) 
   {
      if ( !str.empty() )
      {
         string::size_type pos = str.find_last_not_of(EXP_SPACE_STR) ;
         if ( string::npos == pos ) { str.erase() ; }
         else if ( pos < str.size()-1 ) { str.erase( pos+1 ) ; }
      }
   }
}