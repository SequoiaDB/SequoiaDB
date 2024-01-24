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
 * 指定多个分区键执行切分，其中一个分区键字段切分范围不匹配
 * 
 * @author chensiqin
 * @Date 2016-12-21
 *
 */
public class TestSplit10524 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10524";
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
                    "{ShardingKey:{num:1,age:1},ShardingType:\"range\",Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl = SplitUtils2.createCL( this.cs, this.clName, option );

            insertData();

            BSONObject startCondition = ( BSONObject ) JSON
                    .parse( "{num:0.01,age:-9}" );
            BSONObject endCondition = ( BSONObject ) JSON
                    .parse( "{num:0.01,age:2}" );
            this.cl.split( rgNames.get( 0 ), rgNames.get( 1 ), startCondition,
                    endCondition );

            // 连接coord节点验证数据是否正确
            testCoordSplitResult( rgNames );
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames );

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
     * @param isReverse
     */
    public void testSrcDataSplitResult( List< String > rgNames ) {
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
            // 期望结果是2,3,5,6,7,8,9
            expected.add( this.insertRecods.get( 2 - 1 ) );
            expected.add( this.insertRecods.get( 3 - 1 ) );
            expected.add( this.insertRecods.get( 5 - 1 ) );
            expected.add( this.insertRecods.get( 6 - 1 ) );
            expected.add( this.insertRecods.get( 7 - 1 ) );
            expected.add( this.insertRecods.get( 8 - 1 ) );
            expected.add( this.insertRecods.get( 9 - 1 ) );
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
    public void testDestDataSplitResult( List< String > rgNames ) {
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
            // 期望结果是1,4
            expected.add( this.insertRecods.get( 0 ) );
            expected.add( this.insertRecods.get( 3 ) );
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
            for ( int i = 1; i < 10; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "age", i - 5 );
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
