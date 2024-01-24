package com.sequoiadb.basicoperation;

import java.util.ArrayList;
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
import com.sequoiadb.base.DBQuery;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestInsertToJson7151.java test interface: insert (String insertor),
 * update (DBQuery query), getCount (String matcher)
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestInsertToJson7151 extends SdbTestBase {
    private String clName = "cl_7151";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;
    // insert records
    String[] records = {
            "{'no':-2147483648,'tlong':9223372036854775807,'tf':1.7e+308,'td':{'$decimal':'123.45'},boolt:true}",
            "{no:0,numlong:{'$numberLong':'-9223372036854775808'},oid:{ '$oid':'123abcd00ef12358902300ef'},tfm:-1.7E+308}",
            "{no:1,numlongm:{'$numberLong':'9223372036854775807'},'ts':'test',reg:{'$regex':'^张','$options':'i'},tc:'可能会被调'}",
            "{no:2147483647,date:{'$date':'2012-01-01'},time:{'$timestamp':'2012-01-01-13.14.26.124233'},arr:['abc',345,true]}" };

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }

        createCL();
    }

    private void createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    public void insertDatas() {
        Object oid = null;
        for ( int i = 0; i < records.length; i++ ) {
            try {
                // return oid;
                oid = cl.insert( records[ i ] );
            } catch ( BaseException e ) {
                Assert.assertTrue( false,
                        "insert jsonDatas fail " + e.getMessage() );
            }
        }

        // test the insert return value :oid
        BSONObject obj = new BasicBSONObject();
        obj.put( "_id", oid );
        Assert.assertEquals( 1, cl.getCount( obj ),
                "the insert return oid is wrong" );

        // test the records count
        long count = cl.getCount();
        Assert.assertEquals( records.length, count );
        checkClData();
    }

    // test update (DBQuery query)
    public void updateDatas() {
        try {
            DBQuery query = new DBQuery();
            BSONObject modifier = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            BSONObject mValue = new BasicBSONObject();
            matcher.put( "no", 1 );
            mValue.put( "ts", "uptest85" );
            modifier.put( "$set", mValue );
            query.setModifier( modifier );
            query.setMatcher( matcher );
            cl.update( query );
            long count = cl.getCount( mValue );
            Assert.assertEquals( 1, count );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "update(DBQuery query) fail " + e.getMessage() );
        }
    }

    public void checkClData() {
        DBCursor cursor = null;
        BSONObject actRecs = null;
        List< BSONObject > list = new ArrayList< BSONObject >();
        List< BSONObject > listExpdatas = new ArrayList< BSONObject >();
        try {
            cursor = cl.query( "", "{_id:{$include:0}}", "{no:1}", "" );
            while ( cursor.hasNext() ) {
                actRecs = cursor.getNext();
                list.add( actRecs );
            }

            for ( int i = 0; i < records.length; i++ ) {
                BSONObject expRec = ( BSONObject ) JSON.parse( records[ i ] );
                listExpdatas.add( expRec );
            }
            Assert.assertEquals( listExpdatas, list,
                    "actDatas: " + list + "\n" + "expRecs: " + listExpdatas );
            cursor.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "check result is wrong, errMsg:%s\n" + e.getMessage() );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
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

    @Test
    public void testInsertToJson() {
        try {
            insertDatas();
            updateDatas();
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.assertTrue( false, e.getMessage() );
        }
    }
}
