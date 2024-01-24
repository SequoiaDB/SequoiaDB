package com.sequoiadb.sdb;

import java.util.Date;

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
 * FileName: TestGetLastUseTime11316 test interface:getLastUseTime()*
 * 
 * @author wuyan
 * @Date 2017.4.7
 * @version 1.00
 */
public class TestGetLastUseTime11316 extends SdbTestBase {
    private String clName = "cl_11316";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            sdb.setSessionAttr(
                    new BasicBSONObject( "PreferedInstance", "M" ) );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect %s failed,"
                    + SdbTestBase.coordUrl + e.getMessage() );
        }
    }

    @Test
    public void testLastUseTime() {
        try {
            long beforeTime = sdb.getLastUseTime();
            createCL();
            insertDatas();
            long afterTime = sdb.getLastUseTime();

            if ( beforeTime >= afterTime ) {
                Assert.assertTrue( false,
                        "beforeTime greater than afterTime!" + "  beforeTime:"
                                + beforeTime + "   afterTime:" + afterTime );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getErrorCode() + e.getMessage() );
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "clean up failed:" + e.getMessage() );
        }
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
        // insert records
        String[] records = {
                "{'no':-2147483648,'tlong':9223372036854775807,'tf':1.7e+308,'td':{'$decimal':'123.45'},boolt:true}",
                "{no:0,numlong:{'$numberLong':'-9223372036854775808'},oid:{ '$oid':'123abcd00ef12358902300ef'},tfm:-1.7E+308}",
                "{no:1,numlongm:{'$numberLong':'9223372036854775807'},'ts':'test',reg:{'$regex':'^张','$options':'i'},tc:'可能会被调'}",
                "{no:2147483647,date:{'$date':'2012-01-01'},time:{'$timestamp':'2012-01-01-13.14.26.124233'},arr:['abc',345,true]}" };

        for ( int i = 0; i < records.length; i++ ) {
            try {
                cl.insert( records[ i ] );
            } catch ( BaseException e ) {
                Assert.assertTrue( false,
                        "insert jsonDatas fail " + e.getMessage() );
            }
        }
    }

}
