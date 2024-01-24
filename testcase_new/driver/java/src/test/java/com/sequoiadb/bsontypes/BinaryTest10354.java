package com.sequoiadb.bsontypes;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.Binary;
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
 * FileName: BinaryTest10354.java* test interface: Binary (byte type, byte[]
 * data),getDate (),length (),getType () TestLink: seqDB-10354:
 * 
 * @author wuyan
 * @Date 2016.10.14
 * @version 1.00
 */
public class BinaryTest10354 extends SdbTestBase {

    private String clName = "cl_10354";
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

    @Test
    public void testBinary() {
        try {
            BSONObject obj = new BasicBSONObject();
            String str = "hello world!";
            byte type = 0;
            byte[] arr = str.getBytes();
            Binary bindata = new Binary( type, arr );
            System.out.println( "type=" + bindata.getType() );
            // test interface length()
            Assert.assertEquals( bindata.length(), str.length(),
                    "bindata length wrong" );

            // test the bindata type ,is 0
            Assert.assertEquals( bindata.getType(), 0, "bindata type wrong" );

            obj.put( "bindata", bindata );
            cl.insert( obj );

            // check the insert result
            BSONObject tmp = new BasicBSONObject();
            DBCursor tmpCursor = cl.query( tmp, null, null, null );
            BasicBSONObject actRecs = null;
            while ( tmpCursor.hasNext() ) {
                actRecs = ( BasicBSONObject ) tmpCursor.getNext();
            }
            System.out.println( "actRecs=" + actRecs.toString() );
            tmpCursor.close();
            Assert.assertEquals( actRecs, obj,
                    "check datas are unequal\n" + "actDatas: " + actRecs + "\n"
                            + "expectDatas: " + obj.toString() );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, e.getMessage() + e.getStackTrace() );
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
