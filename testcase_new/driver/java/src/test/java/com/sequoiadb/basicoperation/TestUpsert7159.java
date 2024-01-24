package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestUpsert7159.java test interface: upsert (BSONObject matcher,
 * BSONObject modifier, BSONObject hint)
 * 
 * @author wuyan
 * @Date 2016.9.19
 * @version 1.00
 */
public class TestUpsert7159 extends SdbTestBase {

    private String clName = "cl_7159";
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

        createCL();
        insertDatas();
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
        try {
            List< BSONObject > list = new ArrayList< BSONObject >();
            long num = 2;
            for ( long i = 0; i < num; i++ ) {
                BSONObject obj = new BasicBSONObject();
                ObjectId id = new ObjectId();
                obj.put( "_id", id );
                String str = "32345.067891234567890123456789" + i;
                BSONDecimal decimal = new BSONDecimal( str );
                obj.put( "decimal", decimal );
                obj.put( "no", i );
                obj.put( "str", "test_" + String.valueOf( i ) );
                list.add( obj );
            }
            cl.bulkInsert( list, 0 );
            long count = cl.getCount();
            Assert.assertEquals( num, count, "insert datas count error" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "bulkinsert fail " + e.getMessage() );
        }
    }

    /**
     * test inteface: upsert (BSONObject matcher, BSONObject modifier,
     * BSONObject hint) upsert (null, BSONObject modifier, null,null)
     */
    @Test
    public void upsertData() {
        try {
            BSONObject modifier = new BasicBSONObject();
            BSONObject mValue = new BasicBSONObject();
            BSONObject modifier1 = new BasicBSONObject();
            BSONObject mValue1 = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            BSONObject hint = new BasicBSONObject();
            BSONObject arr = new BasicBSONList();
            arr.put( "0", 4 );
            mValue.put( "arr", arr );
            modifier.put( "$addtoset", mValue );
            matcher.put( "no", 4 );
            hint.put( "", "testIndex" );
            mValue1.put( "test", 1 );
            modifier1.put( "$set", mValue1 );

            cl.createIndex( "testIndex", "{no:1}", false, false );
            cl.upsert( matcher, modifier, hint );
            // check the upsert result,match not to record then insert
            // :{arr:[4],no:4}
            long count = cl.getCount( mValue, matcher );
            Assert.assertEquals( count, 1, "upsertBson data error" );
            // noly modifier,all results insert :{"test":1}
            cl.upsert( null, modifier1, null );
            long count1 = cl.getCount( mValue1 );
            Assert.assertEquals( count1, 3, "only modifier upsertBson error" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "upsert fail " + e.getMessage() );
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

}
