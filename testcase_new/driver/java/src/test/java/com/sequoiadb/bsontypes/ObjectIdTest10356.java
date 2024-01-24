package com.sequoiadb.bsontypes;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
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
 * FileName: ObjectIdTest10356.java* test interface: ObjectId (String s, boolean
 * babble) ObjectId (String s) TestLink: seqDB-10356:
 * 
 * @author wuyan
 * @Date 2016.10.17
 * @version 1.00
 */
public class ObjectIdTest10356 extends SdbTestBase {

    private String clName = "cl_10356";
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

    /**
     * test beyond the int of sequaoidb boundary value, verify insert and query
     * data
     */
    @Test
    public void testObjectId10356() {
        try {
            BSONObject obj = new BasicBSONObject();
            String str = "53bb5667c5d061d6f579d0bb";
            boolean babble_t = true;
            boolean babble_f = false;
            ObjectId id0 = new ObjectId( str );
            ObjectId id1 = new ObjectId( str, babble_t );
            ObjectId id2 = new ObjectId( str, babble_f );
            obj.put( "_id", id0 );
            obj.put( "_id1", id1 );
            obj.put( "_id2", id2 );
            cl.insert( obj );
            System.out.println( "obj=" + obj.toString() );
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
