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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: TestDeleteToBson7163.java test interface: delete (BSONObject
 * matcher,BSONObject hint)
 * 
 * @author wuyan
 * @Date 2016.9.22
 * @version 1.00
 */
public class TestDeleteToBson7163 extends SdbTestBase {
    private String clName = "cl_7163";
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
            long num = 2;
            for ( long i = 0; i < num; i++ ) {
                BSONObject obj = new BasicBSONObject();
                obj.put( "no", i );
                obj.put( "str", "test_" + String.valueOf( i ) );
                BSONObject arr = new BasicBSONList();
                arr.put( "0", 3 + i );
                arr.put( "1", "test" );
                arr.put( "2", 2.34 );
                arr.put( "3", "中文测*试" );
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
     * test inteface: delete (BSONObject matcher,BSONObject hint)
     */
    @Test
    public void deleteData() {
        try {
            cl.createIndex( "testIndex", "{no:1}", false, false );

            BSONObject hint = new BasicBSONObject();
            hint.put( "", "testIndex" );
            BSONObject matcher = new BasicBSONObject();
            matcher.put( "no", 1 );
            cl.delete( matcher, hint );

            // query delete record,not match to record :{no:1}
            DBCursor cursor = cl.query( matcher, null, null, hint );
            Assert.assertEquals( cursor.getNext(), null,
                    "query match data not null" );
            cursor.close();
            // query cl records ,now the records count is 1
            long count = cl.getCount();
            Assert.assertEquals( count, 1, "query match data not null" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "deleteBsonData fail " + e.getMessage() );
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
