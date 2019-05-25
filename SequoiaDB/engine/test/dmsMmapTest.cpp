/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

*******************************************************************************/

#include "core.hpp"
#include "ossIO.hpp"
#include "ossMmap.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsExtent.hpp"
#include "dmsRecord.hpp"
#include "rtnContext.hpp"
#include "ixm.hpp"
#include "ixmExtent.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
#include "../util/fromjson.hpp"
#include "../bson/ordering.h"
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include "boost/thread.hpp"
#include "rtnIXScanner.hpp"
#include "optAPM.hpp"
#include "boost/thread.hpp"
#include <stdio.h>
#include <vector>

#define DFT_FILENAME "testMmap.dat"
#define DFT_SEGSIZE  1024*1024
#define DFT_NUMSEG   50

#define MAXTHREADS 1024

#define BIG_COLLECTION_NAME "big"
#define SMALL_COLLECTION_NAME "small"
UINT32  TESTTHREADS = 10 ;
UINT32  LOOPNUM     = 100000 ;
UINT32  TESTCREATETABLES = 10 ;
CHAR *  defaultCollectionName = "foo" ;
CHAR *  COLLECTIONNAME = defaultCollectionName ;
boost::thread *threadList[MAXTHREADS];

using namespace engine ;
using namespace bson ;
char fileName[1024];
int segmentSize = DFT_SEGSIZE ;
int numSeg      = DFT_NUMSEG ;
void printHelp (char *progName)
{
   printf("Syntax: %s [-t numThreads] [-l numLoop] [-c createTables] [-n name]\n",
            progName ) ;
}
dmsStorageUnit *myUnit = NULL ;

INT32 queryTest(BSONObj *selects, BSONObj *pattern)
{
   INT32 rc = SDB_OK ;
   boost::posix_time::ptime t1 ;
   boost::posix_time::ptime t2 ;
   rtnContext context ;
   SINT64 previousRecords = 0 ;
   if ( pattern )
   {
      try
      {
         context.newMatcher() ;
         context._matcher->loadPattern(*pattern) ;
      }
      catch(...)
      {
         printf("Failed to load pattern\n");
         return -1 ;
      }
   }
   if ( selects )
   {
      try
      {
         context._mthselector.loadPattern(*selects) ;
      }
      catch(...)
      {
         printf("Failed to load selector\n");
         return -1 ;
      }
   }
   rc = myUnit->openContext( SMALL_COLLECTION_NAME, context ) ;
   if ( rc )
   {
      printf("Failed to open context against collection %s\n",
             BIG_COLLECTION_NAME ) ;
      return rc ;
   }
   t1 = boost::posix_time::microsec_clock::local_time() ;
   do
   {
      rc = myUnit->queryFromContext( context, NULL ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
            break ;
         printf("Failed to query from context, rc = %d\n", rc) ;
         return rc ;
      }
      char *p = context._pResultBuffer ;
      for ( int i=0; i<context._numRecords- previousRecords; i++ )
      {
         BSONObj obj(p) ;
         p+=obj.objsize() ;
         p = (char*)ossRoundUpToMultipleX((ossValuePtr)p,4);
      }
      previousRecords = context._numRecords ;
   } while ( context._isOpened ) ;
   t2 = boost::posix_time::microsec_clock::local_time() ;
   boost::posix_time::time_duration diff = t2-t1 ;
   printf ( "Takes %lld ms to select %lld records\n",
            (long long)diff.total_milliseconds(), context._numRecords) ;
   return rc ;
}

