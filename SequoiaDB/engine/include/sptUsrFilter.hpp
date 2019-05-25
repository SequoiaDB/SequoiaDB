/*******************************************************************************

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

   Source File Name = sptUsrFilter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/08/2016  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USRFILTER_HPP_
#define SPT_USRFILTER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptApi.hpp"


namespace engine
{
   enum SPT_FILTER_MATCH
   {
      SPT_FILTER_MATCH_AND,
      SPT_FILTER_MATCH_OR,
      SPT_FILTER_MATCH_NOT
   } ;

   class _sptUsrFilter : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrFilter )

   public:
      _sptUsrFilter() ;
      virtual ~_sptUsrFilter() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 match( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail) ;

   private:
      BOOLEAN _match( const bson::BSONObj &obj,
                    const bson::BSONObj &filter,
                    SPT_FILTER_MATCH pred ) ;

      bson::BSONObj _filter ;
   } ;
   typedef class _sptUsrFilter sptUsrFilter ;
}
#endif
