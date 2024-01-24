package com.sequoiadb.basicoperation;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
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
 * FileName: TestInsertSameOid7154.java concurrent insert datas of the same
 * OID,only 1 records inserted successfully
 * 
 * @author wuyan
 * @Date 2016.9.12
 * @version 1.00
 */
public class TestInsertSameOid7154 extends SdbTestBase {
    private String clName = "cl_7154";
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

    @Test(invocationCount = 2, threadPoolSize = 1)
    public void testInsert() {
        BSONObject obj = new BasicBSONObject();
        try {
            obj = new BasicBSONObject();
            ObjectId id = new ObjectId( "53bb5667c5d061d6f579d0bb" );
            obj.put( "_id", id );
            obj.put( "no", 2 );
            cl.insert( obj );
        } catch ( BaseException e ) {
            Assert.assertEquals( -38, e.getErrorCode(), e.getMessage() );
        }
    }

    /**
     * test insert result,only insert 1 records
     */
    @Test
    public void testResult() {
        long count = cl.getCount();
        Assert.assertEquals( count, 1, "the actDatas is :" + count );
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
