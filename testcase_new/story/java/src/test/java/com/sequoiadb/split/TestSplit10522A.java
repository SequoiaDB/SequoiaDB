package com.sequoiadb.split;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONTimestamp;
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
 * 1、创建主表、子表，指定多个分区键字段，其中字段排序都为逆序，子表已成功挂载到主表
 * 2、向cl中插入数据，其中分区键字段分别覆盖多种数据类型，参考如下组合： a、数组、long、float、timestamp
 * b、int、date、decimal c、_id+float 3、执行split，设置范围切分条件
 * 4、查看数据切分结果（分别连接coord、源组data、目标组data查询
 * 
 * @author chensiqin
 * @Date 2016-12-21
 *
 */
public class TestSplit10522A extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection mainCL;
    private DBCollection subCL;
    private String mainCLName = "mainCL10522_0";
    private String subCLName = "subCL10522_0";
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
        // 得到数据组
        List< String > rgNames = SplitUtils2.getDataRgNames( this.sdb );
        // 创建主表
        BSONObject option = ( BSONObject ) JSON.parse(
                "{IsMainCL:true,ShardingKey:{arrtype:-1,height:-1,num:-1,timestamp:-1},ShardingType:\"range\"}" );
        this.mainCL = SplitUtils2.createCL( this.cs, this.mainCLName, option );

        // a、数组、long、float、timestamp
        option = ( BSONObject ) JSON.parse(
                "{ShardingKey:{arrtype:-1,height:-1,num:-1,timestamp:-1},ShardingType:\"range\",Group:\""
                        + rgNames.get( 0 ) + "\"}" );
        this.subCL = SplitUtils2.createCL( this.cs, this.subCLName, option );
        // 挂载子表
        option = ( BSONObject ) JSON
                .parse( "{LowBound:{arrtype:11,height:11,num:0.2,"
                        + "timestamp:{\"$timestamp\":\"2016-11-30-00.00.00.000000\"}},"
                        + "UpBound:{arrtype:-5,height:-5,num:0.0001,"
                        + "timestamp:{\"$timestamp\":\"2016-12-03-23.59.59.999999\"}}}" );
        this.mainCL.attachCollection( SdbTestBase.csName + "." + this.subCLName,
                option );
        this.insertRecods = ( ArrayList< BSONObject > ) SplitUtils2
                .insertData( this.mainCL, 10 );

        BSONObject arr1 = new BasicBSONList();
        arr1.put( "0", 8 );
        String mydate = "2016-11-30 23:59:59.999999";
        String dateStr = mydate.substring( 0, mydate.lastIndexOf( '.' ) );
        String incStr = mydate.substring( mydate.lastIndexOf( '.' ) + 1 );
        SimpleDateFormat format = new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" );
        Date date = null;
        try {
            date = format.parse( dateStr );
        } catch ( ParseException e ) {
            e.printStackTrace();
        }
        int seconds = ( int ) ( date.getTime() / 1000 );
        int inc = Integer.parseInt( incStr );
        BSONTimestamp ts1 = new BSONTimestamp( seconds, inc );
        BSONObject startCondition = new BasicBSONObject();
        startCondition.put( "arrtype", arr1 );
        startCondition.put( "height", 6 );
        startCondition.put( "num", 0.2 );
        startCondition.put( "timestamp", ts1 );

        BSONObject arr2 = new BasicBSONList();
        arr2.put( "0", 1 );
        mydate = "2016-11-30 00:00:00.000000";
        dateStr = mydate.substring( 0, mydate.lastIndexOf( '.' ) );
        incStr = mydate.substring( mydate.lastIndexOf( '.' ) + 1 );
        format = new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss" );
        try {
            date = format.parse( dateStr );
        } catch ( ParseException e ) {
            e.printStackTrace();
        }
        seconds = ( int ) ( date.getTime() / 1000 );
        inc = Integer.parseInt( incStr );
        BSONTimestamp ts2 = new BSONTimestamp( seconds, inc );
        BSONObject endCondition = new BasicBSONObject();
        endCondition.put( "arrtype", arr2 );
        endCondition.put( "height", 2 );
        endCondition.put( "num", 0.01 );
        endCondition.put( "timestamp", ts2 );
        // 执行切分
        this.subCL.split( rgNames.get( 0 ), rgNames.get( 1 ), startCondition,
                endCondition );

        // 主表连接coord节点验证数据是否正确
        testCoordSplitResult( rgNames );
        // 子表连接源组data验证数据
        testSrcDataSplitResult( rgNames );
        // 子表连接目标组data验证数据
        testDestDataSplitResult( rgNames );
    }

    public void testCoordSplitResult( List< String > rgNames ) {
        try {
            // 连接coord节点验证数据是否正确
            List< BSONObject > actual = new ArrayList< BSONObject >();
            DBCursor cursor = this.mainCL.query( null, null, "{\"_id\":1}",
                    null );
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
            DBCollection dbcl = cs.getCollection( this.subCLName );
            DBCursor cursor = dbcl.query( null, null, "{\"_id\":1}", null );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            cursor.close();
            List< BSONObject > expected = new ArrayList< BSONObject >();
            // 期望值
            expected.add( this.insertRecods.get( 1 - 1 ) );
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
            DBCollection dbcl = cs.getCollection( this.subCLName );
            DBCursor cursor = dbcl.query( null, null, "{\"_id\":1}", null );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            cursor.close();
            List< BSONObject > expected = new ArrayList< BSONObject >();
            // 期望结果
            for ( int i = 2; i <= 7; i++ ) {
                expected.add( this.insertRecods.get( i - 1 ) );
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
            if ( this.cs.isCollectionExist( this.mainCLName ) ) {
                this.cs.dropCollection( this.mainCLName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            this.sdb.disconnect();
        }
    }
}
