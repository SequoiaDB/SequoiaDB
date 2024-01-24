package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
 * FileName: TestUpsertSetOnInsert7160.java test interface: upsert (BSONObject
 * matcher, BSONObject modifier, BSONObject hint, BSONObject setOnInsert)
 * 
 * @author wuyan
 * @Date 2016.9.19
 * @version 1.00
 */
public class TestUpsertSetOnInsert7160 extends SdbTestBase {

    private String clName = "cl_7160";
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
                obj.put( "no", i );
                obj.put( "str", "test_" + String.valueOf( i ) );
                BSONObject arr = new BasicBSONList();
                arr.put( "0", 3 );
                arr.put( "1", "test" );
                arr.put( "2", 2.34 );
                obj.put( "arr", arr );
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
     * BSONObject hint, BSONObject setOnInsert) upsert (null, BSONObject
     * modifier, null,null)
     */
    @Test
    public void upsertData() {
        try {
            BSONObject modifier = new BasicBSONObject();
            BSONObject moValue = new BasicBSONObject();
            BSONObject modifier1 = new BasicBSONObject();
            BSONObject mValue1 = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            BSONObject maValue = new BasicBSONObject();
            BSONObject hint = new BasicBSONObject();
            BSONObject setOnInsert = new BasicBSONObject();
            moValue.put( "name", "make" );
            modifier.put( "$set", moValue );
            maValue.put( "$gt", 1 );
            matcher.put( "no", maValue );
            hint.put( "", "testIndex" );
            setOnInsert.put( "age", 88 );
            mValue1.put( "test", 999 );
            modifier1.put( "$set", mValue1 );

            cl.createIndex( "testIndex", "{str:1}", false, false );
            cl.upsert( matcher, modifier, hint, setOnInsert );
            // check the upsert result,match not to record then insert
            // :{arr:[4],no:4}
            long count = cl.getCount( moValue, setOnInsert );
            Assert.assertEquals( count, 1, "upsertBson data error" );

            // noly modifier,all results insert :{"test":999}
            cl.upsert( null, modifier1, null, null );
            // all records add :{"test":999}
            long count1 = cl.getCount( mValue1 );
            Assert.assertEquals( count1, 3, "only modifier upsertBson error" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "upsertSetOnInsert fail " + e.getMessage() );
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
