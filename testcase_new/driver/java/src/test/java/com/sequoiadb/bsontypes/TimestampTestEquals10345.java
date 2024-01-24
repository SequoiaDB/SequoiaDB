package com.sequoiadb.bsontypes;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TimestampTestEquals10345.java* test interface: equals (Object obj)
 * TestLink: seqDB-10345:
 * 
 * @author wuyan
 * @Date 2016.10.14
 * @version 1.00
 */
public class TimestampTestEquals10345 extends SdbTestBase {
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
            int seconds = 23456;
            int inc = 99988;
            String expectTime = "{ $timestamp : 1970-01-01-14.30.56.99988}";
            BSONTimestamp timestamp = new BSONTimestamp( seconds, inc );
            BSONTimestamp timestamp1 = new BSONTimestamp( seconds, inc );
            obj.put( "time", expectTime );

            Assert.assertEquals( timestamp.equals( timestamp ), true,
                    "check timestamp self fail" );
            Assert.assertEquals( timestamp.equals( timestamp1 ), true,
                    "check timestamp self fail" );
            Assert.assertEquals( timestamp.equals( obj.get( "time" ) ), false,
                    "check the get object" );
            Assert.assertEquals( timestamp.equals( null ), false,
                    "check null fail" );
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
