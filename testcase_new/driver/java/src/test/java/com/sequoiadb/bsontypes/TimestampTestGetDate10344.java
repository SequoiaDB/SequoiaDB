package com.sequoiadb.bsontypes;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
import org.bson.util.DateInterceptUtil;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TimestampTestGetDate10344.java* test interface: BSONTimestamp (int
 * time, int inc),getDate (),toString() TestLink: seqDB-10344:
 * 
 * @author wuyan
 * @Date 2016.10.13
 * @version 1.00
 */
public class TimestampTestGetDate10344 extends SdbTestBase {
    private String clName = "cl_10344";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
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
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName );
    }

    @Test
    public void testGetDateAndToString() {
        try {
            BSONObject obj = new BasicBSONObject();
            int seconds = 23456;
            int inc = 99988;
            String expectTime = "{ $timestamp : 1970-01-01-14.30.56.99988 }";
            BSONTimestamp timestamp = new BSONTimestamp( seconds, inc );

            // test the toString()
            String timeString = timestamp.toString();
            Assert.assertEquals( timeString, expectTime,
                    "check not string data" );
            // test the getDate()
            Date date = timestamp.getDate();
            Date expdate = DateInterceptUtil.interceptDate( date,
                    "yyyy-MM-dd" );
            obj.put( "date", expdate );
            obj.put( "timestamp", timestamp );
            cl.insert( obj );
            // check the insert result
            BSONObject tmp = new BasicBSONObject();
            DBCursor tmpCursor = cl.query( tmp, null, null, null );
            BasicBSONObject actRecs = null;
            while ( tmpCursor.hasNext() ) {
                actRecs = ( BasicBSONObject ) tmpCursor.getNext();
            }
            tmpCursor.close();
            Assert.assertEquals( actRecs, obj,
                    "check datas are unequal\n" + "actDatas: " + actRecs + "\n"
                            + "expectDatas: " + timestamp.toString() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() + e.getStackTrace() );
        }
    }

    @AfterClass
    public void tearDown() {
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        sdb.disconnect();
    }
}
