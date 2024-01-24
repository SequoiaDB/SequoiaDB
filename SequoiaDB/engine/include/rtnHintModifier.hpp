/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = rtnHintModifier.hpp

   Descriptive Name = Hint modifier header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/06/2022  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_HINT_MODIFIER_HPP__
#define RTN_HINT_MODIFIER_HPP__

#include "utilPooledObject.hpp"
#include "../bson/bson.hpp"

using bson::BSONObj ;
using bson::BSONElement ;

namespace engine
{
   enum _RTN_MODIFY_OP
   {
      RTN_MODIFY_INVALID,
      RTN_MODIFY_UPDATE,
      RTN_MODIFY_REMOVE
   } ;
   typedef _RTN_MODIFY_OP RTN_MODIFY_OP ;

   class _rtnHintModifier : public utilPooledObject
   {
   public:
      _rtnHintModifier() ;
      virtual ~_rtnHintModifier() ;

      INT32 init( const BSONObj &hint, BOOLEAN getOwned = FALSE ) ;

      RTN_MODIFY_OP getOpType() const
      {
         return _modifyOp ;
      }

      const BSONObj& getOpOption() const
      {
         return _opOption ;
      }

      void setModifyShardKey( BOOLEAN modify )
      {
         _updateShardingKey = modify ;
      }

      BOOLEAN isModifyShardKey() const
      {
         return _updateShardingKey ;
      }

      BOOLEAN needRebuild() const
      {
         return _updateShardingKey ;
      }

      INT32 hint( BSONObj &hint ) ;

   protected:
      virtual INT32 _parseModifyEle( const BSONElement &ele )
      {
         return SDB_OK ;
      }

      virtual INT32 _onInit()
      {
         return SDB_OK ;
      }

   protected:
      RTN_MODIFY_OP _modifyOp ;
      BSONObj       _hint ;
      BSONObj       _opOption ;  // Operation option, e.g. updator for update.
      BOOLEAN       _updateShardingKey ;
   } ;
   typedef _rtnHintModifier  rtnHintModifier ;
}

#endif /* RTN_HINT_MODIFIER_HPP__ */
