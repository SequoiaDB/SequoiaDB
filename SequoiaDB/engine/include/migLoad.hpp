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

   Source File Name = migLoad.hpp

   Descriptive Name =

   When/how to use: parse Jsons util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2013  JW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef MIG_LOAD_JSONS_HPP_
#define MIG_LOAD_JSONS_HPP_

#include "core.hpp"
#include "ossQueue.hpp"
#include "utilParseData.hpp"
#include "../util/fromjson.hpp"
#include "../util/csv2rawbson.hpp"
#include "dmsStorageLoadExtent.hpp"
#include <boost/thread/shared_mutex.hpp>

using namespace bson ;

namespace engine
{
   #define MIG_SUM_BUFFER_SIZE   33554432
   #define MIG_BUCKET_NUMBER     32768
   #define MIG_THREAD_NUMBER     4

   struct _dataQueue
   {
      UINT32 offset ;
      UINT32 size ;
      UINT32 line ;
      UINT32 column ;

      _dataQueue()
      {
         offset = 0 ;
         size = 0 ;
         line = 0 ;
         column = 0 ;
      }
   } ;

   struct _workerReturn : public SDBObject
   {
      UINT32 success ;
      UINT32 failure ;
      INT32  rc ;

      _workerReturn()
      {
         success = 0 ;
         failure = 0 ;
         rc = 0 ;
      }
   } ;

   enum _MIG_PARSER_FILE_TYPE
   {
      MIG_PARSER_JSON = 0,
      MIG_PARSER_CSV
   } ;
   typedef enum _MIG_PARSER_FILE_TYPE MIG_PARSER_FILE_TYPE ;

   struct _setParameters : public SDBObject
   {
      //file
      CHAR        *pFileName ;
      //cs
      CHAR        *pCollectionSpaceName ;
      //cl
      CHAR        *pCollectionName ;
      //CSV field list ( CSV only )
      CHAR        *pFieldArray ;
      //send msg to client socket
      ossSocket   *clientSock ;
      //listen port ;
      CHAR        *port ;
      //buffer sum size
      UINT32       bufferSize ;
      //how many buffer split
      UINT32       bucketNum ;
      //workers number
      UINT32       workerNum ;
      //first line in input file is a header ( CSV only )
      BOOLEAN      headerline ;
      //asynchronous load
      BOOLEAN      isAsynchronous ;
      //Del char field record
      CHAR         delCFR[4] ;
      //input file type ( CSV or Json )
      MIG_PARSER_FILE_TYPE fileType ;
   } ;
   typedef struct _setParameters setParameters ;

   /*
      migMaster define
   */
   class migMaster : public SDBObject
   {
   private:
      CHAR                *_buffer     ;
      setParameters       *_pParameters;
      CHAR                *_sendMsg    ;
      INT32                _sendSize   ;
      _bucket            **_ppBucket   ;
      UINT32               _blockSize  ;

      boost::mutex         _mutex      ;
      ossQueue < _dataQueue > _queue   ;
   private:
      INT32 _stopAndWaitWorker ( pmdEDUCB *eduCB,
                                 UINT32 &success,
                                 UINT32 &failure ) ;
      INT32 _checkErrAndRollback  ( pmdEDUCB *eduCB,
                                    dmsStorageLoadOp* loadOp,
                                    dmsMBContext *mbContext,
                                    UINT32 &success,
                                    UINT32 &failure ) ;
   public:
      CHAR                 _delChar ;
      CHAR                 _delField ;
      CHAR                 _delRecord ;
      INT32                _fieldsSize ;
      BOOLEAN              _exitSignal ;
      BOOLEAN              _autoAddField ;
      BOOLEAN              _autoCompletion ;
      BOOLEAN              _isHeaderline ;
      _utilDataParser     *_parser ;
      ossSocket           *_sock ;
      CHAR                *_pFields ;
      MIG_PARSER_FILE_TYPE _fileType ;
   public:
      INT32 sendMsgToClient ( const CHAR *format, ... ) ;
      INT32 getBlockFromPointer ( UINT32 startOffset, UINT32 size,
                                  UINT32 &startBlock, UINT32 &endBlock ) ;
      void popFromQueue( pmdEDUCB *eduCB,
                         UINT32 &offset, UINT32 &size,
                         UINT32 &line,   UINT32 &column ) ;
      void pushToQueue ( UINT32 offset, UINT32 size,
                         UINT32 line,   UINT32 column ) ;
      CHAR *getBuffer() ;
      migMaster() ;
      ~migMaster() ;

      INT32 initialize( setParameters *pParameters ) ;
      INT32 run( pmdEDUCB *cb ) ;
      void bucketDec ( INT32 blockID ) ;
   } ;

   struct initWorker
   {
      migMaster      *pMaster ;        // 4 | 8 byte
      dmsStorageUnit *pSu ;            // 4 | 8 byte
      EDUID           masterEDUID ;    // 8 byte
      UINT32          clLID ;          // 4 byte
      BOOLEAN         isAsynchr ;      // 4 byte
      UINT16          collectionID ;   // 2 byte
      CHAR           _pad[6] ;
   } ;

   /*
      migWorker define
   */
   class migWorker : public SDBObject
   {
   private:
      migMaster *_master ;
      csvParser _csvParser ;
   private:
      INT32 _getBsonFromQueue ( pmdEDUCB *eduCB, BSONObj &obj ) ;
   public:
      migWorker( migMaster *master ) ;
      ~migWorker() ;
      INT32 importData( EDUID masterEDUID,
                        dmsStorageUnit *su,
                        UINT16 collectionID,
                        UINT32 clLID,
                        BOOLEAN isAsynchr,
                        pmdEDUCB *cb ) ;
   } ;

}

#endif //MIG_LOAD_JSONS_HPP_
