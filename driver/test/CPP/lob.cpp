/*******************************************************************************
*@Description : Test domain C++ driver, include createDomain/sdbDropDomain/
*               /sdbGetDomain/sdbListDomains/sdbAlterDomain/
*               /sdbListCollectionSpacesInDomain/sdbListCollectionsInDomain
*@Modify List :
*               2014-7-15   xiaojun Hu   Change [adb abnormal test]
*******************************************************************************/

#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

TEST( lob, not_connect )
{
   sdbLob lob ;
   INT32 rc = SDB_OK ;

   rc = lob.read( 0, NULL, NULL ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

TEST( lob, global )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   sdbLob lob ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   const UINT32 putNum = 1000 ;
   const UINT32 bufSize = 1000 ;
   CHAR buf[bufSize] = { 0 } ;
   CHAR readBuf[bufSize] = { 0 } ;
   BOOLEAN flag = FALSE ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.createLob( lob ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   flag = FALSE ;
   rc = lob.isClosed( flag ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, flag ) ;
   bson::OID oid ;
   rc = lob.getOid( oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "Auto build lob's oid is: " << oid.str().c_str() << endl ;
   memset( buf, 'a', bufSize ) ;
   rc = lob.write( buf, bufSize ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.close() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   flag = FALSE ;
   rc = lob.isClosed( flag ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, flag ) ;
   
   sdbLob lob2 ;
   rc = cl.openLob( lob2, oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   flag = FALSE ;
   rc = lob2.isClosed( flag ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, flag ) ;
   bson::OID oid2 ;
   rc = lob2.getOid( oid2 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "oid is: " << oid.str().c_str() << endl ;
   cout << "oid2 is: " << oid2.str().c_str() << endl ;
   ASSERT_EQ( 0, strncmp( (char *)oid.str().c_str(),
                              (char *)oid2.str().c_str(),
                              strlen( oid.str().c_str() ) ) ) ;
   SINT64 size = 0 ;
   rc = lob2.getSize( &size ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "lob's size is: " << size << endl ;
   ASSERT_EQ( bufSize, size ) ;   
   UINT64 time = 0 ;
   rc = lob2.getCreateTime( &time ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "lob's create time is: " << time << endl ;
   ASSERT_TRUE ( time > 0 ) ;
   UINT32 readNum = bufSize / 4 ;
   UINT32 retNum = 0 ;
   rc = lob2.read( readNum, readBuf, &retNum ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "going to read [%d] bytes from lob, actually read [%d] bytes\n",
           readNum, retNum ) ;
   ASSERT_EQ( readNum, retNum ) ;
   SINT64 offset = bufSize / 2 ;
   rc = lob2.seek( offset, SDB_LOB_SEEK_CUR ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   readNum = bufSize ;
   retNum = 0 ;
   rc = lob2.read( readNum, readBuf, &retNum ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "going to read [%d] bytes from lob, actually read [%d] bytes\n",
           readNum, retNum ) ;
   ASSERT_EQ( (bufSize / 4), retNum ) ;
   rc = lob2.close() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   flag = FALSE ;
   rc = lob2.isClosed( flag ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, flag ) ;
   
   sdbLob lob3 ;
   bson::OID oid3 = bson::OID::gen() ;
   rc = cl.createLob( lob3, &oid3 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson::OID oid4 ;
   rc = lob3.getOid( oid4 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "oid3 is: " << oid3.str().c_str() << endl ;
   cout << "oid4 is: " << oid4.str().c_str() << endl ;
   ASSERT_EQ( 0, strncmp( (char *)oid3.str().c_str(),
                              (char *)oid4.str().c_str(),
                              strlen( oid3.str().c_str() ) ) ) ;
   memset( buf, 'a', bufSize ) ;
   rc = lob3.write( buf, bufSize ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob3.close() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.listLobs( cur ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj record ;
   INT32 i = 0 ;
   while( !( cur.next( record ) ) )
   {
      BSONElement ele ;
      BSONType bType ;
      UINT64 lobSize = 0 ;
      i++ ;
      cout << "record is: " << record.toString(FALSE, TRUE).c_str() << endl ;
      ele = record.getField( "Size" ) ;
      bType = ele.type() ;
      if ( NumberInt == bType || NumberLong == bType )
      {
         lobSize = ele.numberLong() ;
      }
      else
      {
          ASSERT_TRUE( FALSE ) ;
      }
      printf( "bufSize is [%d], lobSize is[%d]\n", bufSize, lobSize ) ;
      ASSERT_EQ ( bufSize, lobSize ) ;
   }
   ASSERT_EQ( 2, i ) ;
   rc = cl.removeLob( oid3 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.listLobs( cur ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   i = 0 ;
   while ( !( cur.next( record ) ) )
   {
      i++ ;
   }
   cout << "i is: " << i << endl ;
   ASSERT_EQ( 1, i ) ;

   db.disconnect() ;

}

TEST( lob, lob_connection_was_destruct )
{
   INT32 rc = SDB_OK ;
   sdbLob lob ;
   {
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   const UINT32 putNum = 1000 ;
   const UINT32 bufSize = 1000 ;
   CHAR buf[bufSize] = { 0 } ;
   CHAR readBuf[bufSize] = { 0 } ;
   BOOLEAN flag = FALSE ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   rc = cl.createLob( lob ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson::OID oid ;
   rc = lob.getOid( oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "Auto build lob's oid is: " << oid.str().c_str() << endl ;
   db.disconnect() ;
   }
   sleep( 5 ) ;
   rc = lob.close() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;   
}

TEST( lob, lobWriteZeroSizeAndRead )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   sdbLob lob ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   const UINT32 putNum = 1000 ;
   const UINT32 bufSize = 1000 ;
   CHAR buf[bufSize] = { 0 } ;
   CHAR readBuf[bufSize] = { 0 } ;
   BOOLEAN flag = FALSE ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   rc = cl.createLob( lob ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   memset( buf, 'a', bufSize ) ;
   rc = lob.write( buf, 0 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson::OID oid ;
   rc = lob.getOid( oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = lob.close() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( rc, SDB_OK ) ;

   sdbLob lob2 ;
   rc = cl.openLob( lob2, oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   UINT32 readNum = 0 ;
   UINT32 retNum = 0 ;
   rc = lob2.read( readNum, readBuf, &retNum ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, retNum ) ;

   db.disconnect() ;
}

TEST( lob, lob_write_not_close )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   bson::OID oid ;

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   const UINT32 putNum = 1000 ;
   const UINT32 bufSize = 1000 ;
   CHAR buf[bufSize] = { 0 } ;
   CHAR readBuf[bufSize] = { 0 } ;
   BOOLEAN flag = FALSE ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   {
   sdbLob lob ;
   rc = cl.createLob( lob ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.getOid( oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "Auto build lob's oid is: " << oid.str().c_str() << endl ;
   memset( buf, 'a', bufSize ) ;
   rc = lob.write( buf, bufSize ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
   rc = cl.listLobs( cur ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj record ;
   while( !( cur.next( record ) ) )
   {
      BSONElement ele ;
      BSONType bType ;
      BOOLEAN flag = 0 ;
      cout << "record is: " << record.toString(FALSE, TRUE).c_str() << endl ;
      ele = record.getField( "Available" ) ;
      bType = ele.type() ;
      if ( Bool == bType )
      {
         flag = (BOOLEAN)ele.Bool() ;
      }
      else
      {
          ASSERT_TRUE( FALSE ) ;
      }
      ASSERT_EQ ( TRUE, flag ) ;
   }

   {
   sdbLob lob2 ;
   rc = cl.openLob( lob2, oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   UINT32 readNum = bufSize / 4 ;
   UINT32 retNum = 0 ;
   rc = lob2.read( readNum, readBuf, &retNum ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "going to read [%d] bytes from lob, actually read [%d] bytes\n",
           readNum, retNum ) ;
   ASSERT_EQ( readNum, retNum ) ;
   rc = cl.listLobs( cur ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj record ;
   while( !( cur.next( record ) ) )
   {
      BSONElement ele ;
      BSONType bType ;
      BOOLEAN flag = 0 ;
      cout << "record is: " << record.toString(FALSE, TRUE).c_str() << endl ;
      ele = record.getField( "Available" ) ;
      bType = ele.type() ;
      if ( Bool == bType )
      {
         flag = (BOOLEAN)ele.Bool() ;
      }
      else
      {
          ASSERT_TRUE( FALSE ) ;
      }
      ASSERT_EQ ( TRUE, flag ) ;
   }
   }
   rc = cl.listLobs( cur ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   while( !( cur.next( record ) ) )
   {
      BSONElement ele ;
      BSONType bType ;
      BOOLEAN flag = 0 ;
      cout << "record is: " << record.toString(FALSE, TRUE).c_str() << endl ;
      ele = record.getField( "Available" ) ;
      bType = ele.type() ;
      if ( Bool == bType )
      {
         flag = (BOOLEAN)ele.Bool() ;
      }
      else
      {
          ASSERT_TRUE( FALSE ) ;
      }
      ASSERT_EQ ( TRUE, flag ) ;
   }

   db.disconnect() ;
}

TEST( lob, lob_write_getSize_getCreateTime_then_close )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   bson::OID oid ;

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   const UINT32 putNum = 1000 ;
   const UINT32 bufSize = 1000 ;
   CHAR buf[bufSize] = { 0 } ;
   CHAR readBuf[bufSize] = { 0 } ;
   BOOLEAN flag      = FALSE ;
   INT32 writeNum    = 0 ;
   INT32 lobSize     = 0 ;
   INT32 lobSize2    = 0 ;
   INT32 createTime  = 0 ;
   INT32 createTime2 = 0 ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbLob lob ;
   lobSize = lob.getSize();
   ASSERT_EQ(-1, lobSize);
   createTime = lob.getCreateTime();
   ASSERT_EQ(-1, createTime);
   rc = cl.createLob( lob ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.getOid( oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "Auto build lob's oid is: " << oid.str().c_str() << endl ;
   lobSize = lob.getSize();
   ASSERT_EQ(0, lobSize);
   createTime = lob.getCreateTime();
   ASSERT_EQ(0, createTime);
   memset( buf, 'a', bufSize ) ;
   rc = lob.write( buf, bufSize ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   writeNum += bufSize ;
   lobSize = lob.getSize();
   ASSERT_EQ(lobSize, bufSize);
   rc = lob.write( buf, bufSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   writeNum += bufSize ;
   lobSize = lob.getSize();
   ASSERT_EQ(lobSize, writeNum);
   createTime = lob.getCreateTime();
   ASSERT_EQ(0, createTime);
   rc = lob.close();
   ASSERT_EQ(SDB_OK, rc);
   lobSize = lob.getSize();
   ASSERT_EQ(writeNum, lobSize);
   createTime2 = lob.getCreateTime();
   ASSERT_EQ(createTime, createTime2);

   db.disconnect() ;
}

TEST( lob, lobWithReturnData )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   sdbLob lob ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   #define BUFSIZE1 (1024 * 1024 * 3)
   #define BUFSIZE2 (1024 * 1024 * 2)
   CHAR buf[BUFSIZE1] = { 0 } ;
   CHAR readBuf[BUFSIZE2] = { 0 } ;
   BOOLEAN flag = FALSE ;
   CHAR c = 'a' ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   rc = cl.createLob( lob ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   memset( buf, c, BUFSIZE1 ) ;
   rc = lob.write( buf, BUFSIZE1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.write( buf, BUFSIZE1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson::OID oid ;
   rc = lob.getOid( oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = lob.close() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( rc, SDB_OK ) ;

   sdbLob lob2 ;
   rc = cl.openLob( lob2, oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   UINT32 readNum = 1000 ;
   UINT32 retNum = 0 ;
   rc = lob2.read( readNum, readBuf, &retNum ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( readNum, retNum ) ;
   for ( INT32 i = 0; i < retNum; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) ;
   }
   readNum = BUFSIZE2 ;
   rc = lob2.read( readNum, readBuf, &retNum ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( readNum, retNum ) ;
   for ( INT32 i = 0; i < retNum; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) ;
   }
   rc = lob.close() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   rc = cl.removeLob( oid ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   db.disconnect() ;
}

static INT32 putData( UINT32 putSize, CHAR *buffer )
{
   CHAR buf[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ;
   CHAR dest[27] = {0} ;
   UINT32 putNum = 0 ;
   UINT32 putTmp = 0 ;
   UINT32 i = 1 ;
   CHAR *tmpBuf = NULL ;
   UINT32 allocSize = 64*1024*1024 ;
   if( NULL == ( tmpBuf = (CHAR *)malloc( allocSize ) ) )
   {
      perror( "tmpBuf" ) ;
      exit(1) ;
   }
   memset( tmpBuf, 0 , allocSize ) ;
   sprintf( tmpBuf, "%s", buf ) ;
   do
   {
      putNum = 26*( 1 << i ) ;
      if( putNum <= putSize )
      {
         sprintf( tmpBuf, "%s%s", tmpBuf, tmpBuf ) ;
      }
      else
      {
         if( 1 == i )
         {
            sprintf( tmpBuf, "%s", buf ) ;
            putTmp = 26 ;
         }
         putSize -= putTmp ;
         if( putSize > 26 )
         {
            sprintf( buffer, "%s%s", buffer, tmpBuf ) ;
            putSize = putData( putSize, buffer ) ;
         }
         else
         {
            sprintf( buffer, "%s%s", buffer, tmpBuf ) ;
            memcpy( dest, buf, putSize%26 ) ;
            strcat( buffer, dest ) ;
            putSize = 0 ;
         }
      }
      putTmp = putNum ;
      ++i ;
   }while( 0 != putSize ) ;
   free( tmpBuf ) ;
   tmpBuf = NULL ;
   return putSize ;
}

void genLobData( CHAR *lobWriteBuf, UINT64 size )
{
   const CHAR *head = "B==head:" ;
   const CHAR *tail = ":tail==E" ;
   const CHAR buf[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ;
   CHAR *buffer = NULL ;
   UINT32 allocSize = 64*1024*1024 ;
   UINT32 putSize = 0 ;
   UINT32 putNum = 0 ;
   UINT32 putTmp= 0 ;
   UINT32 rc = SDB_OK ;
   UINT64 i ;
   if( NULL == ( buffer = (CHAR *)malloc( allocSize ) ) )
   {
      perror( "buffer" ) ;
      exit(1) ;
   }
   UINT64 bodySize = size-(strlen(head)+strlen(tail)) ;
   memset( lobWriteBuf, 0, size ) ;
   for( i = 0 ; i <= bodySize/allocSize ; ++i )
   {
      memset( buffer, 0, allocSize ) ;
      if( i == bodySize/allocSize )
         putSize = bodySize%allocSize ;
      else
         putSize = allocSize ;
      rc = putData( putSize, buffer ) ;
      if( SDB_OK != rc )
         printf( "Failed to put data\n" ) ;
      sprintf( lobWriteBuf, "%s%s", lobWriteBuf, buffer ) ;
   }
   sprintf( lobWriteBuf, "%s%s%s", head, lobWriteBuf, tail ) ;
   free( buffer ) ;
   buffer = NULL ;
}

/*******************************************************************************
*@Description : test write lob buffer equal 'NULL' and ''
*@Modify List :
*               2014-10-22   xiaojun Hu   Change [adb abnormal test]
*******************************************************************************/
TEST( lob, lobWriteZeroSize )
{
   INT32 rc = SDB_OK ;
   const UINT64 lobSize = 20 ;
   const CHAR *csName = "sdb_clientcpp_collection_test" ;
   const CHAR *clName = "sdb_query_limit_one" ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   sdbLob lob, lob1 ;

   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.dropCollectionSpace( csName ) ;
   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_DEFAULT, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj obj = BSON( "ReplSize" << 0 ) ;   //replsize = 0
   rc = cs.createCollection( clName, obj, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   /*********************************************************
   * [TestPoint_1: write a lob that buffer equal NULL]
   *********************************************************/
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   CHAR *buffer = NULL ;
   rc = lob.write( buffer, 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;   // should throw error
   UINT32 len = 0 ;
   UINT32 readLen = 0 ;
   rc = lob.read( len, buffer, &readLen ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   cout << "success to test write lob, which lob buffer equal 'NULL'" << endl ;
   /*********************************************************
   * [TestPoint_1: write a lob that buffer equal NULL]
   *********************************************************/
   rc = cl.createLob( lob1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   buffer = "" ;
   rc = lob.write( buffer, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson::OID oid ;
   rc = lob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   len = 0 ;
   rc = cl.openLob( lob1, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob1.read( len, buffer, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( SDB_OK, readLen ) ;   // readLen need equal 0
   rc = lob1.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test write lob, which lob buffer equal ''" << endl ;
   db.disconnect() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   db.disconnect() ;
}

/*******************************************************************************
*@Description : test lob api in cpp driver.Use api do basic operation
*@Modify List :
*               2014-10-22   xiaojun Hu   Change
*******************************************************************************/
TEST( lob, lobApiBasicOperation )
{
   INT32 rc = SDB_OK ;
   const CHAR *csName = "sdb_clientcpp_collection_test" ;
   const CHAR *clName = "sdb_query_limit_one" ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   sdb db ;
   const UINT64 lobSize = 17*1024*1024 ;
   CHAR *lobBuffer = NULL ;
   const UINT32 putNum = 10 ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   sdbLob lob, lob1 ;
   bson::OID oids[putNum] ;

   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.dropCollectionSpace( csName ) ;
   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_DEFAULT, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj obj = BSON( "ReplSize" << 0 ) ;   //replsize = 0
   rc = cs.createCollection( clName, obj, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if( NULL == ( lobBuffer = (CHAR*)calloc( lobSize, sizeof(char) ) ) )
   {
      perror( "lobBuffer" ) ;
      ASSERT_TRUE( false ) ;
   }
   genLobData( lobBuffer, lobSize ) ;
   for( INT32 i = 0 ; i < putNum ; ++ i )
   {
      rc = cl.createLob( lob ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = lob.write( lobBuffer, lobSize ) ;
      ASSERT_EQ( SDB_OK, rc ) ;   // should throw error
      bson::OID oid ;
      rc = lob.getOid( oid ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      oids[i] = oid ;
      rc = lob.close() ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   cout << "success to write lob" << endl ;
   UINT32 readLen = 0 ;
   for( INT32 i = 0 ; i < putNum ; ++ i )
   {
      memset( lobBuffer, 0, lobSize ) ;
      UINT32 len = 0 ;
      bson::OID oid ;
      oid = oids[i] ;
      rc = cl.openLob( lob, oid ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = lob.read( lobSize, lobBuffer, &readLen ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( lobSize, readLen ) ;
   }
   cout << "success to read lob" << endl ;
   readLen = 0 ;
   for( INT32 i = 0 ; i < putNum ; ++ i )
   {
      memset( lobBuffer, 0, lobSize ) ;
      SINT64 seekSz = 100*i+79 ;
      bson::OID oid ;
      oid = oids[i] ;
      rc = cl.openLob( lob , oid ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = lob.seek( seekSz, SDB_LOB_SEEK_SET ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = lob.read( lobSize, lobBuffer, &readLen ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( readLen, (lobSize-seekSz) ) ;
   }
   cout << "success to seek read lob" << endl ;
   for( INT32 i = 0 ; i < putNum ; ++ i )
   {
      bson::OID oid ;
      oid = oids[i] ;
      rc = cl.openLob( lob, oid ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      UINT64 millis = 0 ;
      rc = lob.getCreateTime( &millis ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      BOOLEAN flag = false ;
      rc = lob.isClosed( flag ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      if( false == flag )
      {
         rc = lob.close() ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }
   }
   cout << "success to get lob create time" << endl ;
   rc = cl.listLobs( cur ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   INT32 cnt = 0 ;
   BSONObj nobj ;
   while( SDB_OK == cur.next( nobj ) )
   {
      cnt++ ;
   }
   ASSERT_EQ( putNum, cnt ) ;
   cout << "success to list lobs" << endl ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   free( lobBuffer ) ;
   lobBuffer = NULL ;
   db.disconnect() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   db.disconnect() ;
}

/*******************************************************************************
*@Description : Test Open and Close not exist LOB.
*               { TestCase Number : LOB.ABNORMAL_TEST.001 }
*@Modify List :
*               2014-8-20   xiaojun Hu   Init
*******************************************************************************/
TEST( lob, NotExistLob )
{

   INT32 rc = SDB_OK ;
   const CHAR *csName = "sdb_clientcpp_collection_test" ;
   const CHAR *clName = "sdb_query_limit_one" ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   sdb db ;
   const UINT64 lobSize = 17*1024*1024 ;
   CHAR *lobBuffer = NULL ;
   const UINT32 putNum = 10 ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   sdbLob lob, lob1 ;
   bson::OID oids[putNum] ;


   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.dropCollectionSpace( csName ) ;
   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_DEFAULT, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj obj = BSON( "ReplSize" << 0 ) ;   //replsize = 0
   rc = cs.createCollection( clName, obj, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if( NULL == ( lobBuffer = (CHAR*)calloc( lobSize, sizeof(char) ) ) )
   {
      perror( "lobBuffer" ) ;
      ASSERT_TRUE( false ) ;
   }
   genLobData( lobBuffer, lobSize ) ;

   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.write( lobBuffer, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to write lob" << endl ;
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs.createCollection( clName, obj, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson::OID oid ;
   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_FNE, rc ) ;
   SINT64 getSize = 0 ;
   rc = lob.getSize( &getSize ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
   UINT64 millis = 0 ;
   rc = lob.getCreateTime( &millis ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.removeLob( oid ) ;
   ASSERT_EQ( SDB_FNE, rc ) ;
   cout << "success to test not exist lob" << endl ;

   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   free( lobBuffer ) ;
   lobBuffer = NULL ;
   db.disconnect() ;
}

TEST( lob, use_lob_after_close_contexts )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cur ;
   sdbLob lob ;
   const CHAR *pHostName    = HOST ;
   const CHAR *pPort        = SERVER ;
   const CHAR *pUsr         = USER ;
   const CHAR *pPasswd      = PASSWD ;
   INT32 rc                 = SDB_OK ;

   UINT64 createTime        = 0 ;
   UINT64 createTime2       = 0 ;
   SINT64 lobSize           = 0 ;
   SINT64 lobSize2          = 0 ;
   bson::OID oid ;
   bson::OID oid2 ;
#define writeBuffSize (2 * 1024 * 1024)
#define readBuffSize (writeBuffSize/2)
   CHAR buf[10]                  = { 0 } ;
   CHAR writeBuff[writeBuffSize] = { 0 };
   CHAR readBuff[readBuffSize]   = { 0 };
   UINT32 readNum                = 0 ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.createLob( lob ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   oid = lob.getOid() ;
   createTime = lob.getCreateTime() ;
   lobSize = lob.getSize() ;

   rc = lob.write( buf, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   lobSize += 10 ;

   rc = db.closeAllCursors() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = lob.write( buf, 10 ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;

   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BOOLEAN flag = FALSE ;
   rc = lob.isClosed( flag ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( TRUE, flag ) ;

   oid2 = lob.getOid() ;
   createTime2 = lob.getCreateTime() ;
   lobSize2 = lob.getSize() ;

   ASSERT_EQ(0, oid.compare(oid2)) ;
   ASSERT_EQ(createTime, createTime2) ;
   ASSERT_EQ(lobSize, lobSize2) ;


   rc = cl.createLob( lob ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.write( writeBuff, writeBuffSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   oid = lob.getOid() ;

   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   createTime = lob.getCreateTime() ;
   lobSize = lob.getSize() ;
   rc = lob.read( readBuffSize, readBuff, &readNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.closeAllCursors() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   flag = FALSE ;
   rc = lob.isClosed( flag ) ;
   ASSERT_EQ( TRUE, flag ) ;
   rc = lob.seek( 10, SDB_LOB_SEEK_CUR ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = lob.read( readBuffSize, readBuff, &readNum ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;

   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   oid2 = lob.getOid() ;
   createTime2 = lob.getCreateTime() ;
   lobSize2 = lob.getSize() ;

   ASSERT_EQ(0, oid.compare(oid2)) ;
   ASSERT_EQ(createTime, createTime2) ;
   ASSERT_EQ(lobSize, lobSize2) ;
}

