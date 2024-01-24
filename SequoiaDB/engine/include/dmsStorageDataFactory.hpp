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

   Source File Name = dmsStorageDataFactory.hpp

   Descriptive Name = dms Storage Data Object Factory.

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SU cache
   management ( including plan cache, statistics cache, etc. )

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_DATAFACTORY_HPP__
#define DMSSTORAGE_DATAFACTORY_HPP__

#include "dmsStorageDataCommon.hpp"

namespace engine
{
   class _dmsStorageDataFactory : public SDBObject
   {
   public:
      _dmsStorageDataFactory() {}
      ~_dmsStorageDataFactory() {}

      dmsStorageDataCommon* createProduct( IStorageService *engine,
                                           dmsSUDescriptor *suDescriptor,
                                           DMS_STORAGE_TYPE type,
                                           const CHAR *suFileName,
                                           _IDmsEventHolder *pEventHolder ) ;
   } ;
   typedef _dmsStorageDataFactory dmsStorageDataFactory ;

   dmsStorageDataFactory* getDMSStorageDataFactory() ;
}

#endif /* DMSSTORAGE_DATAFACTORY_HPP__ */

