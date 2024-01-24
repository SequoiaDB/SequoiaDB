package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Index7165.java
 * TestLink: seqDB-7172
 * 
 * @author zhaoyu
 * @Date 2016.9.21
 * @version 1.00
 */
public class Index7172 extends SdbTestBase {
    private Sequoiadb sdb;
    private String coordAddr;
    private String commCSName;

    @BeforeClass
    public void setUp() {
        this.coordAddr = SdbTestBase.coordUrl;
        this.commCSName = SdbTestBase.csName;
        try {
            sdb = new Sequoiadb( coordAddr, "", "" );
            if ( !sdb.isCollectionSpaceExist( commCSName ) ) {
                sdb.createCollectionSpace( commCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        CollectionSpace cs = sdb.getCollectionSpace( commCSName );
        DBCollection cl = null;
        // create cl
        String clName = "cl7172";
        BSONObject autoIndexId = ( BSONObject ) JSON
                .parse( "{AutoIndexId:false}" );
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            cl = cs.createCollection( clName, autoIndexId );
        } catch ( BaseException e ) {
            Assert.fail( "create cl:" + clName + " failed, errMsg:"
                    + e.getMessage() );
        }

        // insert data and check
        int dataNum = 100;
        ArrayList< BSONObject > insertData = new ArrayList< BSONObject >();

        try {
            for ( int i = 0; i < dataNum; i++ ) {
                BSONObject dataObj = new BasicBSONObject();
                dataObj.put( "a", i );
                dataObj.put( "_id", i );
                insertData.add( dataObj );
            }
            cl.bulkInsert( insertData, 0 );
        } catch ( BaseException e ) {
            Assert.fail( "insert data:" + insertData + " failed, errMsg:"
                    + e.getMessage() );
        }

        // can not update data when has no id index
        BSONObject modifier = ( BSONObject ) JSON.parse( "{$set:{a:2}}" );
        BSONObject match = ( BSONObject ) JSON.parse( "{a:1}" );
        try {
            cl.update( match, modifier, null );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -279 ) {
                Assert.assertTrue( false,
                        "update data, errMsg:" + e.getMessage() );
            }
        }

        // create id index
        try {
            BSONObject idIndexOption = ( BSONObject ) JSON
                    .parse( "{SortBufferSize:64}" );
            cl.createIdIndex( idIndexOption );
        } catch ( BaseException e ) {
            Assert.fail( "create index failed, errMsg:" + e.getMessage() );
        }

        DBCursor cursorIndex = cl.getIndex( "$id" );
        boolean isUnique = true;
        boolean enforced = true;
        BSONObject indexObj = ( BSONObject ) JSON.parse( "{_id:1}" );

        while ( cursorIndex.hasNext() ) {
            BSONObject record = ( BSONObject ) cursorIndex.getNext()
                    .get( "IndexDef" );

            boolean actualUnique = ( boolean ) record.get( "unique" );
            Assert.assertEquals( actualUnique, isUnique );

            boolean actualEnforced = ( boolean ) record.get( "enforced" );
            Assert.assertEquals( actualEnforced, enforced );

            Assert.assertEquals( record.get( "key" ), indexObj );

        }
        cursorIndex.close();
        // id index query
        ArrayList< BSONObject > expectData = new ArrayList< BSONObject >();
        for ( int i = 0; i < 50; i++ ) {
            expectData.add( insertData.get( i ) );
        }

        BSONObject matcher = ( BSONObject ) JSON.parse( "{_id:{$lt:50}}" );
        BSONObject orderBy = ( BSONObject ) JSON.parse( "{_id:1}" );
        DBCursor queryCursor = cl.query( matcher, null, orderBy, null );
        ArrayList< BSONObject > actualData = new ArrayList< BSONObject >();
        while ( queryCursor.hasNext() ) {
            actualData.add( queryCursor.getNext() );
        }
        queryCursor.close();
        Assert.assertEquals( actualData, expectData );

        // check explain
        BSONObject explainOption = ( BSONObject ) JSON.parse( "{Run:true}" );
        DBCursor explainCursor = cl.explain( matcher, null, orderBy, null, 0,
                -1, 0, explainOption );
        while ( explainCursor.hasNext() ) {
            BSONObject record = explainCursor.getNext();

            String scanType = ( String ) record.get( "ScanType" );
            Assert.assertEquals( scanType, "ixscan" );

            String actualIndexName = ( String ) record.get( "IndexName" );
            Assert.assertEquals( actualIndexName, "$id" );
        }
        explainCursor.close();

        // drop id index
        try {
            cl.dropIdIndex();
        } catch ( BaseException e ) {
            Assert.fail( "drop id index failed, errMsg:" + e.getMessage() );
        }

        DBCursor indexCursor = cl.getIndexes();
        int i = 0;
        while ( indexCursor.hasNext() ) {
            indexCursor.getNext();
            i++;
        }
        indexCursor.close();
        Assert.assertEquals( i, 0 );

        // clear env
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
    }
}
