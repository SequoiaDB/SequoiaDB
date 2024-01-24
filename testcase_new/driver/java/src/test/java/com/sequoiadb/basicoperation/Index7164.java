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
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Index7164.java
 * TestLink: seqDB-7164
 * 
 * @author zhaoyu
 * @Date 2016.9.21
 * @version 1.00
 */
public class Index7164 extends SdbTestBase {
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
        String clName = "cl7164";
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( "create cl:" + clName + " failed, errMsg:"
                    + e.getMessage() );
        }

        // insert data
        int dataNum = 100;
        ArrayList< BSONObject > insertData = new ArrayList< BSONObject >();

        try {
            for ( int i = 0; i < dataNum; i++ ) {
                BSONObject dataObj = new BasicBSONObject();
                dataObj.put( "a", i );
                insertData.add( dataObj );
            }
            cl.bulkInsert( insertData, 0 );
        } catch ( BaseException e ) {
            Assert.fail( "insert data:" + insertData + " failed, errMsg:"
                    + e.getMessage() );
        }

        // create index
        String indexName = "aIndex";
        boolean isUnique = false;
        boolean enforced = false;
        int sortBufferSize = 64;
        BSONObject indexObj = ( BSONObject ) JSON.parse( "{a:1}" );
        try {
            cl.createIndex( indexName, indexObj, isUnique, enforced,
                    sortBufferSize );
        } catch ( BaseException e ) {
            Assert.fail( "create index failed, errMsg:" + e.getMessage() );
        }
        Assert.assertNotNull( cl.getIndex( indexName ),
                "expect index is not null" );
        ;

        // index scan and check result
        ArrayList< BSONObject > expectData = new ArrayList< BSONObject >();
        for ( int i = 0; i < 50; i++ ) {
            expectData.add( insertData.get( i ) );
        }

        BSONObject matcher = ( BSONObject ) JSON.parse( "{a:{$lt:50}}" );
        BSONObject orderBy = ( BSONObject ) JSON.parse( "{a:1}" );
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
            Assert.assertEquals( actualIndexName, indexName );
        }
        explainCursor.close();

        // check index option
        DBCursor cursorIndex = cl.getIndex( indexName );
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

        // drop index
        try {
            cl.dropIndex( indexName );
        } catch ( BaseException e ) {
            Assert.fail( "drop index failed, errMsg:" + e.getMessage() );
        }

        // check index drop successfully
        DBCursor cursorDropIndex = cl.getIndex( indexName );
        while ( cursorDropIndex.hasNext() ) {
            Assert.assertNull( cursorDropIndex.getNext() );
        }
        cursorDropIndex.close();

        // createIdIndex(BSONObject options) options is null
        cl.dropIdIndex();
        DBCursor idIndex = cl.getIndex( "$id" );
        while ( idIndex.hasNext() ) {
            Assert.assertNull( idIndex.getNext() );
        }
        idIndex.close();
        cl.createIdIndex( null );
        idIndex = cl.getIndex( "$id" );
        while ( idIndex.hasNext() ) {
            Assert.assertNotNull( idIndex.getNext() );
        }
        idIndex.close();

        // clear env
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
    }
}
