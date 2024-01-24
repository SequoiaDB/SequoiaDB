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

   Source File Name = ICollection.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_COLLECTION_HPP_
#define SDB_I_COLLECTION_HPP_

#include "sdbInterface.hpp"
#include "interface/IIndex.hpp"
#include "interface/ICursor.hpp"
#include "interface/ILob.hpp"
#include "dms.hpp"
#include "dmsRecord.hpp"
#include "dmsMetadata.hpp"
#include "dmsOprtOptions.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      ICollection define
    */
   class ICollection : public _utilPooledObject,
                       public std::enable_shared_from_this<ICollection>
   {
   public:
      ICollection() = default ;
      virtual ~ICollection() = default ;
      ICollection( const ICollection & ) = delete ;
      ICollection &operator =( const ICollection & ) = delete ;

      virtual const dmsCLMetadata &getMetadata() const = 0 ;
      virtual dmsCLMetadata &getMetadata() = 0 ;
      virtual INT32 getLobPtr( std::shared_ptr< ILob > &lob ) = 0 ;
      virtual UINT64 fetchSnapshotID() = 0 ;

      virtual INT32 createIndex( const dmsIdxMetadata &metadata,
                                 const dmsCreateIdxOptions &options,
                                 IExecutor *executor ) = 0 ;
      virtual INT32 dropIndex( const dmsIdxMetadata &metadata,
                               const dmsDropIdxOptions &options,
                               IContext *context,
                               IExecutor *executor ) = 0 ;
      virtual INT32 truncate( const dmsTruncCLOptions &options,
                              IExecutor *executor ) = 0 ;
      virtual INT32 compact( const dmsCompactCLOptions &options,
                             IExecutor *executor ) = 0 ;

      virtual INT32 getIndex( const dmsIdxMetadataKey &metadataKey,
                              IExecutor *executor,
                              std::shared_ptr<IIndex> &idxPtr ) = 0 ;
      virtual INT32 loadIndex( const dmsIdxMetadata &metadata,
                               IExecutor *executor,
                               std::shared_ptr<IIndex> &idxPtr ) = 0 ;

      virtual INT32 insertRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 updateRecord( const dmsRecordID &rid,
                                  const dmsRecordData &recordData,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 removeRecord( const dmsRecordID &rid,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 extractRecord( const dmsRecordID &rid,
                                   dmsRecordData &recordData,
                                   BOOLEAN needGetOwned,
                                   IExecutor *executor ) = 0 ;

      virtual INT32 popRecords( const dmsRecordID &rid,
                                INT32 direction,
                                IExecutor *executor,
                                UINT64 &popCount,
                                UINT64 &popSize ) = 0 ;

      virtual INT32 prepareLoads( const dmsRecordData &recordData,
                                  BOOLEAN isLast,
                                  BOOLEAN isAsynchr,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 truncateLoads( IExecutor *executor ) = 0 ;
      virtual INT32 buildLoads( BOOLEAN isAsynchr, IExecutor *executor ) = 0 ;

      virtual INT32 createDataCursor( std::unique_ptr<IDataCursor> &cursor,
                                      const dmsRecordID &startRID,
                                      BOOLEAN afterStartRID,
                                      BOOLEAN isForward,
                                      BOOLEAN isAsync,
                                      IExecutor *executor ) = 0 ;
      virtual INT32 createDataSampleCursor( std::unique_ptr<IDataCursor> &cursor,
                                            UINT64 sampleNum,
                                            BOOLEAN isAsync,
                                            IExecutor *executor ) = 0 ;

      virtual INT32 getCount( UINT64 &count,
                              BOOLEAN isFast,
                              IExecutor *executor ) = 0 ;
      virtual INT32 getDataStats( UINT64 &totalSize,
                                  UINT64 &freeSize,
                                  BOOLEAN isFast,
                                  IExecutor *executor ) = 0 ;
      virtual INT32 getIndexStats( UINT64 &totalSize,
                                   UINT64 &freeSize,
                                   BOOLEAN isFast,
                                   IExecutor *executor ) = 0 ;

      virtual INT32 getMinRecordID( dmsRecordID &rid, IExecutor *executor ) = 0 ;
      virtual INT32 getMaxRecordID( dmsRecordID &rid, IExecutor *executor ) = 0 ;

      virtual INT32 validateData( IExecutor *executor ) = 0 ;

      dmsCLMetadataKey getMetadataKey() const
      {
         return getMetadata().getCLKey() ;
      }
   } ;

}


#endif // SDB_I_COLLECTION_HPP_
