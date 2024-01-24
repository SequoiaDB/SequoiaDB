package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

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

import com.sequoiadb.testcommon.*;

public class Aggregate7103 extends SdbTestBase {
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
        String clName = "cl7103";
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
        int dataNum = 10;
        ArrayList< BSONObject > insertData = new ArrayList< BSONObject >();
        try {
            for ( int i = 0; i < dataNum; i++ ) {
                BSONObject dataObj = new BasicBSONObject();
                dataObj.put( "a", i );
                dataObj.put( "b", i );
                dataObj.put( "c", i );
                insertData.add( dataObj );
            }
            cl.bulkInsert( insertData, 0 );
        } catch ( BaseException e ) {
            Assert.fail( "insert data:" + insertData + " failed, errMsg:"
                    + e.getMessage() );
        }

        // aggregate
        List< BSONObject > aggregateObj = new ArrayList< BSONObject >();
        BSONObject limitObj = ( BSONObject ) JSON.parse( "{$limit:5}" );
        BSONObject projectObj = ( BSONObject ) JSON
                .parse( "{$project:{a: 0,b: 1}}" );
        aggregateObj.add( limitObj );
        aggregateObj.add( projectObj );

        List< BSONObject > actualData = new ArrayList< BSONObject >();
        DBCursor dbCursor = cl.aggregate( ( List< BSONObject > ) aggregateObj );
        while ( dbCursor.hasNext() ) {
            actualData.add( dbCursor.getNext() );
        }

        ArrayList< BSONObject > expectData = new ArrayList< BSONObject >();
        for ( int i = 0; i < 5; i++ ) {
            BSONObject obj = new BasicBSONObject();
            int value = ( int ) insertData.get( i ).get( "b" );
            obj.put( "b", value );
            expectData.add( obj );
        }

        Assert.assertEquals( actualData, expectData );

        // clear env
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
    }
}