INT32 concurrentDeleteBig(void *ptr)
{
   INT32 rc = SDB_OK ;
   INT64 numDeletedRecords ;
   boost::posix_time::ptime t1 ;
   boost::posix_time::ptime t2 ;
   int thread_id=*(int*)ptr;
   mthMatcher matcher ;
   try
   {
      matcher.loadPattern(BSON("tid"<<thread_id)) ;
   }
   catch(...)
   {
      printf("Failed to load pattern\n");
      return -1 ;
   }
   t1 = boost::posix_time::microsec_clock::local_time() ;
   rc = myUnit->deleteRecords(BIG_COLLECTION_NAME, NULL, NULL, &matcher,
                             numDeletedRecords );
   if ( rc )
   {
      printf("Failed to append record, rc=%d\n", rc);
      return 0 ;
   }
   t2 = boost::posix_time::microsec_clock::local_time() ;
   boost::posix_time::time_duration diff = t2-t1 ;
   printf ( "Takes %lld ms to delete %lld records\n",
            (long long)diff.total_milliseconds(), numDeletedRecords ) ;

   return 0 ;
}

INT32 concurrentUpdateSmall(void *ptr)
{
   INT32 rc = SDB_OK ;
   INT64 numUpdatedRecords ;
   boost::posix_time::ptime t1 ;
   boost::posix_time::ptime t2 ;
   int thread_id=*(int*)ptr;
   mthMatcher matcher ;
   mthModifier modifier ;
   try
   {
      matcher.loadPattern(BSON("tid"<<thread_id)) ;
   }
   catch(...)
   {
      printf("Failed to load matcher pattern\n");
      return -1 ;
   }

   try
   {
      modifier.loadPattern(BSON("$push"<<BSON("this is a long field \
name"<<"this is a very very very very very long data" ) ) ) ;
   }
   catch(...)
   {
      printf("Failed to load modifier pattern\n");
      return -1 ;
   }
   t1 = boost::posix_time::microsec_clock::local_time() ;
   rc = myUnit->updateRecords(SMALL_COLLECTION_NAME, NULL, NULL, &matcher,
                             modifier, numUpdatedRecords );
   if ( rc )
   {
      printf("Failed to update record, rc=%d\n", rc);
      return 0 ;
   }
   t2 = boost::posix_time::microsec_clock::local_time() ;
   boost::posix_time::time_duration diff = t2-t1 ;
   printf ( "Takes %lld ms to update %lld records\n",
            (long long)diff.total_milliseconds(), numUpdatedRecords) ;

   return 0 ;
}

INT32 concurrentInsertBig(void *ptr)
{
   INT32 rc = SDB_OK ;
   boost::posix_time::ptime t1 ;
   boost::posix_time::ptime t2 ;
   int thread_id=*(int*)ptr;
   BSONObj sampleObj = BSON("name"<<"concurrentInsertBig"<<"tid"<<thread_id);
   t1 = boost::posix_time::microsec_clock::local_time() ;
   for(unsigned int i=0; i<LOOPNUM; i++)
   {
      rc = myUnit->insertRecord( BIG_COLLECTION_NAME, sampleObj,
                                 NULL, NULL ) ;
      if ( rc )
      {
         printf("Failed to append record, rc=%d\n", rc);
         return 0 ;
      }
   }
   t2 = boost::posix_time::microsec_clock::local_time() ;
   boost::posix_time::time_duration diff = t2-t1 ;
   printf ( "Takes %lld ms to insert %d records\n",
            (long long)diff.total_milliseconds(), LOOPNUM) ;

   return 0 ;
}

INT32 concurrentInsertSmall(void *ptr)
{
   INT32 rc = SDB_OK ;
   boost::posix_time::ptime t1 ;
   boost::posix_time::ptime t2 ;
   int thread_id=*(int*)ptr;
   BSONObj sampleObj = BSON("name"<<"concurrentInsertSmall"<<"tid"<<thread_id);
   t1 = boost::posix_time::microsec_clock::local_time() ;
   for(unsigned int i=0; i<LOOPNUM; i++)
   {
      rc = myUnit->insertRecord( SMALL_COLLECTION_NAME, sampleObj,
                                 NULL, NULL ) ;
      if ( rc )
      {
         printf("Failed to append record, rc=%d\n", rc);
         return 0 ;
      }
   }
   t2 = boost::posix_time::microsec_clock::local_time() ;
   boost::posix_time::time_duration diff = t2-t1 ;
   printf ( "Takes %lld ms to insert %d records\n",
            (long long)diff.total_milliseconds(), LOOPNUM) ;

   return 0 ;
}
typedef INT32 (*threadFunc)(void*ptr) ;
void RunThreads ( threadFunc F, char *pDescription )
{
   int thread_id[MAXTHREADS];
   printf("%s\n", pDescription);
   getchar();
   for(unsigned int i=0; i<TESTTHREADS; i++)
   {
      thread_id[i]=i;
      threadList[i]=new boost::thread ( F,
                        (void*)&(thread_id[i]));
   }
   for(unsigned int i=0; i<TESTTHREADS; i++)
   {
      threadList[i]->join();
      delete(threadList[i]);
   }
}

