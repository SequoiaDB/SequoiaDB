/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dmsExtDataHandler.hpp

   Descriptive Name = External data process handler for dms.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_EXTDATAHANDLER_HPP__
#define DMS_EXTDATAHANDLER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogWrapper.hpp"
#include "monDMS.hpp"
#include "../bson/oid.h"

namespace engine
{
   class _IDmsExtDataHandler
   {
   public:
      _IDmsExtDataHandler() {}
      virtual ~_IDmsExtDataHandler() {}

   public:
      virtual INT32 getExtDataName( const CHAR *csName, const CHAR *clName,
                                    const CHAR *idxName, CHAR *buff,
                                    UINT32 buffSize ) = 0 ;

      virtual INT32 onOpenTextIdx( const CHAR *csName, const CHAR *clName,
                                   const CHAR *idxName ) = 0 ;

      virtual INT32 onDelCS( const CHAR *csName, _pmdEDUCB *cb,
                             BOOLEAN removeFiles, SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onDropAllIndexes( const CHAR *csName, const CHAR *clName,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onDropTextIdx( const CHAR *csName, const CHAR *clName,
                                   const CHAR *idxName, _pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onRebuildTextIdx( const CHAR *csName, const CHAR *clName,
                                      const CHAR *idxName, _pmdEDUCB *cb,
                                      SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onInsert( const CHAR *csName, const CHAR *clName,
                              const CHAR *idxName, const ixmIndexCB &indexCB,
                              const BSONObj &object, _pmdEDUCB* cb,
                              SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onDelete( const CHAR *csName, const CHAR *clName,
                              const CHAR *idxName, const ixmIndexCB &indexCB,
                              const BSONObj &object, _pmdEDUCB* cb,
                              SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onUpdate( const CHAR *csName, const CHAR *clName,
                              const CHAR *idxName, const ixmIndexCB &indexCB,
                              const BSONObj &orignalObj, const BSONObj &newObj,
                              _pmdEDUCB* cb, SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onTruncateCL( const CHAR *csName, const CHAR *clName,
                                  _pmdEDUCB *cb, SDB_DPSCB *dpsCB = NULL ) = 0 ;

      virtual INT32 done( _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 abortOperation( _pmdEDUCB *cb ) = 0 ;
   } ;
   typedef _IDmsExtDataHandler IDmsExtDataHandler ;
}

#endif /* DMS_EXTDATAHANDLER_HPP__ */

