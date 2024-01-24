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

   Source File Name = mthMathParser.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_MATHPARSER_HPP_
#define MTH_MATHPARSER_HPP_

#include "mthSActionParser.hpp"

namespace engine
{
   class _mthAbsParser : public _mthSActionParser::parser
   {
   public:
      _mthAbsParser()
      {
         _name = MTH_S_ABS ;
      }
      virtual ~_mthAbsParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthCeilingParser : public _mthSActionParser::parser
   {
   public:
      _mthCeilingParser()
      {
         _name = MTH_S_CEILING ;
      }
      virtual ~_mthCeilingParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthFloorParser : public _mthSActionParser::parser
   {
   public:
      _mthFloorParser()
      {
         _name = MTH_S_FLOOR ;
      }
      virtual ~_mthFloorParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthModParser : public _mthSActionParser::parser
   {
   public:
      _mthModParser()
      {
         _name = MTH_S_MOD ;
      }
      virtual ~_mthModParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthAddParser : public _mthSActionParser::parser
   {
   public:
      _mthAddParser()
      {
         _name = MTH_S_ADD ;
      }
      virtual ~_mthAddParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthSubtractParser : public _mthSActionParser::parser
   {
   public:
      _mthSubtractParser()
      {
         _name = MTH_S_SUBTRACT ;
      }
      virtual ~_mthSubtractParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthMultiplyParser : public _mthSActionParser::parser
   {
   public:
      _mthMultiplyParser()
      {
         _name = MTH_S_MULTIPLY ;
      }
      virtual ~_mthMultiplyParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthDivideParser : public _mthSActionParser::parser
   {
   public:
      _mthDivideParser()
      {
         _name = MTH_S_DIVIDE ;
      }
      virtual ~_mthDivideParser() {}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

}

#endif

