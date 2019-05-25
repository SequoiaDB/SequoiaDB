/******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = mthCastParser.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_CASTPARSER_HPP_
#define MTH_CASTPARSER_HPP_

#include "mthSActionParser.hpp"
#include <map>

namespace engine
{
   class _mthCastParser : public _mthSActionParser::parser
   {
   public:
      _mthCastParser() ;
      virtual ~_mthCastParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;

   private:
      INT32 _getCastType( const CHAR *str,
                          BSONType &type ) const ;

   private:
   } ;
}

#endif

