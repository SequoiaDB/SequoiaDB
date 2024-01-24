package com.sequoiadb.basicoperation;

import static org.testng.Assert.assertEquals;
import static org.testng.Assert.assertTrue;

import java.util.Arrays;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.util.Helper;

/**
 * Created by laojingtang on 17-4-10. 覆盖的测试用例11318 测试点：交替调用Cursor.getNext()
 * Cursor.getNextRaw()
 *
 */
public class CursorTest11318 extends SdbTestBase {
    private String clName = "cl_11318";
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
    public void test() {
        BSONObject b1 = new BasicBSONObject( "a", 1 );
        BSONObject b2 = new BasicBSONObject( "a", 2 );
        BSONObject b3 = new BasicBSONObject( "a", 3 );
        BSONObject b4 = new BasicBSONObject( "a", 4 );
        cl.insert( b1 );
        cl.insert( b2 );
        cl.insert( b3 );
        cl.insert( b4 );

        DBCursor cursor = cl.query();
        assertTrue( cursor.getCurrent().equals( b1 ) );
        assertTrue( cursor.getNext().equals( b2 ) );
        byte[] temp = Helper.encodeBSONObj( b3 );
        byte[] temp2 = cursor.getNextRaw();
        assertTrue( Arrays.equals( temp, temp2 ) );
        assertTrue( cursor.getNext().equals( b4 ) );
        cursor.close();
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
