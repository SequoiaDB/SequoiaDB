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

   Source File Name = expCL.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef EXP_CL_HPP_
#define EXP_CL_HPP_

#include "oss.hpp"
#include "../client/bson/bson.h"
#include "../client/client.h"
#include <vector>
#include <string>
#include <set>

namespace exprt
{
   using namespace std ;

   #define EXPCL_FIELDS_SEP_CHAR    ':'
   #define EXPCL_FIELDS_SEP_STR     ":"

   struct expCL : public SDBObject
   {
      string csName ;
      string clName ;
      string fields ;
      string select ;
      string filter ;
      string sort ;
      INT64  skip ;
      INT64  limit ;

      expCL( const string &csName_,
             const string &clName_,
             const string &fields_ = "",
             const string &select_ = "",
             const string &filter_ = "",
             const string &sort_ = "" ) :
             csName(csName_), clName(clName_), fields(fields_),
             select(select_), filter(filter_), sort(sort_), skip(0), limit(-1)
      {
      }

      expCL() : csName(""), clName(""), fields(""), select(""),
                filter(""), sort(""), skip(0), limit(-1)
      {
      }

      inline string fullName() const
      {
         string name = csName ;
         name += "." ;
         name += clName ;
         return name ;
      }

      // format of rawStr should be :
      // "<csName>.<clName> <field-list>"
      // "<csName>.<clName>" must be specified while "<field-list>" may not be
      INT32 parseCLFields( const string &rawStr ) ;

      expCL &swap( expCL &other )
      {
         csName.swap(other.csName) ;
         clName.swap(other.clName) ;
         fields.swap(other.fields) ;
         filter.swap(other.filter) ;
         sort.swap(other.sort) ;
         return *this ;
      }
   } ;

   bool operator<( const expCL &CL1, const expCL &CL2 ) ;

   class expOptions ;
   class expCLSet : public SDBObject
   {
   public :
      typedef vector<expCL>::iterator iterator ;
      typedef vector<expCL>::const_iterator const_iterator ;

   public :
      explicit expCLSet( expOptions &options ) :
         _options(options), _includeAll(FALSE)
      {
      }
      INT32 parse( sdbConnectionHandle hConn ) ;
      inline iterator       begin()       { return _collections.begin() ; }
      inline const_iterator begin() const { return _collections.begin() ; }
      inline iterator       end()         { return _collections.end() ; }
      inline const_iterator end()   const { return _collections.end() ; }

   private :
      INT32 _completeCLListFields( sdbConnectionHandle hConn ) ;
      INT32 _parseRawFileds( set<expCL> &clFields ) ;
      INT32 _generateCLList( sdbConnectionHandle hConn,
                             const set<string> &includeCS,
                             const set<expCL> &includeCollection,
                             const set<string> &excludeCS,
                             const set<expCL> &excludeCollection ) ;
      // should be call after parsing
      INT32 _parsePost() ;
   private :
      expOptions     &_options ;
      vector<expCL>   _collections ;
      BOOLEAN         _includeAll ;
   } ;

}
#endif
