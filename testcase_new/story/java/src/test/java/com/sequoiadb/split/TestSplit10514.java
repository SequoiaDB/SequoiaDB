package com.sequoiadb.split;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 分区键为Date类型，执行切分
 * 
 * @author chensiqin
 * @Date 2016-12-16
 *
 */
public class TestSplit10514 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10514";
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
                    "{ShardingKey:{date:1},ShardingType:\"range\",Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl = SplitUtils2.createCL( this.cs, this.clName, option );
            this.insertRecods = ( ArrayList< BSONObject > ) SplitUtils2
                    .insertData( this.cl, 100 );
            // SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
            // Date date1 = null;
            // Date date2 = null;
            // try {
            // date1 = sdf.parse("2017-02-02");
            // date2 = sdf.parse("2017-02-27");
            // } catch (ParseException e) {
            // e.printStackTrace();
            // }
            // BSONObject o1 = new BasicBSONObject();
            // BSONObject o2 = new BasicBSONObject();
            // BSONObject startCondition = new BasicBSONObject();
            // BSONObject endCondition = new BasicBSONObject();
            // o1.put("$date", date1);
            // o2.put("date", date2);
            // startCondition.put("date", date1);
            // endCondition.put("date", date2);
            BSONObject startCondition = ( BSONObject ) JSON
                    .parse( "{date:{\"$date\":\"2017-02-02\"}}" );
            BSONObject endCondition = ( BSONObject ) JSON
                    .parse( "{date:{\"$date\":\"2017-02-27\"}}" );
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
                    "{ShardingKey:{date:-1},ShardingType:\"range\",Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl = SplitUtils2.createCL( this.cs, this.clName, option );
            this.insertRecods = ( ArrayList< BSONObject > ) SplitUtils2
                    .insertData( this.cl, 100 );
            // SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
            // Date date1 = null;
            // Date date2 = null;
            // try {
            // date1 = sdf.parse("2017-03-01");
            // date2 = sdf.parse("2017-01-31");
            // } catch (ParseException e) {
            // e.printStackTrace();
            // }
            // BSONObject startCondition = new BasicBSONObject();
            // BSONObject endCondition = new BasicBSONObject();
            // startCondition.put("date", date1);
            // endCondition.put("date", date2);
            BSONObject startCondition = ( BSONObject ) JSON
                    .parse( "{date:{\"$date\":\"2017-03-01\"}}" );
            BSONObject endCondition = ( BSONObject ) JSON
                    .parse( "{date:{\"$date\":\"2017-01-31\"}}" );
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
            DBCursor cursor = this.cl.query( null, null, "{\"age\":1}", null );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            cursor.close();
            Assert.assertEquals( actual, this.insertRecods );
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
            DBCollection dbcl = cs.getCollection( this.clName );
            DBCursor cursor = dbcl.query( null, null, "{\"age\":1}", null );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            cursor.close();
            List< BSONObject > expected = new ArrayList< BSONObject >();
            if ( isReverse ) {
                // 当切分键[2017-03-01,2017-01-31)
                for ( int i = 1; i <= 62; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
                }
                for ( int i = 92; i <= 99; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
                }
            } else {
                // [2017-02-02,2017-02-27)在组上的期望结果对应的是
                for ( int i = 1; i <= 63; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
                }
                for ( int i = 89; i <= 99; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
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
            DBCollection dbcl = cs.getCollection( this.clName );
            DBCursor cursor = dbcl.query( null, null, "{\"age\":1}", null );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            cursor.close();
            List< BSONObject > expected = new ArrayList< BSONObject >();
            if ( isReverse ) {
                // 逆序时[2017-03-01,2017-01-31)
                for ( int i = 63; i <= 91; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
                }
            } else {
                // [2017-02-02,2017-02-27)在组上的期望结果对应的是
                for ( int i = 64; i <= 88; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
                }
            }
            Assert.assertEquals( actual, expected );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            dataDb.disconnect();
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( this.cs.isCollectionExist( clName ) ) {
                this.cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            this.sdb.disconnect();
        }
    }
}
