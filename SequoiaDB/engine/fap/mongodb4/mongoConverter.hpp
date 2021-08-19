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

   Source File Name = mongoConverter.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          01/03/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_CONVERTER_HPP_
#define _SDB_MONGO_CONVERTER_HPP_

#include "util.hpp"
#include "oss.hpp"
#include "parser.hpp"
#include "commands.hpp"
#include "msg.hpp"

class mongoConverter : public baseConverter
{
public:
   mongoConverter()
   {
      _bigEndian = checkBigEndian() ;
      _parser.setEndian( _bigEndian ) ;
   }

   ~mongoConverter()
   {
   }

   BOOLEAN isBigEndian() const
   {
      return _bigEndian ;
   }

   const UINT32 getOpType()
   {
      return _parser.currentOperation() ;
   }

   msgParser& getParser()
   {
      return _parser ;
   }

   // virtual function for baseConverter
   virtual INT32 convert( msgBuffer &out ) ;

private:
   BOOLEAN _bigEndian ;
   msgParser _parser ;
};
#endif