int main ( int argc, char **argv )
{
   INT32 rc = SDB_OK ;
printf ( "dmsStorageUnitHeader size = %d\n", sizeof ( engine::_dmsStorageUnit::_dmsStorageUnitHeader ) ) ;
   if ( argc != 1 && argc != 3 && argc !=5 && argc != 7 && argc != 9 )
   {
      printHelp((char*)argv[0]);
      return 0;
   }
   for (int i=1; i<(argc>8?8:argc); i++)
   {
      if ( ossStrncmp((char*)argv[i],"-t",strlen("-t"))==0 )
      {
         TESTTHREADS=atoi((char*)argv[++i]);
         if(TESTTHREADS<0 || TESTTHREADS>MAXTHREADS)
         {
            printf("num threads must be within 0 to %d\n",MAXTHREADS);
            return 0;
         }
      }
      else if (ossStrncmp((char*)argv[i],"-n",strlen("-n"))==0 )
      {
         COLLECTIONNAME=(char*)argv[++i];
      }
      else if (ossStrncmp((char*)argv[i],"-l",strlen("-l"))==0 )
      {
         LOOPNUM=atoi((char*)argv[++i]);
      }
      else if ( ossStrncmp((char*)argv[i],"-c", strlen("-c"))== 0 )
      {
         TESTCREATETABLES = atoi((char*)argv[++i]);
      }
      else
      {
         printHelp((char*)argv[0]);
         return 0;
      }
   }
   printf("numThread = %d\n", TESTTHREADS);
   printf("loopNum   = %d\n", LOOPNUM);
   printf("testTables = %d\n", TESTCREATETABLES ) ;

   printf("dmsStorageUnit size = %d\n", (int)sizeof(dmsStorageUnit));
   printf("dmsExtent size = %d\n", (int)sizeof(dmsExtent)) ;
   printf("dmsRecord size = %d\n", (int)sizeof(dmsRecord)) ;
   printf("dmsDeletedRecord size = %d\n", (int)sizeof(dmsDeletedRecord)) ;

   fflush(stdout);
   myUnit = new dmsStorageUnit( COLLECTIONNAME ,1, NULL, NULL ) ;
   if ( !myUnit )
   {
      printf ("Failed to allocate memory for myUnit\n" );
      return 0 ;
   }
   rc = myUnit->open( "./", "./", TRUE) ;
   if ( rc )
   {
      printf("Failed to open SU, rc = %d\n", rc );
      return 0 ;
   }

   boost::posix_time::ptime t1 ;
   boost::posix_time::ptime t2 ;
   boost::posix_time::ptime t3 ;
   boost::posix_time::time_duration diff1 ;

   srand(time(NULL));

   char collectionName [ 1024 ] = {0} ;
   dmsMBContext *mbContext = NULL ;
   printf("Test creating 10 tables without any page\n") ;
   for ( UINT32 testTableNum = 0; testTableNum < TESTCREATETABLES;
         testTableNum++ )
   {
      UINT32 pages = rand()%99+1 ;

      UINT32 stringSize = rand()%9+1 ;
      for ( UINT32 i = 0; i<stringSize; i++ )
      {
         collectionName[i]=rand()%26+'a' ;
      }
      collectionName[stringSize] = 0 ;
      printf("CollectionName: %s, NumPages: %d\n", collectionName, pages ) ;
      rc = myUnit->data()->addCollection( collectionName, NULL, 0, NULL, NULL,
                                          pages, TRUE ) ;
      if ( rc )
      {
         printf("Failed to create collection, rc=%d\n", rc ) ;
         break ;
      }
   }
   t1 = boost::posix_time::microsec_clock::local_time() ;
   rc = myUnit->data()->dropCollection( BIG_COLLECTION_NAME, NULL, NULL,
                                        TRUE ) ;
   if ( SDB_DMS_NOTEXIST == rc )
   {
      printf("Collection '%s' does not exist\n", BIG_COLLECTION_NAME );
   }
   else if ( rc )
   {
      printf("Failed to drop collection, rc = %d\n", rc ) ;
      return 0 ;
   }
   else
   {
      t2 = boost::posix_time::microsec_clock::local_time() ;
      diff1 = t2-t1 ;
      printf("Drop collection successful in %lld ms\n",
             (long long)diff1.total_milliseconds()) ;
   }
   printf("add 4096 pages extent");
   t1 = boost::posix_time::microsec_clock::local_time() ;
   rc = myUnit->data()->addCollection( BIG_COLLECTION_NAME, NULL, 0, NULL, NULL,
                                       4096, TRUE ) ;
   if ( rc )
   {
      printf("Failed to create collection, rc=%d\n", rc ) ;
      return 0 ;
   }
   t2 = boost::posix_time::microsec_clock::local_time() ;
   diff1 = t2-t1 ;
   printf("Add collection successful in %lld ms\n",
          (long long)diff1.total_milliseconds()) ;
   RunThreads(concurrentInsertBig, "Inserting records into big collection");

   dmsExtentID indexCBExtent ;
   BSONObj indexDef ;
   if ( SDB_OK != fromjson ( "{key:{tid:-1},name:\"test\"}", indexDef ) )
   {
      printf ( "Failed to create index def\n" ) ;
      return 0 ;
   }
   rc = myUnit->createIndex ( BIG_COLLECTION_NAME, indexDef, NULL, NULL ) ;
   if ( rc )
   {
      printf ("Failed to create index\n" ) ;
      return 0 ;
   }
   rc = myUnit->getIndexCBExtent ( BIG_COLLECTION_NAME, "test", indexCBExtent);
   if ( rc )
   {
      printf("Failed to get index cb extent, rc = %d\n", rc ) ;
      return 0 ;
   }
   ixmIndexCB indexCB ( indexCBExtent, myUnit->index() ) ;
   if ( !indexCB.isInitialized() )
   {
      printf ("Failed to init indexcb\n" ) ;
      return 0 ;
   }
   dmsExtentID rootExtent = indexCB.getRoot() ;
   if ( DMS_INVALID_EXTENT == rootExtent )
   {
      printf ("No root extent exist\n" ) ;
      return 0 ;
   }
   ixmExtent root ( rootExtent, myUnit ) ;
   printf("Totally %lld keys in the index\n", root.count()) ;
while ( true )
{
   CHAR inputBuffer[1024] = {0} ;
   BSONObj inputObj ;
   printf("Please input search condition: ");
   ossMemset ( inputBuffer, 0, sizeof(inputBuffer)) ;
   gets(inputBuffer) ;
   inputBuffer[ossStrlen(inputBuffer)]=0 ;
   if ( SDB_OK != fromjson ( inputBuffer, inputObj ) )
   {
      printf ( "Invalid BSON Key\n" ) ;
      break ;
   }
   BSONObj emptyObj ;
}
/*
   rtnPredicateListIterator listIterator ( rtnList ) ;
   ixmRecordID rid ;
   rid.reset() ;
   Ordering order = Ordering::make(indexCB.keyPattern());
   rc = root.keyLocate ( rid, BSONObj(), 0, FALSE, listIterator.cmp(),
                         listIterator.inc(), order, 1 ) ;
   if ( rc )
   {
      printf("failed to locate key\n") ;
      return 0;
   }
   while ( !rid.isNull() )
   {
      ixmExtent indexExtent ( rid._extent, myUnit ) ;
      CHAR *dataBuffer = indexExtent.getKeyData(rid._slot ) ;
      if ( !dataBuffer )
      {
         printf("Failed to get buffer\n") ;
         return 0 ;
      }
      BSONObj keyObj = ixmKey(dataBuffer).toBson() ;
      rc = listIterator.advance ( keyObj ) ;
      if ( rc == -2 )
      {
         printf("hit end of result\n");
         break ;
      }
      if ( rc >= 0 )
      {
         rc = indexExtent.keyAdvance ( rid, keyObj, rc, listIterator.after(),
                                       listIterator.cmp(), listIterator.inc(),
                                       order, 1 ) ;
         if ( rc )
         {
            printf("Failed to keyadvance\n") ;
            return 0 ;
         }
      }
      else if ( rc == -1 )
      {
         dmsRecordID dmsrid = indexExtent.getRID ( rid._slot ) ;
         BSONObj dataRecord ;
         if ( !dmsrid.isNull() )
         {
            rc = myUnit->fetch ( dmsrid, dataRecord ) ;
            if ( rc )
            {
               printf("failed to fetch\n");
               return 0 ;
            }
            printf("dataRecord = %s\n",dataRecord.toString(false, false).c_str()) ;
         }
         rc = indexExtent.advance ( rid, 1 ) ;
         if ( rc )
         {
            printf("Failed to advance\n");
            return 0;
         }
         continue ;
      }
   }
*/
/*
   while ( true )
   {
      BSONObj dataobj ;
      printf("Please input record element: " ) ;
      ossMemset ( inputBuffer, 0, sizeof(inputBuffer)) ;
      gets(inputBuffer) ;
      inputBuffer[ossStrlen(inputBuffer)]=0 ;
      try
      {
         dataobj = fromjson ( inputBuffer ) ;
      }
      catch(...)
      {
         printf("Invalid BSON key\n") ;
         return 0 ;
      }
      INT32 result = listIterator.advance ( dataobj ) ;
      printf ("result = %d\n", result ) ;
   }*/
   return  0 ;
   t1 = boost::posix_time::microsec_clock::local_time() ;
   rc = myUnit->dropCollection(SMALL_COLLECTION_NAME, NULL, NULL);
   if ( SDB_DMS_NOTEXIST == rc )
   {
      printf("Collection '%s' does not exist\n",SMALL_COLLECTION_NAME);
   }
   else if ( rc )
   {
      printf("Failed to drop collection, rc = %d\n", rc ) ;
      return 0 ;
   }
   else
   {
      t2 = boost::posix_time::microsec_clock::local_time() ;
      diff1 = t2-t1 ;
      printf("Drop collection successful in %lld ms\n",
             (long long)diff1.total_milliseconds()) ;
   }
   printf("add 0 page extent");
   t1 = boost::posix_time::microsec_clock::local_time() ;
   rc = myUnit->addCollection(SMALL_COLLECTION_NAME, NULL, 0, NULL, NULL, 0 ) ;
   if ( rc )
   {
      printf("Failed to create collection, rc=%d\n", rc ) ;
      return 0 ;
   }
   t2 = boost::posix_time::microsec_clock::local_time() ;
   diff1 = t2-t1 ;
   printf("Add collection successful in %lld ms\n",
          (long long)diff1.total_milliseconds()) ;
   RunThreads(concurrentInsertSmall, "Inserting records into small \
collection");
   BSONObj pattern = BSON("tid"<<0) ;
   BSONObj selects = BSON("tid"<<100) ;
   rc = queryTest(&selects, &pattern) ;
   if ( rc )
   {
      printf("Failed to query\n");
   }

   RunThreads(concurrentDeleteBig, "Deleting records from big collection");
   rc = queryTest(NULL, NULL) ;
   if ( rc )
   {
      printf("Failed to query\n");
   }

   RunThreads(concurrentUpdateSmall, "Updating records from small collection") ;
   RunThreads(concurrentUpdateSmall, "Updating records from small collection") ;
   rc = queryTest(&selects, NULL) ;
   if ( rc )
   {
      printf("Failed to query\n");
   }
   if ( myUnit )
      delete myUnit ;
   return 0;
}
