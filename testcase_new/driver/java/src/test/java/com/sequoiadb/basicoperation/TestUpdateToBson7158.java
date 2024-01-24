package com.sequoiadb.basicoperation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * FileName: TestUpdateToBson7158.java test interface: update (BSONObject
 * matcher, BSONObject modifier, BSONObject hint)
 * 
 * @author wuyan
 * @Date 2016.9.19
 * @version 1.00
 */
public class TestUpdateToBson7158 extends SdbTestBase {

    private String clName = "cl_7158";
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
        String[] records = { "{'no':0,arr:['abc',345,true]}",
                "{no:1,numlong:{'$numberLong':'-9223372036854775808'}}",
                "{no:2,'ts':'test'}" };
        insertDatas( records );
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

    public void insertDatas( String[] records ) {
        // insert records
        for ( int i = 0; i < records.length; i++ ) {
            try {
                cl.insert( records[ i ] );
            } catch ( BaseException e ) {
                Assert.assertTrue( false,
                        "insert jsonDatas fail " + e.getMessage() );
            }
        }
        long count = cl.getCount();
        Assert.assertEquals( records.length, count,
                "insert datas count error" );
    }

    /**
     * test inteface: update (String matcher, String modifier, String hint)
     * update (null, String modifier, null)
     */
    @Test
    public void updateData() {
        try {
            BSONObject modifier = new BasicBSONObject();
            BSONObject mValue = new BasicBSONObject();
            BSONObject modifier1 = new BasicBSONObject();
            BSONObject mValue1 = new BasicBSONObject();
            BSONObject matcher = new BasicBSONObject();
            BSONObject hint = new BasicBSONObject();
            mValue.put( "ts", "testbson" );
            modifier.put( "$set", mValue );
            matcher.put( "no", 2 );
            hint.put( "", "testIndex" );
            mValue1.put( "ts", "testnull" );
            modifier1.put( "$set", mValue1 );

            cl.createIndex( "testIndex", "{no:1}", false, false );
            cl.update( matcher, modifier, hint );
            long count = cl.getCount( mValue );
            Assert.assertEquals( count, 1, "updateBson data error" );

            // verfy for the null fields
            cl.update( null, modifier1, null );
            long count1 = cl.getCount( mValue1 );
            Assert.assertEquals( count1, 3, "only modifier updateBson error" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "update fail " + e.getMessage() );
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
