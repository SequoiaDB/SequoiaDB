#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"
#include <iostream>
#include <pthread.h>

TEST(lob, lob_global_test)
{
   INT32 rc = SDB_OK ;
   BOOLEAN eof = FALSE ;
   INT32 counter = 0 ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   sdbCursorHandle cur               = 0 ;
   sdbLobHandle lob                  = 0 ;
   INT32 NUM                         = 10 ;
   SINT64 count                      = 0 ;
   bson_oid_t oid ;
   bson obj ;
   #define BUFSIZE1 (1024 * 1024 * 3)
   //#define BUFSIZE1 ( 1024 * 2 )
   #define BUFSIZE2 (1024 * 1024 * 2)
   SINT64 lobSize = -1 ;
   UINT64 createTime = -1 ;
   CHAR buf[BUFSIZE1] = { 0 } ;
   CHAR readBuf[BUFSIZE2] = { 0 } ;
   UINT32 readCount = 0 ;
   CHAR c = 'a' ;
   for ( INT32 i = 0; i < BUFSIZE1; i++ )
   {
      buf[i] = c ;
   }
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // open lob 
   bson_oid_gen( &oid ) ; 
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get lob size 
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, lobSize ) ;
   // get lob create time
//   rc = sdbGetLobCreateTime( lob, &createTime ) ;
//   ASSERT_EQ( 0, createTime ) ;
//   ASSERT_EQ( 0, createTime ) ;
   // write lob 
   rc = sdbWriteLob( lob, buf, BUFSIZE1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get lob size 
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BUFSIZE1, lobSize ) ;
   // write lob
   rc = sdbWriteLob( lob, buf, BUFSIZE1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get lob size
   rc = sdbGetLobSize( lob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 2 * BUFSIZE1, lobSize ) ;
   // close lob
   rc = sdbCloseLob ( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // open lob with the mode SDB_LOB_READ
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   // read lob
   rc = sdbReadLob( lob, 1000, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) ;
   }
   // read lob
   rc = sdbReadLob( lob, BUFSIZE2, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) << "readCount is: " << readCount 
         << ", c is: " << c << ", i is: " 
         << i << ", readBuf[i] is: " << readBuf[i] ;
   }
   // close lob
   rc = sdbCloseLob ( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // reopen it, and read all the content
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = SDB_OK ;
   counter = 0 ;
   while( SDB_EOF != ( rc = sdbReadLob( lob, BUFSIZE2, readBuf, &readCount ) ) )
   {
       eof = sdbLobIsEof( lob ) ;
       ASSERT_EQ( SDB_OK, rc ) ;
       if ( TRUE == eof )
       {
          counter = 1 ;
       }
   }
   ASSERT_EQ( 1, counter ) ;
   //eof = sdbLobIsEof( lob ) ;
   //ASSERT_EQ( SDB_OK, rc ) ;
   //ASSERT_EQ( TRUE, eof ) ;
   // close lob
   rc = sdbCloseLob ( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // remove lob
   rc = sdbRemoveLob( cl, &oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCursor ( cur ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
}

TEST(lob, lob_createLOBID_test)
{
   INT32 rc = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   bson_oid_t oid ;
   CHAR * pTimeStamp                 = NULL;

   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl) ;
   CHECK_MSG("%s%d\n"," getCollection rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // createLOBID

   //case 1, pTimeStamp = NULL;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   //case 2, pTimeStamp = "2019-07-23-18.04.07";   
   pTimeStamp = "2019-07-23-18.04.07" ;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   //case 3, pTimeStamp = "2019-07";   
   pTimeStamp = "2019-07" ;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   //case 4, pTimeStamp = "2019-07-23-18";   
   pTimeStamp = "2019-07-23-18" ;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;


   //case 5, sdbCreateLobID
   rc = sdbCreateLobID( cl, &oid) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;

}


TEST(lob,lob_createLob_test)
{
   INT32 rc = SDB_OK ;
   BOOLEAN eof = FALSE ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   bson_oid_t oid ;
   bson_oid_t oid1 ;
   bson_oid_t oid2 ;
   sdbLobHandle lob1                 = 0 ;
   sdbLobHandle lob2                 = 0 ;
   sdbLobHandle lob3                 = 0 ;
   sdbLobHandle lob4                 = 0 ;
   sdbLobHandle lob5                 = 0 ;
   CHAR * pTimeStamp                 = NULL;
   sdbCursorHandle cursor            = 0;
   
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection

   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl) ;
   ASSERT_EQ( SDB_OK, rc ) ;


  // case 1, use bson_oid_gen()
   bson_oid_gen( &oid1 );
   rc = sdbOpenLob(cl, &oid1, SDB_LOB_CREATEONLY, &lob1);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_oid_t lob_oid1 ;
   rc = sdbGetLobId(lob1, &lob_oid1) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_STREQ(oid1.bytes,lob_oid1.bytes);
   rc = sdbCloseLob(&lob1);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbOpenLob(cl, &oid1, SDB_LOB_READ, &lob1);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseLob(&lob1);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbOpenLob(cl, &lob_oid1, SDB_LOB_READ, &lob1);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseLob(&lob1);
   ASSERT_EQ( SDB_OK, rc ) ;


   // case 2, oid is NULL 
   //rc = sdbCreateLob(cl, NULL, &lob2);
   rc = sdbOpenLob(cl, NULL, SDB_LOB_CREATEONLY, &lob2);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_oid_t lob_oid ;
   rc = sdbGetLobId(lob2, &lob_oid) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   CHECK_MSG("%s%d\n","lob_oid = ", lob_oid) ;

   rc = sdbCloseLob(&lob2);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbOpenLob(cl, &lob_oid, SDB_LOB_READ, &lob2);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseLob(&lob2);
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 3, use bson_oid_gen()
    bson_oid_gen( &oid2 );
    rc = sdbOpenLob(cl, &oid2, SDB_LOB_CREATEONLY, &lob3);
    ASSERT_EQ( SDB_OK, rc ) ;

    bson_oid_t lob_oid2 ;
    rc = sdbGetLobId(lob3, &lob_oid2) ;
    ASSERT_EQ( SDB_OK, rc ) ;

    ASSERT_STREQ(oid2.bytes,lob_oid2.bytes);
    rc = sdbCloseLob(&lob3);
    ASSERT_EQ( SDB_OK, rc ) ;
    rc = sdbOpenLob(cl, &oid2, SDB_LOB_READ, &lob3);
    ASSERT_EQ( SDB_OK, rc ) ;
    rc = sdbCloseLob(&lob3);
    ASSERT_EQ( SDB_OK, rc ) ;
    rc = sdbOpenLob(cl, &lob_oid2, SDB_LOB_READ, &lob3);
    ASSERT_EQ( SDB_OK, rc ) ;
    rc = sdbCloseLob(&lob3);
    ASSERT_EQ( SDB_OK, rc ) ;


   // case 4, use sdbCreateLobID to gen oid
   pTimeStamp = "2019-07-23-18.04.07" ;
   rc = sdbCreateLobID1(cl,pTimeStamp, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   //rc = sdbCreateLob(cl, &oid , &lob3);
   rc = sdbOpenLob(cl, &oid, SDB_LOB_CREATEONLY, &lob4);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob(&lob4);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbOpenLob(cl, &oid, SDB_LOB_READ, &lob4);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseLob(&lob4);
   ASSERT_EQ( SDB_OK, rc ) ;


   // case 5, use sdbCreateLobID to gen oid
   rc = sdbCreateLobID(cl, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   //rc = sdbCreateLob(cl, &oid , &lob4);
   rc = sdbOpenLob(cl, &oid, SDB_LOB_CREATEONLY, &lob5);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob(&lob5);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbOpenLob(cl, &oid, SDB_LOB_READ, &lob5);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob(&lob5);
   ASSERT_EQ( SDB_OK, rc ) ;

   // display had create Lob
   rc = sdbListLobs(cl, &cursor);
   ASSERT_EQ( SDB_OK, rc ) ;
   
   printf("#######display had create Lob########\n");
   displayRecord( &cursor ) ;
   sdbReleaseCursor( cursor ) ;

   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;

}

TEST(lob,lob_listLobs_test)
{
   INT32 rc = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   bson condition                    ;
   bson selected                     ;
   bson orderBy                      ;
   bson hint                         ;
   INT64 numToSkip                   = 0 ;
   INT64 numToReturn                 = -1 ;
   sdbCursorHandle cursor            = 0;
   sdbCursorHandle lobPiecesCursor   = 0;
   sdbLobHandle lob                 = 0 ;

   
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl) ;

   ASSERT_EQ( SDB_OK, rc ) ;


   // create lob;
   for ( int i=0; i < 10; i++)
   {
      rc = sdbOpenLob(cl, NULL, SDB_LOB_CREATEONLY, &lob);
      ASSERT_EQ( SDB_OK, rc ) ;

      rc = sdbCloseLob( &lob );
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   

   // case 1, without options. 
   rc = sdbListLobs(cl, &cursor);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &cursor ) ;
   sdbReleaseCursor ( cursor ) ;

   rc = sdbListLobPieces(cl, &lobPiecesCursor);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &lobPiecesCursor ) ;
   sdbReleaseCursor ( lobPiecesCursor ) ;

   bson_init( &condition );
   bson_init( &selected );
   bson_init( &orderBy );
   bson_init( &hint );
   
   bson_append_code(&selected, "oid", "");

   bson_finish( &condition );
   bson_finish( &selected );
   bson_finish( &orderBy );
   bson_finish( &hint );

   // case 2, have options.
   rc = sdbListLobs1(cl, &condition, &selected, &orderBy, &hint, numToSkip, numToReturn, &cursor);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &cursor ) ;
   sdbReleaseCursor ( cursor ) ;


   rc = sdbListLobPieces1(cl, &condition, &selected, &orderBy, &hint, numToSkip, numToReturn, &lobPiecesCursor);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &condition ) ;
   bson_destroy( &selected ) ;
   bson_destroy( &orderBy ) ;
   bson_destroy( &hint ) ;

   displayRecord( &lobPiecesCursor ) ;
   sdbReleaseCursor ( lobPiecesCursor ) ;

   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
}

