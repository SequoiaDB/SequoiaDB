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

   Source File Name = rtnAlter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2018  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_ALTER_HPP_
#define RTN_ALTER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "rtnAlterTask.hpp"
#include "dms.hpp"
#include "utilResult.hpp"

namespace engine
{

   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _dpsLogWrapper ;
   class _dmsStorageUnit ;
   class _dmsMBContext ;

   INT32 rtnAlterCommand ( const CHAR * name,
                           RTN_ALTER_OBJECT_TYPE objectType,
                           bson::BSONObj alterObject,
                           _pmdEDUCB * cb,
                           _dpsLogWrapper * dpsCB ) ;

   INT32 rtnAlter ( const CHAR * name,
                    const rtnAlterTask * task,
                    const rtnAlterOptions * options,
                    _pmdEDUCB * cb,
                    _dpsLogWrapper * dpsCB,
                    utilWriteResult *pResult = NULL ) ;

   INT32 rtnCheckAlterCollection ( const CHAR * collection,
                                   const rtnAlterTask * task,
                                   _pmdEDUCB * cb,
                                   _dmsMBContext * mbContext,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB ) ;

   INT32 rtnAlterCollection ( const CHAR * collection,
                              const rtnAlterTask * task,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB,
                              utilWriteResult *pResult = NULL ) ;

   INT32 rtnAlterCollection ( const CHAR * collection,
                              const rtnAlterTask * task,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB,
                              _dmsMBContext * mbContext,
                              _dmsStorageUnit * su,
                              _SDB_DMSCB * dmsCB,
                              utilWriteResult *pResult = NULL ) ;

   INT32 rtnAlterCollectionSpace ( const CHAR * collectionSpace,
                                   const rtnAlterTask * task,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB ) ;

   INT32 rtnAlterCollectionSpace ( const CHAR * collectionSpace,
                                   const rtnAlterTask * task,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB ) ;

}

#endif // RTN_ALTER_HPP_
