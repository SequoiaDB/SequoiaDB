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

   Source File Name = mthSActionParser.hpp

   Descriptive Name = mth selector action parser

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_SACTIONPARSER_HPP_
#define MTH_SACTIONPARSER_HPP_

#include "mthSAction.hpp"
#include "ossLatch.hpp"
#include "ossMemPool.hpp"
#include <map>

namespace engine
{
   class _mthSActionParser : public SDBObject
   {
   public:
      _mthSActionParser() ;
      ~_mthSActionParser() ;

   public:
      static const _mthSActionParser *instance() ;

      INT32 parse( const bson::BSONElement &e,
                   _mthSAction &action ) const ;

      INT32 buildDefaultValueAction( const bson::BSONElement &e,
                                     _mthSAction &action ) const ;

      INT32 buildSliceAction( INT32 begin,
                              INT32 limit,
                               _mthSAction &action ) const ;
   public:
      /// all children will be used as a singleton
      /// do not hold any dynamic member in child class.
      class parser : public SDBObject
      {
      public:
         parser(){}
         virtual ~parser() {}

      public:
         virtual INT32 parse( const bson::BSONElement &e,
                              _mthSAction &action ) const = 0 ;

         OSS_INLINE const ossPoolString &getActionName() const
         {
            return _name ;
         }

      protected:
         ossPoolString _name ;
      } ;

   private:
      typedef std::map<ossPoolString, parser *> PARSERS ;

   private:
      INT32 _registerParsers() ;
   private:
      PARSERS _parsers ;
   } ;
   typedef class _mthSActionParser mthSActionParser ;

   class _mthTypeParser : public _mthSActionParser::parser
   {
   public:
      _mthTypeParser()
      {
         _name = MTH_S_TYPE ;
      }
      virtual ~_mthTypeParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;

   class _mthSizeParser : public _mthSActionParser::parser
   {
   public:
      _mthSizeParser()
      {
         _name = MTH_S_SIZE ;
      }
      virtual ~_mthSizeParser(){}

   public:
      virtual INT32 parse( const bson::BSONElement &e,
                           _mthSAction &action ) const ;
   } ;
}

#endif

