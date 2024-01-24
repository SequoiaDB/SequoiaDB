/******************************************************************************

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

   Source File Name = mthElemMatchParser.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_ELEMMATCHPARSER_HPP_
#define MTH_ELEMMATCHPARSER_HPP_

#include "mthSActionParser.hpp"

namespace engine
{
   class _mthElemMatchParser : public _mthSActionParser::parser
   {
   public:
      _mthElemMatchParser() ;
      virtual ~_mthElemMatchParser() ;
   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;
   typedef class _mthElemMatchParser mthElemMatchParser ;
}

#endif

