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

#include "dmsStorageUnit.hpp"
#include "ixm.hpp"
#include "ixmExtent.hpp"
#include "ixmInsertRequest.hpp"
#include "ossUtil.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "../util/fromjson.hpp"
#include "dmsDump.hpp"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <map>
using namespace engine ;
using namespace bson ;
dmsStorageUnit myUnit("fooixm", 1, NULL, NULL ) ;

char myoutBuf[8192] = { 0 } ;
int main ( int argc, char **argv )
{
   INT32 rc = SDB_OK ;
   srand(time(NULL)) ;
   CHAR inputBuffer[1024] ;
   ixmIndexCB *indexCB = NULL ;

   ossValuePtr startOffset = DMS_PAGE_SIZE64K + DMS_SME_LEN ;
   CHAR * addPtr = (CHAR*)startOffset ;
   ossPrimitiveFileOp storageUint ;
   ossPrimitiveFileOp::offsetType fileOffset ;
   storageUint.Open( "/home/sequoiadb/sequoiadb/build/linux2/dd/engine/foo.1",
                     OSS_PRIMITIVE_FILE_OP_READ_ONLY ) ;
   fileOffset.offset = startOffset ;
   storageUint.seekToOffset( fileOffset ) ;
   std::vector<UINT16> collections ;
   for ( UINT16 i=0 ; i< DMS_MB_SIZE ; i++ )
   {
      addPtr = (CHAR*)fileOffset.offset ;
      storageUint.Read( 1024, inputBuffer, NULL ) ;
      dmsDump::dumpMB( inputBuffer, 1024,
                       myoutBuf, sizeof( myoutBuf ),
                       addPtr,
                       DMS_SU_DMP_OPT_HEX |
                       DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                       DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                       DMS_SU_DMP_OPT_FORMATTED,
                       NULL, collections, FALSE ) ;
      fileOffset.offset += 1024 ;
      storageUint.seekToOffset( fileOffset ) ;
      printf( "%s\n", myoutBuf ) ;
   }
   storageUint.Close() ;
   return rc ;

   printf("size=%d\n", (INT32)sizeof(ixmKeyNode));
   rc = myUnit.open("./", "./", "./", TRUE ) ;
   if ( rc )
   {
      printf("Failed to open collection, rc = %d\n", rc ) ;
      return 0 ;
   }
   rc = myUnit.data()->addCollection("test", NULL, 0, NULL, NULL, 1 ) ;
   if ( rc )
   {
      if ( SDB_DMS_EXIST != rc )
      {
         printf("Failed to create collection, rc = %d\n", rc ) ;
         return 0 ;
      }
   }
   BSONObj indexDef ;
   printf("Please input key: ");
   ossMemset ( inputBuffer, 0, sizeof(inputBuffer) ) ;
   gets(inputBuffer) ;
   inputBuffer[ossStrlen(inputBuffer)]=0 ;
   BSONObj inputObj ;
   BSONObjSet keySet ;
   if ( SDB_OK != fromjson ( inputBuffer, indexDef ) )
   {
      printf ( "Invalid BSON Key\n" ) ;
      return 0 ;
   }
   indexCB = new ixmIndexCB ( 1, indexDef, 0, myUnit.index(), NULL ) ;
   if ( !indexCB )
   {
      printf ("Failed to create indexCB\n") ;
      return 0 ;
   }
   if ( !indexCB->isInitialized())
   {
      printf("Failed to initialize index\n") ;
      return 0 ;
   }
   printf("Key Pattern: %s\n", indexCB->keyPattern().toString().c_str()) ;
   dmsExtentID newextent ;
   rc = myUnit.index()->reserveExtent(0, newextent, NULL ) ;
   if ( rc )
   {
      printf("Failed to reserve extent\n" ) ;
      return 0 ;
   }
   indexCB->setRoot(newextent) ;
   ixmExtent idx(newextent, 0, myUnit.index()) ;

   printf("Please input search condition: ");
   ossMemset ( inputBuffer, 0, sizeof(inputBuffer)) ;
   gets(inputBuffer) ;
   inputBuffer[ossStrlen(inputBuffer)]=0 ;
   if ( SDB_OK != fromjson ( inputBuffer, inputObj ) )
   {
      printf ( "Invalid BSON Key\n" ) ;
      return 0 ;
   }
   mthMatcher matcher ;
   rc = matcher.loadPattern ( inputObj ) ;
   if ( rc )
   {
      printf("failed to load pattern\n") ;
      return 0 ;
   }
   const rtnPredicateSet &predSet = matcher.getPredicateSet() ;
   rtnPredicateList rtnList ( predSet, indexCB, 1 ) ;
   printf("rtnList = %s\n", rtnList.toString().c_str() ) ;
   rtnPredicateListIterator listIterator ( rtnList ) ;
   while ( true )
   {
      BSONObj dataobj ;
      printf("Please input record element: " ) ;
      ossMemset ( inputBuffer, 0, sizeof(inputBuffer)) ;
      gets(inputBuffer) ;
      inputBuffer[ossStrlen(inputBuffer)]=0 ;
      if ( SDB_OK != fromjson ( inputBuffer, dataobj ) )
      {
         printf ( "Invalid BSON Key\n" ) ;
         return 0 ;
      }
      INT32 result = listIterator.advance ( dataobj ) ;
      printf ("result = %d\n", result ) ;
   }
   return 0 ;
   INT32 dummyRid = 0 ;
   while ( dummyRid < 10000 )
   {
      ossMemset ( inputBuffer, 0, sizeof(inputBuffer) ) ;
      sprintf(inputBuffer, "{c1:%d}", abs(rand()) % 100 ) ;
      inputBuffer[ossStrlen(inputBuffer)]=0 ;
      BSONObj inputObj ;
      BSONObjSet keySet ;
      if ( SDB_OK != fromjson ( inputBuffer, inputObj ) )
      {
         printf ( "Invalid BSON Key\n" ) ;
         return 0 ;
      }
      rc = indexCB->getKeysFromObject ( inputObj, keySet ) ;
      if ( rc )
      {
         printf ("Failed to get keyset, rc = %d\n", rc ) ;
         rc = SDB_OK ;
         continue ;
      }
      BSONObjSet::iterator it ;
      vector<ixmIndexInsertRequestImpl *> insertRequests ;
      Ordering ordering = Ordering::make(indexCB->keyPattern()) ;
      for ( it = keySet.begin() ; it != keySet.end(); it++ )
      {
         ixmIndexInsertRequestImpl *impl = new
               ixmIndexInsertRequestImpl(dmsRecordID(0,2*dummyRid), (*it),
               ordering, indexCB) ;
         ixmExtent rootidx ( indexCB->getRoot(), myUnit.index()) ;
         if ( rc )
         {
            printf("Failed to run insertStepOne\n") ;
            delete impl ;
            continue ;
         }
         insertRequests.push_back(impl) ;
      }
      vector<ixmIndexInsertRequestImpl *>::iterator it1 ;
      for ( it1 = insertRequests.begin(); it1 != insertRequests.end(); it1++)
      {
         rc = (*it1)->performAction() ;
         if ( rc )
         {
            printf("Failed to perform action\n") ;
            return 0 ;
         }
         delete (*it1) ;
      }
      insertRequests.clear() ;
      dummyRid ++ ;
   }


   while ( dummyRid >= 0 )
   {
      ossMemset ( inputBuffer, 0, sizeof(inputBuffer) ) ;
      strcpy ( inputBuffer, "{c1:10}") ;
      inputBuffer[ossStrlen(inputBuffer)]=0 ;
      BSONObj inputObj ;
      BSONObjSet keySet ;
      if ( SDB_OK != fromjson ( inputBuffer, inputObj ) )
      {
         printf ( "Invalid BSON Object\n" ) ;
         continue ;
      }
      rc = indexCB->getKeysFromObject ( inputObj, keySet ) ;
      if ( rc )
      {
         printf ("Failed to get keyset, rc = %d\n", rc ) ;
         rc = SDB_OK ;
         continue ;
      }
      BSONObjSet::iterator it ;
      vector<ixmIndexInsertRequestImpl *> insertRequests ;
      Ordering ordering = Ordering::make(indexCB->keyPattern()) ;
      INT32 customRID ;
      printf("Please input rid: " );
      scanf ("%d", &customRID ) ;
      printf("Deleting rid %d\n", customRID) ;
      for ( it = keySet.begin() ; it != keySet.end(); it++ )
      {
         ixmExtent rootidx ( indexCB->getRoot(), myUnit.index()) ;
         BOOLEAN result ;
         rc = rootidx.unindex ( ixmKeyOwned(*it), dmsRecordID(0,2*customRID),
                                ordering, indexCB, result ) ;
         if ( rc )
         {
            printf ("unindex failed, rc = %d\n", rc ) ;
            return 0 ;
         }
      }
      dummyRid -- ;
   }

   myUnit.close() ;
   return 0;
}
