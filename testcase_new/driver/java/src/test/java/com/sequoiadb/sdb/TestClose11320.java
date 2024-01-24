package com.sequoiadb.sdb;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestClose11320 test interface:close()
 * 
 * @author wuyan
 * @Date 2017.4.7
 * @version 1.00
 */
public class TestClose11320 extends SdbTestBase {
    private String clName = "cl_11320";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }
    }

    @Test
    public void testClose() {
        try {
            // check the resource is used
            Assert.assertEquals( sdb.isClosed(), false,
                    "the isClosed should be false" );
            Assert.assertEquals( sdb.isValid(), true,
                    "the isValid should be true" );
            sdb.disconnect();
            // if the resource has been released,
            // then the isClosed() is false and the isValid is true
            Assert.assertEquals( sdb.isClosed(), true,
                    "the isClosed should be true" );
            Assert.assertEquals( sdb.isValid(), false,
                    "the isValid should be false" );

            try {
                cs = sdb.getCollectionSpace( SdbTestBase.csName );
                cs.createCollection( clName );
                Assert.fail( "expect result need throw an error but not." );
            } catch ( BaseException e ) {
                // -80/-64,ignore exceptions
                if ( e.getErrorCode() != -64 && e.getErrorCode() != -80 ) {
                    Assert.fail( e.getMessage() );
                }

            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getErrorCode() + e.getMessage() );
        }
    }

    @AfterClass()
    public void tearDown() {

    }

}
