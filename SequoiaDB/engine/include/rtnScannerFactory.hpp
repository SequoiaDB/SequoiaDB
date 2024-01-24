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

   Source File Name = rtnIXScanner.hpp

   Descriptive Name = RunTime Index Scanner Factory Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for index
   scanner, which is used to traverse index tree.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/09/2018  YXC Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNSCANNERFAC_HPP__
#define RTNSCANNERFAC_HPP__

#include "rtnIXScanner.hpp"
#include "rtnDiskIXScanner.hpp"
#include "rtnMemIXTreeScanner.hpp"
#include "rtnMergeIXScanner.hpp"
#include "rtnTBScanner.hpp"
#include "rtnDiskTBScanner.hpp"
#include "rtnMemTBScanner.hpp"
#include "rtnMergeTBScanner.hpp"

namespace engine
{
   // class factory for initializing scanner purpose
   class _rtnScannerFactory
   {
   public:
      INT32 createTBScanner( rtnScannerType type,
                             _dmsStorageUnit *su,
                             _dmsMBContext *mbContext,
                             BOOLEAN isAsync,
                             _pmdEDUCB *cb,
                             _rtnTBScanner *&pScanner )
      {
         INT32 rc = SDB_OK ;

         pScanner = NULL ;

         switch ( type )
         {
         case SCANNER_TYPE_DISK:
            pScanner = SDB_OSS_NEW rtnDiskTBScanner(
                              su, mbContext, dmsRecordID(), FALSE, 1, isAsync, cb ) ;
            break ;
         case SCANNER_TYPE_MEM_TREE:
            pScanner = SDB_OSS_NEW rtnMemTBScanner(
                              su, mbContext, dmsRecordID(), FALSE, 1, cb ) ;
            break ;
         case SCANNER_TYPE_MERGE:
            pScanner = SDB_OSS_NEW rtnMergeTBScanner(
                              su, mbContext, dmsRecordID(), FALSE, 1, cb ) ;
            break;
         default:
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid type[%d]", type ) ;
            goto error ;
         }

         if ( !pScanner )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate scanner failed" ) ;
            goto error ;
         }

         rc = pScanner->init() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init scanner[type:%d] failed, rc: %d",
                    type, rc ) ;
            SDB_OSS_DEL pScanner ;
            pScanner = NULL ;
            goto error ;
         }

      done:
         return rc ;

      error:
         goto done ;
      }

      INT32 createIXScanner( rtnScannerType type,
                             ixmIndexCB *indexCB,
                             rtnPredicateList *predList,
                             _dmsStorageUnit *su,
                             _dmsMBContext *mbContext,
                             BOOLEAN isAsync,
                             _pmdEDUCB *cb,
                             _rtnIXScanner *&pScanner )
      {
         INT32 rc = SDB_OK ;

         pScanner = NULL ;

         switch (type)
         {
            case SCANNER_TYPE_DISK:
               pScanner = SDB_OSS_NEW rtnDiskIXScanner( indexCB, predList,
                                                        su, mbContext, isAsync,
                                                        cb ) ;
               break ;
            case SCANNER_TYPE_MEM_TREE:
               pScanner = SDB_OSS_NEW rtnMemIXTreeScanner( indexCB, predList,
                                                           su, mbContext, cb ) ;
               break ;
            case SCANNER_TYPE_MERGE:
               pScanner = SDB_OSS_NEW rtnMergeIXScanner( indexCB, predList,
                                                         su, mbContext, cb ) ;
               break;
            default :
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Invalid type[%d]", type ) ;
               goto error ;
               break ;
         }

         if ( !pScanner )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate scanner[type:%d] failed", type ) ;
            goto error ;
         }

         rc = pScanner->init() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init scanner[type:%d] failed, rc: %d",
                    type, rc ) ;
            SDB_OSS_DEL pScanner ;
            pScanner = NULL ;
            goto error ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      void releaseScanner( _rtnScanner *pScanner )
      {
         if ( pScanner )
         {
            SDB_OSS_DEL pScanner ;
         }
      }

   } ;
   typedef _rtnScannerFactory rtnScannerFactory ;


}

#endif //RTNSCANNERFAC_HPP__

