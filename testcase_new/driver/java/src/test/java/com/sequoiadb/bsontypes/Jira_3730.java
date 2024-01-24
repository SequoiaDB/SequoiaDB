package com.sequoiadb.bsontypes;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class Jira_3730 extends SdbTestBase {

    private String clName = "cl_jira_3730";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
            sdb.createCollectionSpace( SdbTestBase.csName );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
    }

    @Test
    public void jira_3348_BsonTimestampTest() {
        int maxIntSec = 2147483647;
        int minIntSec = -2147483648;
        int secmin = 0;
        int secmax = 999999;
        BSONObject insertor1 = new BasicBSONObject( "a",
                new BSONTimestamp( maxIntSec, secmax ) ).append( "b", 1 );
        BSONObject insertor2 = new BasicBSONObject( "a",
                new BSONTimestamp( maxIntSec, secmin ) ).append( "b", 2 );
        BSONObject insertor3 = new BasicBSONObject( "a",
                new BSONTimestamp( minIntSec, secmax ) ).append( "b", 3 );
        BSONObject insertor4 = new BasicBSONObject( "a",
                new BSONTimestamp( minIntSec, secmin ) ).append( "b", 4 );
        BSONObject insertor5 = new BasicBSONObject( "a",
                new BSONTimestamp( minIntSec, -1 ) ).append( "b", 5 );
        BSONObject insertor6 = new BasicBSONObject( "a",
                new BSONTimestamp( minIntSec, 1000000 ) ).append( "b", 6 );
        BSONObject insertor7 = new BasicBSONObject( "a",
                new BSONTimestamp( maxIntSec, -1 ) ).append( "b", 7 );
        BSONObject insertor8 = new BasicBSONObject( "a",
                new BSONTimestamp( maxIntSec, 1000000 ) ).append( "b", 8 );
        cl.insert( insertor1 );
        cl.insert( insertor2 );
        cl.insert( insertor3 );
        cl.insert( insertor4 );
        cl.insert( insertor5 );
        cl.insert( insertor6 );
        cl.insert( insertor7 );
        cl.insert( insertor8 );
        DBQuery matcher = new DBQuery();
        BSONObject order = new BasicBSONObject();
        order.put( "b", 1 );
        matcher.setOrderBy( order );
        DBCursor cursor = cl.query( matcher );
        BSONObject record;
        List< BSONTimestamp > bsonList = new ArrayList< BSONTimestamp >();
        while ( ( record = cursor.getNext() ) != null ) {
            BSONTimestamp t = ( BSONTimestamp ) record.get( "a" );
            bsonList.add( t );
        }
        BSONTimestamp t = ( BSONTimestamp ) bsonList.get( 0 );
        Assert.assertEquals( t.getTime(), maxIntSec );
        Assert.assertEquals( t.getInc(), secmax );

        t = ( BSONTimestamp ) bsonList.get( 1 );
        Assert.assertEquals( t.getTime(), maxIntSec );
        Assert.assertEquals( t.getInc(), secmin );

        t = ( BSONTimestamp ) bsonList.get( 2 );
        Assert.assertEquals( t.getTime(), minIntSec );
        Assert.assertEquals( t.getInc(), secmax );

        t = ( BSONTimestamp ) bsonList.get( 3 );
        Assert.assertEquals( t.getTime(), minIntSec );
        Assert.assertEquals( t.getInc(), secmin );

        t = ( BSONTimestamp ) bsonList.get( 4 );
        Assert.assertEquals( t.getTime(), minIntSec - 1 );
        Assert.assertEquals( t.getInc(), 999999 );

        t = ( BSONTimestamp ) bsonList.get( 5 );
        Assert.assertEquals( t.getTime(), minIntSec + 1 );
        Assert.assertEquals( t.getInc(), 0 );

        t = ( BSONTimestamp ) bsonList.get( 6 );
        Assert.assertEquals( t.getTime(), maxIntSec - 1 );
        Assert.assertEquals( t.getInc(), 999999 );

        t = ( BSONTimestamp ) bsonList.get( 7 );
        Assert.assertEquals( t.getTime(), maxIntSec + 1 );
        Assert.assertEquals( t.getInc(), 0 );

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

}
