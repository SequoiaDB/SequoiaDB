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

   Source File Name = IStorageEngine.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_STORAGE_ENGINE_HPP_
#define SDB_I_STORAGE_ENGINE_HPP_

#include "sdbInterface.hpp"
#include "dmsMetadata.hpp"
#include "dmsOprtOptions.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      IStorageEngine define
    */
   class IStorageEngine : public SDBObject
   {
   public:
      IStorageEngine() = default ;
      virtual ~IStorageEngine() = default ;
      IStorageEngine( const IStorageEngine &o ) = delete ;
      IStorageEngine &operator =( const IStorageEngine & ) = delete ;

   public:
      virtual DMS_STORAGE_ENGINE_TYPE getEngineType() const = 0 ;
   } ;

}

#endif // SDB_I_STORAGE_ENGINE_HPP_