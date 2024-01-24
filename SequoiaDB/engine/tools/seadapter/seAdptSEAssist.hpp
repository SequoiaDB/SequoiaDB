/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = seAdptSEAssist.hpp

   Descriptive Name = Search engine assistant for search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/14/2019  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_SEASSIST_HPP__
#define SEADPT_SEASSIST_HPP__

#include "utilESBulkBuilder.hpp"
#include "utilESClt.hpp"
#include "seAdptOprMon.hpp"
#include "utilESCltMgr.hpp"

namespace seadapter
{
   /**
    * @brief Search engine assistant. It will maintain a connection with the
    * search engine, and send requests to it.
    */
   class _seAdptSEAssist : public SDBObject
   {
   public:
      _seAdptSEAssist() ;
      ~_seAdptSEAssist() ;

      INT32 init( UINT32 bulkBuffSz = SEADPT_DFT_BULKBUFF_SZ ) ;
      INT32 createIndex( const CHAR *name, const CHAR *mapping = NULL ) ;
      INT32 dropIndex( const CHAR *name ) ;
      INT32 indexExist( const CHAR *name, BOOLEAN &exist ) ;
      INT32 getDocument( const CHAR *index, const CHAR *type, const CHAR *id,
                         BSONObj &result ) ;

      INT32 indexDocument( const CHAR *index, const CHAR *type, const CHAR *id,
                           const CHAR *jsonData ) ;

      INT32 bulkPrepare( const CHAR *index, const CHAR *type ) ;
      INT32 bulkProcess( const utilESBulkActionBase &actionItem ) ;
      INT32 processBigItem( const utilESBulkActionBase &actionItem ) ;
      INT32 bulkAppendIndex( const CHAR *docID, const BSONObj &doc ) ;
      INT32 bulkAppendDel( const CHAR *docID ) ;
      INT32 bulkAppendReplace( const CHAR *id, const CHAR *newId,
                               const BSONObj &doc ) ;
      INT32 bulkFinish() ;

      seAdptOprMon* oprMonitor()
      {
         return &_oprMon ;
      }

   private:
      utilESCltMgr *_esCltMgr ;
      UINT32 _bulkBuffSz ;
      utilESBulkBuilder _bulkBuilder ;
      CHAR _index[ SEADPT_MAX_IDXNAME_SZ + 1 ] ;
      CHAR _type[ SEADPT_MAX_TYPE_SZ + 1 ] ;
      seAdptOprMon _oprMon ;
   } ;
   typedef _seAdptSEAssist seAdptSEAssist ;
}

#endif /* SEADPT_SEASSIST_HPP__ */

