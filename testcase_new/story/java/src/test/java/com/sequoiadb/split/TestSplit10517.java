package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.Binary;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
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
 * 分区键为binary类型，执行范围切分
 * 
 * @author chensiqin
 * @Date 2016-12-20
 *
 */
public class TestSplit10517 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName1 = "cl10517_0";
    private String clName2 = "cl10517_1";
    private ArrayList< BSONObject > insertRecods;

    @BeforeClass
    public void setUp() {
        try {
            this.sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // 跳过 standAlone 和数据组不足的环境
            SplitUtils2 util = new SplitUtils2();
            if ( util.isStandAlone( this.sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            if ( SplitUtils2.getDataRgNames( this.sdb ).size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            BSONObject options = new BasicBSONObject();
            options.put( "PreferedInstance", "M" );
            this.sdb.setSessionAttr( options );
            this.cs = this.sdb.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    @Test
    public void test() {
        try {
            List< String > rgNames = SplitUtils2.getDataRgNames( this.sdb );
            BSONObject option = ( BSONObject ) JSON.parse(
                    "{ShardingKey:{bindata:1},ShardingType:\"range\",Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl = SplitUtils2.createCL( this.cs, this.clName1, option );
            insertData();
            String str = "hello world" + 500;
            byte[] arr = str.getBytes();
            Binary b1 = new Binary( arr );
            str = "hello world" + 8999;
            arr = str.getBytes();
            Binary b2 = new Binary( arr );
            BSONObject startCondition = new BasicBSONObject();
            BSONObject endCondition = new BasicBSONObject();
            startCondition.put( "bindata", b1 );
            endCondition.put( "bindata", b2 );
            this.cl.split( rgNames.get( 0 ), rgNames.get( 1 ), startCondition,
                    endCondition );
            // 连接coord节点验证数据是否正确
            testCoordSplitResult( rgNames );
            boolean isReverse = false;
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames, isReverse );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames, isReverse );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 1. 创建创建cl到指定组 2. 分区键字段排序为逆序，且切分范围与正序不同 3. 执行切分 4.
     * 分别连接coord、源组data、目标组data查询
     */
    @Test
    public void testReverse() {
        try {
            List< String > rgNames = SplitUtils2.getDataRgNames( this.sdb );
            BSONObject option = ( BSONObject ) JSON.parse(
                    "{ShardingKey:{bindata:-1},ShardingType:\"range\",Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl = SplitUtils2.createCL( this.cs, this.clName2, option );
            insertData();
            String str = "hello world" + 7999;
            byte[] arr = str.getBytes();
            Binary b1 = new Binary( arr );
            str = "hello world" + 1999;
            arr = str.getBytes();
            Binary b2 = new Binary( arr );
            BSONObject startCondition = new BasicBSONObject();
            BSONObject endCondition = new BasicBSONObject();
            startCondition.put( "bindata", b1 );
            endCondition.put( "bindata", b2 );
            this.cl.split( rgNames.get( 0 ), rgNames.get( 1 ), startCondition,
                    endCondition );
            // 连接coord节点验证数据是否正确
            testCoordSplitResult( rgNames );
            boolean isReverse = true;
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames, isReverse );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames, isReverse );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void testCoordSplitResult( List< String > rgNames ) {
        try {
            // 连接coord节点验证数据是否正确
            List< BSONObject > actual = new ArrayList< BSONObject >();
            List< BSONObject > expected = new ArrayList< BSONObject >();
            DBCursor cursor = this.cl.query( null, null, "{\"age\":1}", null );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                obj.removeField( "bindata" );
                actual.add( obj );
            }
            cursor.close();
            for ( int i = 1; i < 100; i++ ) {
                BSONObject o = this.insertRecods.get( i - 1 );
                o.removeField( "bindata" );
                expected.add( o );
            }
            Assert.assertEquals( actual, expected );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }

    }

    /**
     * 连接源数据组检查切分结果
     * 
     * @param rgNames
     * @param isReverse
     */
    public void testSrcDataSplitResult( List< String > rgNames,
            boolean isReverse ) {
        Sequoiadb dataDb = null;
        try {
            // 连接源组data验证数据
            String url = SplitUtils2.getGroupIPByGroupName( this.sdb,
                    rgNames.get( 0 ) );
            dataDb = new Sequoiadb( url, "", "" );
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = null;
            if ( isReverse ) {
                dbcl = cs.getCollection( this.clName2 );
            } else {
                dbcl = cs.getCollection( this.clName1 );
            }
            DBCursor cursor = dbcl.query( null, null, "{\"age\":1}", null );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                obj.removeField( "bindata" );
                actual.add( obj );
            }
            cursor.close();
            List< BSONObject > expected = new ArrayList< BSONObject >();
            if ( isReverse ) {
                // 当切分键[7999,1999)
                for ( int i = 1; i <= 19; i++ ) {
                    BSONObject o = this.insertRecods.get( i - 1 );
                    o.removeField( "bindata" );
                    expected.add( o );
                }
                for ( int i = 80; i <= 99; i++ ) {
                    BSONObject o = this.insertRecods.get( i - 1 );
                    o.removeField( "bindata" );
                    expected.add( o );
                }
            } else {
                // [500,899)在组上的期望结果对应的是
                for ( int i = 1; i <= 4; i++ ) {
                    BSONObject o = this.insertRecods.get( i - 1 );
                    o.removeField( "bindata" );
                    expected.add( o );
                }
                for ( int i = 90; i <= 99; i++ ) {
                    BSONObject o = this.insertRecods.get( i - 1 );
                    o.removeField( "bindata" );
                    expected.add( o );
                }
            }
            Assert.assertEquals( actual, expected );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            dataDb.disconnect();
        }

    }

    /**
     * 在目标数组组上查询切分结果
     * 
     * @param rgNames
     * @param isReverse
     */
    public void testDestDataSplitResult( List< String > rgNames,
            boolean isReverse ) {
        Sequoiadb dataDb = null;
        try {
            // 连接目标组data查询
            String url = SplitUtils2.getGroupIPByGroupName( this.sdb,
                    rgNames.get( 1 ) );
            dataDb = new Sequoiadb( url, "", "" );
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = null;
            if ( isReverse ) {
                dbcl = cs.getCollection( this.clName2 );
            } else {
                dbcl = cs.getCollection( this.clName1 );
            }
            DBCursor cursor = dbcl.query( null, null, "{\"age\":1}", null );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                obj.removeField( "bindata" );
                actual.add( obj );
            }
            cursor.close();
            List< BSONObject > expected = new ArrayList< BSONObject >();
            if ( isReverse ) {
                // 逆序时[7999,1999)
                for ( int i = 20; i <= 79; i++ ) {
                    BSONObject o = this.insertRecods.get( i - 1 );
                    o.removeField( "bindata" );
                    expected.add( o );
                }
            } else {
                // [500,899)在组上的期望结果对应的是
                for ( int i = 5; i <= 89; i++ ) {
                    BSONObject o = this.insertRecods.get( i - 1 );
                    o.removeField( "bindata" );
                    expected.add( o );
                }
            }
            Assert.assertEquals( actual, expected );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            dataDb.disconnect();
        }

    }

    public void insertData() {
        insertRecods = new ArrayList< BSONObject >();
        try {
            BSONObject bson = null;
            for ( int i = 1; i < 100; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "str", i + "abc" );
                bson.put( "age", i );
                String str = "12345.06789123456789012345" + i * 100;
                BSONDecimal decimal = new BSONDecimal( str );
                bson.put( "bdecimal", decimal );
                str = "hello world" + i * 100;
                byte[] arr = str.getBytes();
                Binary bindata = new Binary( arr );
                bson.put( "bindata", bindata );
                this.insertRecods.add( bson );
            }
            cl.bulkInsert( insertRecods, 0 );
        } catch ( BaseException e ) {
            e.printStackTrace();
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( this.clName1 ) ) {
                this.cs.dropCollection( this.clName1 );
            }
            if ( this.cs.isCollectionExist( this.clName2 ) ) {
                this.cs.dropCollection( this.clName2 );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            this.sdb.disconnect();
        }
    }
}
