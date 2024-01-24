package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
 * FileName: TestTruncate7161.java test interface: delete (String matcher)
 * truncate ()
 * 
 * @author wuyan
 * @Date 2016.9.22
 * @version 1.00
 */
public class TestTruncate7161 extends SdbTestBase {

    private String clName = "cl_7161";
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
        insertDatas();
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
        try {
            List< BSONObject > list = new ArrayList< BSONObject >();
            long num = 3;
            for ( long i = 0; i < num; i++ ) {
                BSONObject obj = new BasicBSONObject();
                obj.put( "no", i );
                obj.put( "str", "test_" + String.valueOf( i ) );
                BSONObject arr = new BasicBSONList();
                arr.put( "0", 3 + i );
                arr.put( "1", "test" );
                arr.put( "2", 2.34 );
                obj.put( "date", new Date() );
                obj.put( "arr", arr );
                list.add( obj );
            }
            cl.bulkInsert( list, 0 );
            long count = cl.getCount();
            Assert.assertEquals( num, count, "insert datas count error" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "bulkinsert fail " + e.getMessage() );
        }
    }

    /**
     * test inteface: delete (String matcher) truncate ()
     */
    @Test
    public void deleteData() {
        try {
            String matcher = "{arr:[4,'test',2.34]}";
            cl.delete( matcher );
            // check record delete,match not to record :{no:1}
            long count = cl.getCount( matcher );
            Assert.assertEquals( count, 0, "delete JsonData error" );

            // truncate all records
            cl.truncate();
            long count1 = cl.getCount();
            Assert.assertEquals( count1, 0, "truncate error" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "truncate fail " + e.getMessage() );
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
