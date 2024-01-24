package com.sequoiadb.split;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.regex.Pattern;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BSONTimestamp;
import org.bson.types.BasicBSONList;
import org.bson.types.Binary;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * hash分区表指定分区键为多种数据类型，执行切分 ,比较类型 1、创建cl，分区类型为“range”
 * 2、向cl中插入数据，其中分区键字段数据类型包含sequoiadb支持的所有数据类型
 * （long、int、double、decimal、string、OID、bool、date、timestamp、binary、正则表达式、对象、数组、空、minKey、maxKey）
 * 3、执行split，设置范围切分条件
 * （切分键字段要分别覆盖所有的数据类型，如db.cs.cl.split("group1","group2",{a:int类型数据})）
 * 4、查看数据切分结果（分别连接coord、源组data、目标组data查询
 * 
 * @author chensiqin
 *
 */
public class TestSplit10896 extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl10896";
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

    @DataProvider(name = "providerMethod")
    public Object[][] providerMethod() {
        String all = "[{ \"_id\" : 1 , \"type\" : 123 }, "
                + "{ \"_id\" : 2 , \"type\" : 456 }, { \"_id\" : 3 , \"type\" : 123.456 }, "
                + "{ \"_id\" : 4 , \"type\" : { \"$decimal\" : \"12345.06789123456789012345100\" } }, "
                + "{ \"_id\" : 5 , \"type\" : \"zhangsan\" }, "
                + "{ \"_id\" : 6 , \"type\" : { \"$oid\" : \"15713f7953e6769804000001\" } }, "
                + "{ \"_id\" : 7 , \"type\" : true }, "
                + "{ \"_id\" : 8 , \"type\" : { \"$date\" : \"2016-12-12\" } }, "
                + "{ \"_id\" : 9 , \"type\" : { \"$ts\" : 1404189030 , \"$inc\" : 124232 } }, "
                + "{ \"_id\" : 10 , \"type\" : { \"$binary\" : \"aGVsbG8gd29ybGQ=\" , \"$type\" : \"0\" } }, "
                + "{ \"_id\" : 11 , \"type\" : { \"$regex\" : \"^2001\" , \"$options\" : \"i\" } }, "
                + "{ \"_id\" : 12 , \"type\" : { \"a\" : 1 } }, { \"_id\" : 13 , \"type\" : [ 0 ] }, "
                + "{ \"_id\" : 14 , \"type\" :  null  }, { \"_id\" : 15 , \"type\" : { \"$minKey\" : 1 } }, "
                + "{ \"_id\" : 16 , \"type\" : { \"$maxKey\" : 1 } }]";
        /*
         * String queryRe =
         * "[{ \"_id\" : 8 , \"type\" : { \"$date\" : \"2016-12-12\"}}, " +
         * "{ \"_id\" : 9 , \"type\" : { \"$ts\" : 1404189030 , \"$inc\" : 124232}}, "
         * +
         * "{ \"_id\" : 11 , \"type\" : { \"$regex\" : \"^2001\" , \"$options\" : \"i\"}}, "
         * + "{ \"_id\" : 16 , \"type\" : { \"$maxKey\" : 1}}]";
         */
        return new Object[][] { { all } };
    }

    @Test(dataProvider = "providerMethod")
    public void test( String all ) {
        try {
            // 得到数据组
            List< String > rgNames = SplitUtils2.getDataRgNames( this.sdb );
            BSONObject option = new BasicBSONObject();
            option = ( BSONObject ) JSON.parse( "{ShardingKey:{type:-1},"
                    + "ShardingType:\"hash\",Partition:1024,Group:\""
                    + rgNames.get( 0 ) + "\"}" );
            // 创建cl
            this.cl = SplitUtils2.createCL( this.cs, this.clName, option );
            insertData();
            // 执行切分
            BSONObject startCondition = new BasicBSONObject();
            BSONObject endCondition = new BasicBSONObject();
            startCondition.put( "Partition", 0 );
            endCondition.put( "Partition", 410 );
            this.cl.split( rgNames.get( 0 ), rgNames.get( 1 ), startCondition,
                    endCondition );
            BSONObject detail = getSnapshot8();
            Object object = detail.get( "CataInfo" );
            BSONObject cataInfo = ( BSONObject ) object;
            Assert.assertTrue( cataInfo.toString().contains(
                    "\"LowBound\" : { \"\" : 0 } , \"UpBound\" : { \"\" : 410 }" ) );
            Assert.assertTrue( cataInfo.toString().contains(
                    "\"LowBound\" : { \"\" : 410 } , \"UpBound\" : { \"\" : 1024 }" ) );

            testCoordSplitResult( rgNames, all );
            // 验证部分数据
            testDataWithCondition();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    private void testDataWithCondition() {
        try {
            BSONObject o = new BasicBSONObject();
            o.put( "$gt", 1 );
            BSONObject matcher = new BasicBSONObject();
            matcher.put( "type", o );
            DBCursor cursor = this.cl.query( matcher, null,
                    ( BSONObject ) JSON.parse( "{\"_id\":1}" ), null );
            List< BSONObject > actual = new ArrayList< BSONObject >();
            List< BSONObject > expected = new ArrayList< BSONObject >();
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            cursor.close();
            // 期望结果
            expected.add( this.insertRecords.get( 1 - 1 ) );
            expected.add( this.insertRecords.get( 2 - 1 ) );
            expected.add( this.insertRecords.get( 3 - 1 ) );
            expected.add( this.insertRecords.get( 4 - 1 ) );
            Assert.assertEqualsNoOrder( actual.toArray(), expected.toArray() );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public BSONObject getSnapshot8() {
        BSONObject detail = null;
        try {
            detail = new BasicBSONObject();
            // get details of snapshot
            BSONObject nameBSON = new BasicBSONObject();
            nameBSON.put( "Name", SdbTestBase.csName + "." + this.clName );
            DBCursor snapshot = this.sdb.getSnapshot( 8, nameBSON, null, null );
            detail = snapshot.getNext();
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return detail;
    }

    public void testCoordSplitResult( List< String > rgNames, String all ) {
        try {
            // 连接coord节点验证数据是否正确
            List< BSONObject > actual = new ArrayList< BSONObject >();
            DBCursor cursor = this.cl.query( null, null, "{\"_id\":1}", null );
            while ( cursor.hasNext() ) {
                BSONObject obj = cursor.getNext();
                actual.add( obj );
            }
            cursor.close();
            Assert.assertEquals( actual.toString(), all );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public void insertData() {
        try {
            this.insertRecords = new ArrayList< BSONObject >();
            // long、int、double、decimal、string、OID、bool、date、timestamp、binary、
            // 正则表达式、对象、数组、空、minKey、maxKey
            BSONObject obj = new BasicBSONObject();
            long l = 123;
            obj.put( "type", l );
            obj.put( "_id", 1 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // int
            obj = new BasicBSONObject();
            obj.put( "_id", 2 );
            obj.put( "type", 456 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // double
            obj = new BasicBSONObject();
            obj.put( "_id", 3 );
            obj.put( "type", 123.456 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // decimal
            String str = "12345.06789123456789012345100";
            BSONDecimal decimal = new BSONDecimal( str );
            obj = new BasicBSONObject();
            obj.put( "type", decimal );
            obj.put( "_id", 4 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // string
            obj = new BasicBSONObject();
            obj.put( "type", "zhangsan" );
            obj.put( "_id", 5 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // OID
            ObjectId oid = new ObjectId( "15713f7953e6769804000001" );
            obj = new BasicBSONObject();
            obj.put( "type", oid );
            obj.put( "_id", 6 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // bool
            obj = new BasicBSONObject();
            obj.put( "type", true );
            obj.put( "_id", 7 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // date
            SimpleDateFormat sdf = new SimpleDateFormat( "yyyy-MM-dd" );
            Date date = null;
            try {
                date = sdf.parse( "2016-12-12" );
            } catch ( ParseException e ) {
                e.printStackTrace();
            }
            obj = new BasicBSONObject();
            obj.put( "type", date );
            obj.put( "_id", 8 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // timestamp
            String mydate = "2014-07-01 12:30:30.124232";
            String dateStr = mydate.substring( 0, mydate.lastIndexOf( '.' ) );
            String incStr = mydate.substring( mydate.lastIndexOf( '.' ) + 1 );
            SimpleDateFormat format = new SimpleDateFormat(
                    "yyyy-MM-dd HH:mm:ss" );
            Date date1 = null;
            try {
                date1 = format.parse( dateStr );
            } catch ( ParseException e ) {
                e.printStackTrace();
            }
            int seconds = ( int ) ( date1.getTime() / 1000 );
            int inc = Integer.parseInt( incStr );
            BSONTimestamp ts = new BSONTimestamp( seconds, inc );
            obj = new BasicBSONObject();
            obj.put( "type", ts );
            obj.put( "_id", 9 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // binary
            str = "hello world";
            byte[] arr = str.getBytes();
            Binary bindata = new Binary( arr );
            obj = new BasicBSONObject();
            obj.put( "type", bindata );
            obj.put( "_id", 10 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // 正则表达式
            Pattern pattern = Pattern.compile( "^2001",
                    Pattern.CASE_INSENSITIVE );
            obj = new BasicBSONObject();
            obj.put( "type", pattern );
            obj.put( "_id", 11 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // 对象
            BSONObject subObj = new BasicBSONObject();
            subObj.put( "a", 1 );
            obj = new BasicBSONObject();
            obj.put( "type", subObj );
            obj.put( "_id", 12 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // 数组
            BSONObject arr1 = new BasicBSONList();
            arr1.put( "0", 0 );
            obj = new BasicBSONObject();
            obj.put( "type", arr1 );
            obj.put( "_id", 13 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // 空
            obj = new BasicBSONObject();
            obj.put( "type", null );
            obj.put( "_id", 14 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // minKey
            obj = new BasicBSONObject();
            MinKey minKey = new MinKey();
            obj.put( "type", minKey );
            obj.put( "_id", 15 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
            // maxKey
            obj = new BasicBSONObject();
            MaxKey maxKey = new MaxKey();
            obj.put( "type", maxKey );
            obj.put( "_id", 16 );
            this.insertRecords.add( obj );
            this.cl.insert( obj );
        } catch ( BaseException e ) {
            e.printStackTrace();
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
