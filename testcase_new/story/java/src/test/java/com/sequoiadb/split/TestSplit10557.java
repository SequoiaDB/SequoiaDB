package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
 * 1、执行splitAsync，设置范围切分条件，如范围区间a为【20,50】 2、向cl中插入数据（源和目标组分区范围内数据）
 * 3、执行split，设置范围切分条件b，其中a为b的子集，如b范围区间为【10,60】 4、查看数据切分结果
 * 
 * @author chensiqin
 * @Date 2016-12-21
 *
 */
public class TestSplit10557 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10557";
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
                    "{ShardingKey:{age:1},ShardingType:\"range\",Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl = SplitUtils2.createCL( this.cs, this.clName, option );

            // 第一次执行切分
            this.cl.split( rgNames.get( 0 ), rgNames.get( 1 ),
                    ( BSONObject ) JSON.parse( "{age:30}" ),
                    ( BSONObject ) JSON.parse( "{age:40}" ) );

            // 插入记录
            insertData();

            int splitNum = 1;

            // 第一次切分检查数据正确性
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames, splitNum );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames, splitNum );

            // 第二次执行切分
            BSONObject startCondition = ( BSONObject ) JSON.parse( "{age:20}" );
            BSONObject endCondition = ( BSONObject ) JSON.parse( "{age:70}" );
            long id1 = this.cl.splitAsync( rgNames.get( 0 ), rgNames.get( 1 ),
                    startCondition, endCondition );
            long[] taskIDs = { id1 };
            this.sdb.waitTasks( taskIDs );

            splitNum = 2;
            // 第二次次切分，检查数据正确性
            // 连接coord节点查看数据正确性
            testCoordSplitResult( rgNames );
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames, splitNum );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames, splitNum );

        } catch ( BaseException e ) {
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
     * @param splitNum
     *            第几次切分
     */
    public void testSrcDataSplitResult( List< String > rgNames, int splitNum ) {
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
            if ( splitNum == 1 ) { // 第一次切分后校验结果
                // 切分范围[30,40),对应的期望结果是[1,30),[40,100)
                for ( int i = 1; i < 30; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
                }
                for ( int i = 40; i < 100; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
                }
            } else if ( splitNum == 2 ) { // 第二次切分后校验结果
                // 切分范围[20,70),对应的期望结果是[1,20),[70,100)
                for ( int i = 1; i < 20; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
                }
                for ( int i = 70; i < 100; i++ ) {
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
     * @param splitNum
     *            第几次切分
     */
    public void testDestDataSplitResult( List< String > rgNames,
            int splitNum ) {
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
            if ( splitNum == 1 ) { // 第一次切分后校验结果
                // 切分范围[30,40),对应的期望结果是[20,50)
                for ( int i = 30; i < 40; i++ ) {
                    expected.add( this.insertRecods.get( i - 1 ) );
                }
            } else if ( splitNum == 2 ) { // 第二次切分后校验结果
                // 切分范围[20,70),对应的期望结果是[1,60)
                for ( int i = 20; i < 70; i++ ) {
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

    public void insertData() {
        this.insertRecods = new ArrayList< BSONObject >();
        try {
            BSONObject bson = null;
            for ( int i = 1; i < 100; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "age", i );
                bson.put( "num", ( ( i % 3 ) * 1.0 ) / 100 );
                long height = i % 2;
                bson.put( "height", height );

                BSONObject arr = new BasicBSONList();
                arr.put( "0", i % 3 );
                bson.put( "arrtype", arr );

                this.insertRecods.add( bson );
            }
            cl.bulkInsert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            e.printStackTrace();
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
