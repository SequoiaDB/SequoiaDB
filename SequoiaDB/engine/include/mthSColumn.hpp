/*******************************************************************************

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

   Source File Name = mthSColumn.hpp

   Descriptive Name = mth selector column

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_SCOLUMN_HPP_
#define MTH_SCOLUMN_HPP_

#include "mthSAction.hpp"
#include "utilArray.hpp"
#include "mthSAttribute.hpp"
#include "utilPooledObject.hpp"

#define MTH_SCOLUMN_STATIC_NAME_BUF_LEN 32
#define MTH_SCOLUMN_INVALID_SUBARRAY_INDEX -1

namespace engine
{
   class _mthSColumn ;
   typedef _utilArray< _mthSColumn * > MTH_S_COLUMNS ;
   typedef _utilArray< _mthSAction * > MTH_S_ACTIONS ;

   class _mthSColumn : public _utilPooledObject
   {
   public:
      _mthSColumn() ;
      virtual ~_mthSColumn() ;

   public:
      OSS_INLINE const CHAR *getName() const
      {
         return _name ;
      }
   public:
      INT32 init( const CHAR *name ) ;

      INT32 addAction( _mthSAction *action ) ;

      INT32 build( const bson::BSONElement &e,
                   bson::BSONObjBuilder &builder ) ;

   public:
      virtual void clear() ;

   protected:
      OSS_INLINE MTH_S_COLUMNS &_getSubColumns()
      {
         return _subColumns ;
      }

      OSS_INLINE _mthSColumn *&_getFather()
      {
         return _father ;
      }

      OSS_INLINE _utilArray<_mthSAction *> &_getActions()
      {
         return _actions ;
      }

      OSS_INLINE void _setSubArrayIndex( INT32 subArrayIndex )
      {
         _subArrayIndex = subArrayIndex ;
      }

      OSS_INLINE INT32 _getSubArrayIndex()
      {
         return _subArrayIndex ;
      }

      INT32 _setAttribute( MTH_S_ATTRIBUTE attribute ) ;

      INT32 _selectWithExclusion( const bson::BSONObj &obj,
                                  bson::BSONObjBuilder &builder ) ;

      BOOLEAN _findColumn( const CHAR *name,
                           MTH_S_COLUMNS &array,
                           _mthSColumn *&column,
                           UINT32 *number = NULL ) ;

      INT32 _build( const bson::BSONElement &e,
                    bson::BSONObjBuilder &builder,
                    UINT32 actionIndex = 0 ) ;

      INT32 _buildFromChildren( const bson::BSONElement &e,
                                bson::BSONObjBuilder &builder ) ;

      INT32 _buildFromChildren( const bson::BSONElement &e,
                                bson::BSONArrayBuilder &builder ) ;

      INT32 _buildObjFromChildren( const bson::BSONObj &obj,
                                   bson::BSONObjBuilder &builder ) ;

      INT32 _buildLastChildren( MTH_S_COLUMNS &array,
                                bson::BSONObjBuilder &builder ) ;

      INT32 _buildSubArray( const bson::BSONElement &e,
                            bson::BSONArrayBuilder &builder ) ;

      BOOLEAN _needBuildSubArray() ;

   private:
      MTH_S_COLUMNS _subColumns ;
      _mthSColumn *_father ;

      MTH_S_ACTIONS _actions ;

      const CHAR *_name ;
      CHAR _staticName[MTH_SCOLUMN_STATIC_NAME_BUF_LEN] ;
      CHAR *_dynamicName ;

      _mthSAttribute _attribute ;

      INT32 _subArrayIndex ;

   friend class _mthSColumnMatrix ;
   } ;
   typedef class _mthSColumn mthSColumn ;
}

#endif

