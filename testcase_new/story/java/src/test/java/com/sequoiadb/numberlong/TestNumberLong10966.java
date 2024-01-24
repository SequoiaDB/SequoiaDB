package com.sequoiadb.numberlong;

import java.util.Date;

import org.bson.BSON;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * 1.在各驱动的BSON中都增加一个全局控制的配置项来控制BSON的toString()方法的显示格式
 * 2、该配置项默认处于关闭状态。在这种情况，不管long类型的数字范围如何，所有驱动BSON的toString方法都按如下格式输出：
 * {a:9223372036854775807} // 此处9223372036854775807已经大于2^53 - 1
 * 3、该配置项处于开启状态时，驱动BSON的toString方法根据long类型的数字范围显示如下：
 * 1）当long类型的数字在[-2^53-1，2^53-1]范围时，如下显示： {a:9223372036854775807}
 * 2）当long类型的数字小于-2^53-1或者大于2^53-1时，如下显示： {"a" : { "$numberLong" :
 * "9223372036854775807"}}
 * 
 * @author chensiqin
 *
 */
public class TestNumberLong10966 extends SdbTestBase {

    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10966";

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
            createCL();

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public void createCL() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
            this.cl = this.cs.createCollection( this.clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @Test
    public void test() {
        // 默认关闭状态
        BSON.setJSCompatibility( false );
        int id = 1;
        testInt( id );
        // 设置为true
        id = 2;
        BSON.setJSCompatibility( true );
        testInt( id );
        // 使用默认值
        BSON.setJSCompatibility( false );
        id = 3;
        String expected = "{ \"_id\" : " + id
                + " , \"b\" : 2147483648 , \"c\" : -2147483649 }";
        testLong( id, expected );
        BSON.setJSCompatibility( true );
        id = 4;
        expected = "{ \"_id\" : " + id
                + " , \"b\" : 2147483648 , \"c\" : -2147483649 }";
        testLong( id, expected );
        // 大于 （2^53 - 1）小于Long.MAX的long类型的情况
        BSON.setJSCompatibility( false );
        id = 5;
        expected = "{ \"_id\" : " + id
                + " , \"b\" : 9223372036854775807 , \"c\" : -9223372036854775808 }";
        testLong2( id, expected );

        BSON.setJSCompatibility( true );
        id = 6;
        expected = "{ \"_id\" : " + id
                + " , \"b\" : { \"$numberLong\" : \"9223372036854775807\" } , \"c\" : { \"$numberLong\" : \"-9223372036854775808\" } }";
        testLong2( id, expected );

        // 2的53次方9007199254740992
        // [-(2^53-1)，2^53-1]
        BSON.setJSCompatibility( false );
        id = 7;
        expected = "{ \"_id\" : " + id
                + " , \"a\" : 123 , \"b\" : -9007199254740991 , \"c\" : 9007199254740991 }";
        test2_53( id, expected );
        BSON.setJSCompatibility( true );
        id = 8;
        expected = "{ \"_id\" : " + id
                + " , \"a\" : 123 , \"b\" : -9007199254740991 , \"c\" : 9007199254740991 }";
        test2_53( id, expected );
    }

    /**
     * [-(2^53-1)，2^53-1]
     * 
     * @param id
     * @param expected
     */
    public void test2_53( int id, String expected ) {
        try {
            BSONObject obj = new BasicBSONObject();
            // 整形和边界值
            obj.put( "_id", id );
            obj.put( "a", 123 );
            obj.put( "b", -( 9007199254740992L - 1L ) );
            obj.put( "c", 9007199254740992L - 1L );
            this.cl.insert( obj );
            DBCursor cursor = this.cl.query( "{_id:{'$et':" + id + "}}", null,
                    null, null );
            BSONObject next = new BasicBSONObject();
            while ( cursor.hasNext() ) {
                next = cursor.getNext();
            }
            cursor.close();
            Assert.assertEquals( next.toString(), expected );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public void testInt( int id ) {
        try {
            BSONObject obj = new BasicBSONObject();
            // 整形和边界值
            obj.put( "_id", id );
            obj.put( "a", 123 );
            obj.put( "b", Integer.MAX_VALUE );
            obj.put( "c", Integer.MIN_VALUE );
            this.cl.insert( obj );
            DBCursor cursor = this.cl.query( "{_id:{'$et':" + id + "}}", null,
                    null, null );
            String expected = "{ \"_id\" : " + id
                    + " , \"a\" : 123 , \"b\" : 2147483647 , \"c\" : -2147483648 }";
            BSONObject next = new BasicBSONObject();
            while ( cursor.hasNext() ) {
                next = cursor.getNext();
            }
            cursor.close();
            Assert.assertEquals( next.toString(), expected );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    // 小于 （2^53 - 1）的long类型的情况
    public void testLong( int id, String expected ) {
        try {
            BSONObject obj = new BasicBSONObject();
            // 整形和边界值
            obj.put( "_id", id );
            obj.put( "b", Integer.MAX_VALUE + 1L );
            obj.put( "c", Integer.MIN_VALUE - 1L );
            this.cl.insert( obj );
            DBCursor cursor = this.cl.query( "{_id:{'$et':" + id + "}}", null,
                    null, null );
            BSONObject next = new BasicBSONObject();
            while ( cursor.hasNext() ) {
                next = cursor.getNext();
            }
            cursor.close();
            Assert.assertEquals( next.toString(), expected );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    // 边界值
    public void testLong2( int id, String expected ) {
        try {
            BSONObject obj = new BasicBSONObject();
            // 整形和边界值
            obj.put( "_id", id );
            obj.put( "b", Long.MAX_VALUE );
            obj.put( "c", Long.MIN_VALUE );
            this.cl.insert( obj );
            DBCursor cursor = this.cl.query( "{_id:{'$et':" + id + "}}", null,
                    null, null );
            BSONObject next = new BasicBSONObject();
            while ( cursor.hasNext() ) {
                next = cursor.getNext();
            }
            cursor.close();
            Assert.assertEquals( next.toString(), expected );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( this.clName ) ) {
                this.cs.dropCollection( this.clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            this.sdb.disconnect();
        }
    }
}
