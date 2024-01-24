package com.sequoiadb.sdb;

import java.util.Date;

import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestSdb11317 test interface:Sequoiadb (String connString, String
 * username, String password), test only set host,result false
 * 
 * @author wuyan
 * @Date 2017.4.7
 * @version 1.00
 */
public class TestSdb11317 extends SdbTestBase {
    private static Sequoiadb sdb = null;

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
    public void testSdbNoSetPort() {
        try {
            String hostname = sdb.getHost();
            sdb = new Sequoiadb( hostname, "", "" );
            Assert.fail( "expect result need throw an error but not." );
        } catch ( BaseException e ) {
            Assert.assertEquals( -6, e.getErrorCode(), e.getMessage() );
            ;
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            sdb.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

}
