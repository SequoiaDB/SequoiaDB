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

   Source File Name = IIndex.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_INDEX_HPP_
#define SDB_I_INDEX_HPP_

#include "sdbInterface.hpp"
#include "interface/ICursor.hpp"
#include "keystring/utilKeyString.hpp"
#include "dms.hpp"
#include "dmsRecord.hpp"
#include "dmsOprtOptions.hpp"
#include "utilInsertResult.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      _IIndex define
    */
   class IIndex : public _utilPooledObject,
                  public std::enable_shared_from_this<IIndex>
   {
   public:
      IIndex() = default ;
      virtual ~IIndex() = default ;
      IIndex( const IIndex & ) = delete ;
      IIndex &operator =( const IIndex & ) = delete ;

      virtual const dmsIdxMetadata &getMetadata() const = 0 ;
      virtual dmsIdxMetadata &getMetadata() = 0 ;
      virtual UINT64 fetchSnapshotID() = 0 ;

      virtual INT32 truncate( const dmsTruncateIdxOptions &options,
                              IExecutor *executor ) = 0 ;
      virtual INT32 compact( const dmsCompactIdxOptions &options,
                             IExecutor *executor ) = 0 ;

      virtual INT32 index( const bson::BSONObj &key,
                           const dmsRecordID &rid,
                           BOOLEAN allowDuplicated,
                           IExecutor *executor,
                           utilWriteResult *result ) = 0 ;
      virtual INT32 unindex( const bson::BSONObj &key,
                             const dmsRecordID &rid,
                             IExecutor *executor ) = 0 ;

      virtual INT32 createIndexCursor( std::unique_ptr<IIndexCursor> &cursor,
                                       const keystring::keyString &startKey,
                                       BOOLEAN isAfterStartKey,
                                       BOOLEAN isForward,
                                       IExecutor *executor ) = 0 ;
      virtual INT32 createIndexSampleCursor( std::unique_ptr<IIndexCursor> &cursor,
                                             UINT64 sampleNum,
                                             IExecutor *executor ) = 0 ;
      virtual INT32 getIndexStats( UINT64 &totalSize,
                                   UINT64 &freeSize,
                                   BOOLEAN isFast,
                                   IExecutor *executor ) = 0 ;

      dmsIdxMetadataKey getMetadataKey() const
      {
         return getMetadata().getIdxKey() ;
      }
   } ;

}


#endif // SDB_I_INDEX_HPP_