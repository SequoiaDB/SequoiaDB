package com.sequoiadb.basicoperation;

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
 * FileName: TestUpdateToJson7157.java test interface: update (String matcher,
 * String modifier, String hint)
 * 
 * @author wuyan
 * @Date 2016.9.19
 * @version 1.00
 */
public class TestUpdateToJson7157 extends SdbTestBase {

    private String clName = "cl_7157";
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
    public void updateDatas() {
        try {
            String matcher = "{no:{$gt:0}}";
            String modifier = "{$set:{ts:'test01'}}";
            String modifier1 = "{$set:{ts:'test02'}}";
            String hint = "{'':'testIndex'}";
            cl.createIndex( "testIndex", "{no:1}", false, false );
            cl.update( matcher, modifier, hint );
            // check update result
            long count = cl.getCount( "{'ts':'test01'}" );
            Assert.assertEquals( count, 2, "update data error" );

            // verify for the null fields
            cl.update( null, modifier1, null );
            long count1 = cl.getCount( "{'ts':'test02'}" );
            Assert.assertEquals( count1, 3, "update data error" );
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
