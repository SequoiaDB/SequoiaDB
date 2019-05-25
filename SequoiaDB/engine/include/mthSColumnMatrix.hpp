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

   Source File Name = mthSColumnMatrix.hpp

   Descriptive Name = mth selector column matrix

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_SCOLUMNMATRIX_HPP_
#define MTH_SCOLUMNMATRIX_HPP_

#include "mthSColumn.hpp"
#include "mthNodePool.hpp"

namespace engine
{
   class _mthSColumnMatrix : public _mthSColumn 
   {
   public:
      _mthSColumnMatrix() ;
      ~_mthSColumnMatrix() ;

   public:
      OSS_INLINE const bson::BSONObj &getPattern() const
      {
         return _pattern ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         return _pattern.isEmpty() ;
      }

   public:
      virtual void clear() ;

   public:
      INT32 load( const bson::BSONObj &obj, BOOLEAN strictDataMode = FALSE ) ;

      INT32 select( const bson::BSONObj &obj,
                    bson::BSONObj &selected ) ;

   private:
      INT32 _load( const bson::BSONElement &e, BOOLEAN strictDataMode ) ;

      INT32 _loadObj( _mthSColumn *column,
                      const bson::BSONObj &obj,
                      UINT32 &actionNum,
                      BOOLEAN strictDataMode ) ;

      INT32 _loadDefaultValue( const bson::BSONElement &e ) ;

      INT32 _getColumn( const CHAR *name, _mthSColumn *&column ) ;

      INT32 _getColumn( const CHAR *name,
                        _mthSColumn *father,
                        _mthSColumn *&column) ;

      INT32 _allocateAction( _mthSAction *&action ) ;

      INT32 _addMiddleAction( _mthSColumn *column,
                              INT32 numberic ) ;

   private:
      bson::BSONObj _pattern ;

      _mthNodePool<mthSColumn> _columnPool ;
      _mthNodePool<mthSAction> _actionPool ;
   } ;
   typedef class _mthSColumnMatrix mthSColumnMatrix ;
}

#endif

