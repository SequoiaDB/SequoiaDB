/******************************************************************************

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

   Source File Name = mthStrParser.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_STRPARSER_HPP_
#define MTH_STRPARSER_HPP_

#include "mthSActionParser.hpp"

namespace engine
{
   class _mthSubStrParser : public _mthSActionParser::parser
   {
   public:
      _mthSubStrParser()
      {
         _name = MTH_S_SUBSTR ;
      }
      virtual ~_mthSubStrParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthStrLenParser : public _mthSActionParser::parser
   {
   public:
      _mthStrLenParser()
      {
         _name = MTH_S_STRLEN ;
      }
      virtual ~_mthStrLenParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthLowerParser : public _mthSActionParser::parser
   {
   public:
      _mthLowerParser()
      {
         _name = MTH_S_LOWER ;
      }
      virtual ~_mthLowerParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthUpperParser : public _mthSActionParser::parser
   {
   public:
      _mthUpperParser()
      {
         _name = MTH_S_UPPER ;
      }
      virtual ~_mthUpperParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthTrimParser : public _mthSActionParser::parser
   {
   public:
      _mthTrimParser()
      {
         _name = MTH_S_TRIM ;
      }
      virtual ~_mthTrimParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthLTrimParser : public _mthSActionParser::parser
   {
   public:
      _mthLTrimParser()
      {
         _name = MTH_S_LTRIM ;
      }
      virtual ~_mthLTrimParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;

   class _mthRTrimParser : public _mthSActionParser::parser
   {
   public:
      _mthRTrimParser()
      {
         _name = MTH_S_RTRIM ;
      }
      virtual ~_mthRTrimParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                            _mthSAction &action ) const ;
   } ;
}

#endif

