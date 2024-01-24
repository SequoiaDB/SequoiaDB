package com.sequoiadb.bsontypes;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: MaxKeyTestEquals10348.java* test interface: equals (Object obj)
 * TestLink: seqDB-10348:
 * 
 * @author wuyan
 * @Date 2016.10.14
 * @version 1.00
 */
public class MaxKeyTestEquals10348 extends SdbTestBase {
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
    public void testGetDateAndToString() {
        try {
            BSONObject obj = new BasicBSONObject();
            MaxKey maxkey = new MaxKey();
            MaxKey maxkey1 = new MaxKey();
            obj.put( "maxkey", maxkey );

            Assert.assertEquals( maxkey.equals( maxkey ), true,
                    "check self fail" );
            Assert.assertEquals( maxkey.equals( maxkey1 ), true,
                    "check the same data fail" );
            Assert.assertEquals( maxkey.equals( obj.get( "maxkey" ) ), true,
                    "check the get object" );
            Assert.assertEquals( maxkey.equals( null ), false,
                    "check null fail" );
            String otherTypeValue = "MaxKey()";
            Assert.assertEquals( maxkey.equals( otherTypeValue ), false,
                    "check other type fail" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() + e.getStackTrace() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.disconnect();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }

}
