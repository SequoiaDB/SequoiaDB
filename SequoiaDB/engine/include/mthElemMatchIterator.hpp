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

   Source File Name = mthElemMatchIterator.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_ELEMMATCHITERATOR_HPP_
#define MTH_ELEMMATCHITERATOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _mthMatchTree ;

   class _mthElemMatchIterator : public SDBObject
   {
   public:
      _mthElemMatchIterator( const bson::BSONObj &obj,
                             _mthMatchTree *matcher,
                             INT32 n = -1,
                             BOOLEAN isArray = TRUE ) ;
      ~_mthElemMatchIterator() ;

   public:
      INT32 next( bson::BSONElement &e ) ;

   private:
      _mthMatchTree *_matcher ;
      bson::BSONObj _obj ;
      bson::BSONObjIterator _i ;
      INT32 _n ;
      INT32 _matched ;
      BOOLEAN _isArray ;
   } ;
   typedef class _mthElemMatchIterator mthElemMatchIterator ;
}

#endif