TEST(lob,lob_primaryAndSubLob_test)
{
   INT32 rc = SDB_OK ;
   BOOLEAN eof = FALSE ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCSHandle cs                    = 0 ;
   sdbCollectionHandle primaryCL     = 0 ;
   sdbCollectionHandle subACL        = 0 ;
   sdbCollectionHandle subBCL        = 0 ;
   sdbCursorHandle cur               = 0 ;
   sdbLobHandle primaryLob           = 0 ;
   sdbLobHandle subLob               = 0 ;
   bson_oid_t oidB;
   sdbLobHandle subBLOb              = 0 ;
   INT32 NUM                         = 10 ;
   SINT64 count                      = 0 ;
   bson_oid_t oid ;
   bson options ;
   #define BUFSIZE1 (1024 * 1024 * 3)
   //#define BUFSIZE1 ( 1024 * 2 )
   #define BUFSIZE2 (1024 * 1024 * 2)
   SINT64 lobSize = -1 ;
   UINT64 createTime = -1 ;
   CHAR buf[BUFSIZE1] = { 0 } ;
   CHAR readBuf[BUFSIZE2] = { 0 } ;
   UINT32 readCount = 0 ;
   CHAR c = 'a' ;
   for ( INT32 i = 0; i < BUFSIZE1; i++ )
   {
      buf[i] = c ;
   }
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get sub cl.
   rc = getCollection ( db, SUB_A_COLLECTION_FULL_NAME, &subACL) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = getCollection ( db, SUB_B_COLLECTION_FULL_NAME, &subBCL) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // create primary cl.
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, &cs) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_init( &options );

   bson shardingKey;
   bson_init( &shardingKey );
   bson_append_int( &shardingKey, "date", 1);
   bson_finish( &shardingKey) ;
   
   bson_append_string( &options, "LobShardingKeyFormat", "YYYYMMDD") ;
   bson_append_bson( &options, "ShardingKey", &shardingKey);
   bson_append_string( &options, "ShardingType", "range") ;
   bson_append_bool( &options, "IsMainCL", TRUE);
   bson_finish( &options) ;
   
   rc = sdbCreateCollection1( cs, PRIMARY_COLLECTION_NAME, &options, &primaryCL);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson attach_options;
   bson LowBound;
   bson UpBound;
   bson_init( &attach_options );
   bson_init( &LowBound );
   bson_init( &UpBound );

   bson_append_string( &LowBound, "date", "20190701") ;
   bson_append_string( &UpBound , "date", "20190801") ;
   
   bson_finish( &LowBound) ;
   bson_finish( &UpBound) ;

   bson_append_bson( &attach_options, "LowBound", &LowBound);
   bson_append_bson( &attach_options, "UpBound", &UpBound);

   bson_finish( &attach_options) ;

   // attach subACL
   rc = sdbAttachCollection( primaryCL, SUB_A_COLLECTION_FULL_NAME, &attach_options);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &attach_options );
   bson_destroy( &LowBound );
   bson_destroy( &UpBound );
   
   bson_init( &attach_options );
   bson_init( &LowBound );
   bson_init( &UpBound );

   bson_append_string( &LowBound, "date", "20190801") ;
   bson_append_string( &UpBound , "date", "20190901") ;
   
   bson_finish( &LowBound) ;
   bson_finish( &UpBound) ;

   bson_append_bson( &attach_options, "LowBound", &LowBound);
   bson_append_bson( &attach_options, "UpBound", &UpBound);

   bson_finish( &attach_options ) ;

   // attach subBCL
   rc = sdbAttachCollection( primaryCL, SUB_B_COLLECTION_FULL_NAME, &attach_options);
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &attach_options ) ;
   bson_destroy( &LowBound );
   bson_destroy( &UpBound );

   CHAR * pTimeStamp = "2019-07-23-18.04.07";

   rc = sdbCreateLobID1(primaryCL,pTimeStamp, &oid);
   ASSERT_EQ( SDB_OK, rc ) ;

   // createLOB in primaryCL
   printf("#####createLob in primaryCL #####\n");
   //rc = sdbCreateLob(primaryCL, &oid, &primaryLob);
   rc = sdbOpenLob(primaryCL, &oid, SDB_LOB_CREATEONLY, &primaryLob);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob( &primaryLob);
   CHECK_MSG("%s%d\n"," sdbCloseLob rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // openLob in subACL
   printf("#####openLob in subACL #####\n");
   rc = sdbOpenLob(subACL, &oid, SDB_LOB_WRITE, &subLob);
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // write in subACL
   printf("#####write in subACL #####\n");
   rc = sdbGetLobSize( subLob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, lobSize ) ;
   
   rc = sdbWriteLob( subLob, buf, BUFSIZE1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   eof = sdbLobIsEof( subLob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, eof ) ;
   // get subACL size 
   rc = sdbGetLobSize( subLob, &lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( BUFSIZE1, lobSize ) ;
   // close subACL
   rc = sdbCloseLob ( &subLob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // read in primaryCL
   printf("#####read in primaryLob #####\n");

   rc = sdbOpenLob(primaryCL, &oid, SDB_LOB_READ, &primaryLob);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbReadLob( primaryLob, 1000, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( primaryLob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) ;
   }
   // read subACL
   rc = sdbReadLob( primaryLob, BUFSIZE2, readBuf, &readCount ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( readCount > 0 ) ;
   eof = sdbLobIsEof( primaryLob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, eof ) ;
   for ( INT32 i = 0; i < readCount; i++ )
   {
      ASSERT_EQ( c, readBuf[i] ) << "readCount is: " << readCount 
         << ", c is: " << c << ", i is: " 
         << i << ", readBuf[i] is: " << readBuf[i] ;
   }
   // close subCL
   rc = sdbCloseLob ( &primaryLob) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   pTimeStamp = "2019-08-23-18.04.07";
   rc = sdbCreateLobID1(subBCL,pTimeStamp, &oidB);
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("#####CreateLob in subBCL #####\n");
   //rc = sdbCreateLob(subBCL, &oidB, &subBLOb);
   rc = sdbOpenLob(subBCL, &oidB, SDB_LOB_CREATEONLY, &subBLOb);
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCloseLob( &subBLOb );
   ASSERT_EQ( SDB_OK, rc ) ;

   printf("#####sdbListLobs in primaryCL #####\n");
   rc = sdbListLobs(primaryCL, &cur);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &cur ) ;
   sdbReleaseCursor ( cur ) ;

   printf("#####sdbRemoveLob in primaryCL #####\n");
   rc = sdbRemoveLob(primaryCL, &oidB);
   ASSERT_EQ( SDB_OK, rc ) ;

   printf("#####sdbOpenLob in subBCL #####\n");
   rc = sdbOpenLob(subBCL , &oidB, SDB_LOB_READ , &subBLOb);
   ASSERT_EQ( SDB_FNE, rc ) ;

   printf("#####sdbListLobs in primaryCL #####\n");
   rc = sdbListLobs(primaryCL, &cur);
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( &cur ) ;
   sdbReleaseCursor ( cur ) ;


   bson_destroy( &options);
   bson_destroy( &shardingKey);
   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( primaryCL) ;
   sdbReleaseCollection ( subACL ) ;
   sdbReleaseCollection ( subBCL ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection ( db ) ;

}

TEST(lob,lob_readAndWrite_mode_test)
{
   INT32 rc = SDB_OK ;
   BOOLEAN eof = FALSE ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   bson_oid_t oid ;
   sdbLobHandle newLob               = 0 ;
   sdbLobHandle lob1                 = 0 ;
   sdbLobHandle lob2                 = 0 ;
   sdbLobHandle lob3                 = 0 ;
   sdbLobHandle lob4                 = 0 ;
   sdbLobHandle lob5                 = 0 ;

   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = sdbOpenLob( cl, NULL, SDB_LOB_CREATEONLY, &newLob );
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetLobId( newLob, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob( &newLob);
   ASSERT_EQ( SDB_OK, rc ) ;


   INT32 mode = SDB_LOB_SHAREREAD | SDB_LOB_WRITE;
   
   // case 1: test read and write mode

   // setp 1: open lob with SDB_LOB_SHAREREAD mode
   // setp 2: wirte data from lob head
   // setp 3: seek to the lob head 
   // setp 4: read lob and check data

   rc = sdbOpenLob( cl , &oid, mode, &lob1) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const UINT32 writelen1 = 5;
   char writeBuf1[writelen1] = { 0 };

   for(int i = 0; i < writelen1; i++)
   {
      writeBuf1[i] = 'a' + i;
   }
   
   rc = sdbWriteLob( lob1, writeBuf1, writelen1);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbSeekLob( lob1, 0, SDB_LOB_SEEK_SET);
   ASSERT_EQ( SDB_OK, rc ) ;

   SINT64 lobLen1;
   rc = sdbGetLobSize( lob1, &lobLen1);
   ASSERT_EQ( SDB_OK, rc ) ;
   char readBuf1[lobLen1] ;
   memset( readBuf1, 0, lobLen1 ) ;

   UINT32 readLen;
   rc = sdbReadLob( lob1, lobLen1, readBuf1, &readLen);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc =sdbCloseLob( &lob1 );
   ASSERT_EQ( SDB_OK, rc ) ;

   // check data
   for(int i = 0; i < lobLen1; i++)
   {
      ASSERT_EQ( writeBuf1[i], readBuf1[i] ) ;
   }


   // case 2: test read and write mode
   
   // setp 1: open lob with SDB_LOB_SHAREREAD mode
   // setp 2: write after read some data 
   // setp 3: seek to the lob head    
   // setp 4: read all data and check data

   rc = sdbOpenLob( cl , &oid, mode, &lob2) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const UINT32 lobLen2 = 3;
   char readBuf2[lobLen2] = { 0 };

   rc = sdbReadLob( lob2, lobLen2, readBuf2, &readLen);
   ASSERT_EQ( SDB_OK, rc ) ;
   
   const UINT32 writelen2 = 5;
   char writeBuf2[writelen2] = { 0 };
   for(int i = 0; i < writelen2; i++)
   {
      writeBuf2[i] = 'A' + i;
   }

   rc = sdbWriteLob( lob2, writeBuf2, writelen2);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbSeekLob( lob2, 0, SDB_LOB_SEEK_SET);
   ASSERT_EQ( SDB_OK, rc ) ;

   SINT64 lobSize;
   rc = sdbGetLobSize( lob2, &lobSize);
   ASSERT_EQ( SDB_OK, rc ) ;
   
   char data[lobSize] ;
   char expectData[lobSize] ;
   memset( data, 0, lobSize ) ;
   memset( expectData, 0, lobSize ) ;

   for(int i = 0; i < lobSize; i++)
   {
      if( i < lobLen2)
      { 
         expectData[i] = readBuf2[i];
      }
      else
      {
         expectData[i] = writeBuf2[i - lobLen2];
      }
   }

   rc = sdbReadLob( lob2, lobSize, data, &readLen);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob( &lob2 );
   ASSERT_EQ( SDB_OK, rc ) ;

   // check data
   for(int i = 0; i < lobSize; i++)
   {
      ASSERT_EQ( data[i], expectData[i] ) ;
   }


   // case 3: open lob with read mode, then wirte data to lob

   rc = sdbOpenLob( cl , &oid, SDB_LOB_READ , &lob3) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbWriteLob( lob3, writeBuf2, writelen2);
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   rc = sdbCloseLob( &lob3 );
   ASSERT_EQ( SDB_OK, rc ) ;

   
   // case 4: open lob with write mode, then read data from lob

   rc = sdbOpenLob( cl , &oid, SDB_LOB_WRITE , &lob4) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbReadLob( lob4, lobSize, data, &readLen);
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   rc = sdbCloseLob( &lob4 );
   ASSERT_EQ( SDB_OK, rc ) ;


   // case 5: open lob with write mode, then read data from lob

   rc = sdbOpenLob( cl , &oid, SDB_LOB_WRITE , &lob5) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbWriteLob( lob5, writeBuf2, writelen2);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetLobSize( lob5, &lobSize) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = sdbReadLob( lob5, lobSize, data, &readLen);
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   rc = sdbCloseLob( &lob5 );
   ASSERT_EQ( SDB_OK, rc ) ;
   

   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;

}

struct tArg
{
   sdbCollectionHandle cl ;
   bson_oid_t oid;
   INT64 offset ;
   INT32 tNum;
};

void * readAndWriteLob( void * arg )
{
   struct tArg *data = (struct tArg *) arg;
   
   INT32 rc = SDB_OK ;
   const INT32 bufLen = 2;
   sdbLobHandle lob = 0;
   char readBuf[bufLen] = { 0 };
   UINT32 readCount = 0;
   
   sdbCollectionHandle cl = data->cl;
   bson_oid_t oid = data->oid;
   INT64 offset = data->offset;
   char writeBuf[bufLen] = { 'a' + data->tNum, 'A' + data->tNum };
   
   INT32 mode = SDB_LOB_SHAREREAD | SDB_LOB_WRITE;
   
   sdbOpenLob( cl, &oid, mode, &lob );
   sdbLockAndSeekLob( lob, offset, bufLen);
   sdbWriteLob( lob, writeBuf, bufLen);

   sdbSeekLob( lob, offset, SDB_LOB_SEEK_SET);
   sdbReadLob( lob, bufLen, readBuf, &readCount);

   for(int i = 0; i < bufLen; i++)
   {
      if ( writeBuf[i] != readBuf[i] )
      {
         printf("Error :write data(%c) not equal read data(%c)\n",writeBuf[i], readBuf[i]);
         break;
      }
   }

   sdbCloseLob( &lob);
   pthread_exit(NULL);
}

TEST(lob,lob_ReadAndWirte_concurrent_test)
{

   // The test description:
   // setp 1: create some thread
   // setp 2: all threads open the same lob with read and write mode.
   // setp 3: thread lock and seek lob
   // setp 4: thread wirte lob
   // setp 5: thread read data from lob
   // setp 6: thread check read and write data
   // setp 7: main thread check lob data
   
   INT32 rc = SDB_OK ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   bson_oid_t oid ;
   sdbLobHandle newLob               = 0 ;

   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = sdbOpenLob( cl, NULL, SDB_LOB_CREATEONLY, &newLob );
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetLobId( newLob, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob( &newLob);
   ASSERT_EQ( SDB_OK, rc ) ;


   INT64 length = 2;   // each thread read/wirte length
   INT64 offset = 0;
   
   INT32 tCount = 10;   // thread count
   pthread_t thread[tCount];
   
   struct tArg arg[tCount];  // thread argments

   // create thread
   for(int i = 0; i < tCount; i++)
   {
       arg[i].cl = cl;
       arg[i].oid = oid;
       arg[i].offset = offset;
       arg[i].tNum = i;
       
       offset = offset + length;
   
       int rtn_thread ;
       rtn_thread = pthread_create(&thread[i], NULL, readAndWriteLob, (void *)&arg[i] );
       if ( 0 != rtn_thread )
       {
           continue;
       }
   }

   for(int i = 0; i < tCount; i++)
   {
       pthread_join(thread[i], NULL);
   }

   // main thread check lob data
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &newLob );
   ASSERT_EQ( SDB_OK, rc ) ;
   INT64 lobLen;

   sdbGetLobSize(newLob, &lobLen);

   UINT32 readCount = 0;
   char actData[lobLen] ;
   memset( actData, 0, lobLen ) ;
   INT32 expLen = tCount * 2;
   char expData[expLen] ;
   memset( expData, 0, expLen ) ;

   int n = 0;
   for( int i=0,j=0; i<expLen;)
   {
       expData[i] = 'a' + n;
       expData[i + 1] = 'A' + n;
       i = i + 2;
       n++;
   }
   
   rc = sdbReadLob( newLob, lobLen, actData, &readCount);
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbCloseLob( &newLob );
   ASSERT_EQ( SDB_OK, rc ) ;
      
   for(int i = 0; i < expLen; i++)
   {
       ASSERT_EQ( expData[i], actData[i] ) ;
   }


   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
}

TEST(lob,sdbGetRunTimeDetail_test)
{
   INT32 rc = SDB_OK ;
   // initialize the word environment
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize local variables
   sdbConnectionHandle db            = 0 ;
   sdbCollectionHandle cl            = 0 ;
   bson_oid_t oid ;
   sdbLobHandle lob               = 0 ;

   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = sdbOpenLob( cl, NULL, SDB_LOB_CREATEONLY, &lob );
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetLobId( lob, &oid );
   ASSERT_EQ( SDB_OK, rc ) ;

   bson detail;
   bson_init( &detail );
   bson_finish( &detail );
   rc = sdbGetRunTimeDetail( lob , &detail );
   ASSERT_EQ( SDB_OK, rc ) ;

   printf("lob run time detail info:\n");
   bson_print( &detail);

   bson_destroy( &detail ) ;

   rc = sdbCloseLob( &lob) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbDropCollectionSpace( db, COLLECTION_SPACE_NAME);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
}
