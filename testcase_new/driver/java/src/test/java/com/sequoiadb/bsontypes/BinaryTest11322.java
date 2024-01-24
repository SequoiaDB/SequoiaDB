package com.sequoiadb.bsontypes;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertTrue;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.Binary;
import org.bson.util.JSON;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Created by laojingtang on 17-4-7. 覆盖的测试用例：11322 测试点：Binary.equals()
 * Binary.hashCode ()
 */
public class BinaryTest11322 extends SdbTestBase {

    private String clName = "cl_11322";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            assertTrue( false, "connect %s failed," + SdbTestBase.coordUrl
                    + e.getMessage() );
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
            assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }

        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            cl = cs.createCollection( clName );
        } catch ( BaseException e ) {
            assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    @Test
    public void testBinary() {
        BSONObject obj = new BasicBSONObject();
        Binary binary = new Binary( "Hello".getBytes() );
        String b1 = "binary1";
        String b2 = "binary2";
        String b3 = "binary3";

        obj.put( b1, binary );
        obj.put( b2, binary );
        obj.put( b3, new Binary( "hello!".getBytes() ) );
        cl.insert( obj );

        BSONObject obj1 = cl.query( null,
                ( BSONObject ) JSON.parse( "{binary1:\"\"}" ), null, null )
                .getNext();
        BSONObject obj2 = cl.query( null,
                ( BSONObject ) JSON.parse( "{binary2:\"\"}" ), null, null )
                .getNext();
        BSONObject obj3 = cl.query( null,
                ( BSONObject ) JSON.parse( "{binary3:\"\"}" ), null, null )
                .getNext();

        Object o1 = obj1.get( b1 );
        Object o2 = obj2.get( b2 );
        Object o3 = obj3.get( b3 );

        assertTrue( o1.equals( o2 ) );
        assertTrue( o1.hashCode() == o2.hashCode() );
        assertFalse( o1.equals( o3 ) );
        assertFalse( o1.hashCode() == o3.hashCode() );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            sdb.close();
        } catch ( BaseException e ) {
            assertTrue( false, "clean up failed:" + e.getMessage() );
        }
    }
}
