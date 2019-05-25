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

   Source File Name = qgmHashTable.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef qgmHASHTABLE_HPP_
#define qgmHASHTABLE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{
typedef  INT64 QGM_HT_CONTEXT ;
#define QGM_HT_INVALID_CONTEXT -1

   class _qgmHashTable : public SDBObject
   {
   public:
      _qgmHashTable() ;
      virtual ~_qgmHashTable() ;

   public:
      INT32 init( UINT64 bufSize ) ;

      INT32 push( const CHAR *fieldName,
                  const BSONObj &value ) ;

      INT32 find( const BSONElement &key,
                  QGM_HT_CONTEXT &context ) ;

      INT32 getMore( const BSONElement &key,
                     QGM_HT_CONTEXT &context,
                     BSONObj &value ) ;

      void clear() ;

      void release() ;
   private:

#pragma pack(1)
      struct hashTuple
      {
         const CHAR *data ;
         INT32 fieldNameSize ;
         INT32 totalSize ;

         const CHAR *value ; // bosnobj rawdata.
         hashTuple *next ;

         void init()
         {
            data = NULL ;
            fieldNameSize = -1 ;
            totalSize = -1 ;
            value = NULL ;
            next = NULL ;
         }
      } ;
#pragma pack()

   private:
      CHAR *_buf ;
      UINT64 _bufSize ;
      UINT64 _written ;
      UINT32 _buckets ;
   } ;
   typedef class _qgmHashTable qgmHashTable ;
}

#endif

