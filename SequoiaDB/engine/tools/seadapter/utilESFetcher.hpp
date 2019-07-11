/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilESFetcher.hpp

   Descriptive Name = Elasticsearch fetcher.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/16/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ESFETCHER_HPP__
#define UTIL_ESFETCHER_HPP__

#include "oss.hpp"
#include "../bson/bsonobj.h"
#include "utilCommObjBuff.hpp"
#include "utilESClt.hpp"
#include <string>

using bson::BSONObj ;

namespace seadapter
{

   class _utilESFetcher : public SDBObject
   {
      public:
         _utilESFetcher( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESFetcher() ;

         INT32 setClt( utilESClt *esClt ) ;
         INT32 setCondition( const BSONObj &condObj ) ;
         void setSize( INT64 size ) ;
         void setFilterPath( const CHAR *filterPath ) ;

         virtual BOOLEAN more() = 0 ;
         virtual INT32 fetch( utilCommObjBuff &result ) = 0 ;

      protected:
         utilESClt     *_clt ;
         std::string    _index ;
         std::string    _type ;
         BSONObj        _query ;
         INT64          _size ;
         std::string    _filterPath ;
         BOOLEAN        _more ;
   } ;
   typedef _utilESFetcher utilESFetcher ;

   class _utilESPageFetcher : public utilESFetcher
   {
      public:
         _utilESPageFetcher( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESPageFetcher() ;

         void setFrom( INT64 from ) ;
         virtual BOOLEAN more() ;
         virtual INT32 fetch( utilCommObjBuff &result ) ;

      private:
         INT64 _from ;
   } ;
   typedef _utilESPageFetcher utilESPageFetcher ;

   class _utilESScrollFetcher : public utilESFetcher
   {
      public:
         _utilESScrollFetcher( const CHAR *index, const CHAR *type ) ;
         virtual ~_utilESScrollFetcher() ;
         virtual BOOLEAN more() ;
         virtual INT32 fetch( utilCommObjBuff &result ) ;

      private:
         std::string   _scrollID ;
   } ;
   typedef _utilESScrollFetcher utilESScrollFetcher ;
}

#endif /* UTIL_ESFETCHER_HPP__ */

