package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * hash分区表指定分区键值为对象类型，执行切分
 * 
 * @author chensiqin
 * @Date 2016-12-20
 *
 */
public class TestSplit10884 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl1;
    private DBCollection cl2;
    private String clName1 = "cl10884_0";
    private String clName2 = "cl10884_1";
    private ArrayList< BSONObject > insertRecords;

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
                    "{ShardingKey:{subobj:1},ShardingType:\"hash\",Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl1 = SplitUtils2.createCL( this.cs, this.clName1, option );
            this.insertRecords = ( ArrayList< BSONObject > ) SplitUtils2
                    .insertData( this.cl1, 10000 );
            this.cl1.split( rgNames.get( 0 ), rgNames.get( 1 ), 50 );
            boolean isReverse = false;
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames, isReverse );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames, isReverse );
            // 连接coord节点验证数据是否正确
            testCoordSplitResult( rgNames, isReverse );
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
                    "{ShardingKey:{subobj:-1},ShardingType:\"hash\",Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl2 = SplitUtils2.createCL( this.cs, this.clName2, option );
            this.insertRecords = ( ArrayList< BSONObject > ) SplitUtils2
                    .insertData( this.cl2, 10000 );
            this.cl2.split( rgNames.get( 0 ), rgNames.get( 1 ), 75 );
            boolean isReverse = true;
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames, isReverse );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames, isReverse );
            // 连接coord节点验证数据是否正确
            testCoordSplitResult( rgNames, isReverse );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void testCoordSplitResult( List< String > rgNames,
            boolean isReverse ) {
        try {
            // 连接coord节点验证数据是否正确
            List< BSONObject > actual = new ArrayList< BSONObject >();
            DBCursor cursor = null;
            if ( isReverse ) {
                cursor = this.cl2.query( null, null, "{\"age\":1}", null );
            } else {
                cursor = this.cl1.query( null, null, "{\"age\":1}", null );
            }
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            if ( cursor != null ) {
                cursor.close();
            }
            Assert.assertEquals( actual, this.insertRecords );
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
            long actual = dbcl.getCount();
            if ( isReverse ) {
                // 百分比75%
                if ( Math.abs( ( 2500 - actual ) ) / 2500.0 > 0.3 ) {
                    Assert.fail( "hash 百分比切分不够准确！" );
                }
            } else {
                // 百分比50%
                if ( Math.abs( ( 5000 - actual ) ) / 5000.0 > 0.3 ) {
                    Assert.fail( "hash 百分比切分不够准确！" );
                }
            }
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
            long actual = dbcl.getCount();
            if ( isReverse ) {
                // 百分比75%
                if ( Math.abs( ( 7500 - actual ) ) / 7500.0 > 0.3 ) {
                    Assert.fail( "hash 百分比切分不够准确！" );
                }
            } else {
                // 百分比50%
                if ( Math.abs( ( 5000 - actual ) ) / 5000.0 > 0.3 ) {
                    Assert.fail( "hash 百分比切分不够准确！" );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            dataDb.disconnect();
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
