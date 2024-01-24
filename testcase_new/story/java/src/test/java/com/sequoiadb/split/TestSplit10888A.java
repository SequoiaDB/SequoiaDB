package com.sequoiadb.split;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
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
 * hash分区表指定指定多个分区键执行异步切分
 * 
 * @author chensiqin
 * @Date 2016-12-21
 *
 */
public class TestSplit10888A extends SdbTestBase {
    private Sequoiadb sdb;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName1 = "cl10888A_0";
    private String clName2 = "cl10888A_1";
    private String clName3 = "cl10888A_2";
    private ArrayList< BSONObject > insertRecods;
    /*
     * 组合类型: a、数组、long、float、timestamp b、int、date、decimal c、_id+float
     */
    private int combinationType;

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

    /**
     * 分区键字段分别覆盖多种数据类型，参考如下组合： 数组、long、float、timestamp
     */
    @Test
    public void testCombination1() {
        try {
            this.combinationType = 1;
            List< String > rgNames = SplitUtils2.getDataRgNames( this.sdb );
            BSONObject option = ( BSONObject ) JSON.parse(
                    "{ShardingKey:{arrtype:1,height:-1,num:-1,timestamp:1},"
                            + "ShardingType:\"hash\",Partition:1024,Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl = SplitUtils2.createCL( this.cs, this.clName1, option );
            insertData( 1000 );

            BSONObject startCondition = new BasicBSONObject();
            BSONObject endCondition = new BasicBSONObject();
            startCondition.put( "Partition", 0 );
            endCondition.put( "Partition", 512 );
            long splitAsync1 = this.cl.splitAsync( rgNames.get( 0 ),
                    rgNames.get( 1 ), startCondition, endCondition );
            long[] taskIDs = { splitAsync1 };
            this.sdb.waitTasks( taskIDs );
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames );
            // 连接coord节点验证数据是否正确
            testCoordSplitResult( rgNames );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 分区键字段分别覆盖多种数据类型，参考如下组合： int、date、decimal
     */
    @Test
    public void testCombination2() {
        try {
            this.combinationType = 2;
            List< String > rgNames = SplitUtils2.getDataRgNames( this.sdb );
            BSONObject option = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{_id:1,date:1,bdecimal:-1},"
                            + "ShardingType:\"hash\",Partition:1024,Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl = SplitUtils2.createCL( this.cs, this.clName2, option );
            insertData( 1000 );
            BSONObject startCondition = new BasicBSONObject();
            BSONObject endCondition = new BasicBSONObject();
            startCondition.put( "Partition", 512 );
            endCondition.put( "Partition", 1024 );
            long splitAsync2 = this.cl.splitAsync( rgNames.get( 0 ),
                    rgNames.get( 1 ), startCondition, endCondition );
            long[] taskIDs = { splitAsync2 };
            this.sdb.waitTasks( taskIDs );
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames );
            // 连接coord节点验证数据是否正确
            testCoordSplitResult( rgNames );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    /**
     * 分区键字段分别覆盖多种数据类型，参考如下组合： _id+float
     */
    @Test
    public void testCombination3() {
        try {
            this.combinationType = 3;
            List< String > rgNames = SplitUtils2.getDataRgNames( this.sdb );
            BSONObject option = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{_id:-1,num:1},"
                            + "ShardingType:\"hash\",Partition:1024,Group:\""
                            + rgNames.get( 0 ) + "\"}" );
            this.cl = SplitUtils2.createCL( this.cs, this.clName3, option );
            insertData( 1000 );

            BSONObject startCondition = new BasicBSONObject();
            BSONObject endCondition = new BasicBSONObject();
            startCondition.put( "Partition", 768 );
            endCondition.put( "Partition", 1024 );
            long splitAsync3 = this.cl.splitAsync( rgNames.get( 0 ),
                    rgNames.get( 1 ), startCondition, endCondition );
            long[] taskIDs = { splitAsync3 };
            this.sdb.waitTasks( taskIDs );
            // 连接源组data验证数据
            testSrcDataSplitResult( rgNames );
            // 连接目标组data验证数据
            testDestDataSplitResult( rgNames );
            // 连接coord节点验证数据是否正确
            testCoordSplitResult( rgNames );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void testCoordSplitResult( List< String > rgNames ) {
        try {
            // 连接coord节点验证数据是否正确
            List< BSONObject > actual = new ArrayList< BSONObject >();
            DBCursor cursor = this.cl.query( null, null, "{\"_id\":1}", null );
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
            DBCollection dbcl = null;
            if ( this.combinationType == 1 ) {
                dbcl = cs.getCollection( this.clName1 );
            } else if ( this.combinationType == 2 ) {
                dbcl = cs.getCollection( this.clName2 );
            } else if ( this.combinationType == 3 ) {
                dbcl = cs.getCollection( this.clName3 );
            }
            long actual = dbcl.getCount();

            if ( this.combinationType == 1 ) {
                // partition[0,512]
                if ( Math.abs( ( 500 - actual ) ) / 500.0 > 0.3 ) {
                    Assert.fail( "hash 范围切分不够准确！" );
                }
            } else if ( this.combinationType == 2 ) {
                // partition[512,1024]
                if ( Math.abs( ( 500 - actual ) ) / 500.0 > 0.3 ) {
                    Assert.fail( "hash 范围切分不够准确！" );
                }
            } else {
                // partition[768,1024]
                if ( Math.abs( ( 750 - actual ) ) / 750.0 > 0.3 ) {
                    Assert.fail( "hash 范围切分不够准确！" );
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
    public void testDestDataSplitResult( List< String > rgNames ) {
        Sequoiadb dataDb = null;
        try {
            // 连接目标组data查询
            String url = SplitUtils2.getGroupIPByGroupName( this.sdb,
                    rgNames.get( 1 ) );
            dataDb = new Sequoiadb( url, "", "" );
            CollectionSpace cs = dataDb
                    .getCollectionSpace( SdbTestBase.csName );
            DBCollection dbcl = null;
            if ( this.combinationType == 1 ) {
                dbcl = cs.getCollection( this.clName1 );
            } else if ( this.combinationType == 2 ) {
                dbcl = cs.getCollection( this.clName2 );
            } else if ( this.combinationType == 3 ) {
                dbcl = cs.getCollection( this.clName3 );
            }
            long actual = dbcl.getCount();
            if ( this.combinationType == 1 ) {
                // partition[0,512]
                if ( Math.abs( ( 500 - actual ) ) / 500.0 > 0.3 ) {
                    Assert.fail( "hash 范围切分不够准确！" );
                }
            } else if ( this.combinationType == 2 ) {
                // partition[512,1024]
                if ( Math.abs( ( 500 - actual ) ) / 500.0 > 0.3 ) {
                    Assert.fail( "hash 范围切分不够准确！" );
                }
            } else {
                // partition[768,1024]
                if ( Math.abs( ( 250 - actual ) ) / 250.0 > 0.3 ) {
                    Assert.fail( "hash 范围切分不够准确！" );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            dataDb.disconnect();
        }

    }

    public void insertData( int len ) {
        this.insertRecods = new ArrayList< BSONObject >();
        try {
            BSONObject bson = null;
            for ( int i = 1; i < len; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "_id", i );
                bson.put( "age", i - 5 );
                bson.put( "num", ( ( i % 3 ) * 1.0 ) / 100 );
                long height = i * 2;
                bson.put( "height", height );
                SimpleDateFormat sdf = new SimpleDateFormat( "yyyy-MM-dd" );
                Date date = null;
                String str = i + "";
                try {
                    if ( i < 10 ) {
                        str = "0" + str;
                    }
                    date = sdf.parse( "2016-12-" + str );
                } catch ( ParseException e ) {
                    e.printStackTrace();
                }
                bson.put( "date", date );
                // String mydate = "2014-07-01 12:30:30." + 1111111;
                str = "2016-12-0" + i;
                str = str + " 12:30:0" + i + "."
                        + ( 100000 + ( 123456 + i ) % 100000 );
                String mydate = str;
                String dateStr = mydate.substring( 0,
                        mydate.lastIndexOf( '.' ) );
                String incStr = mydate
                        .substring( mydate.lastIndexOf( '.' ) + 1 );
                SimpleDateFormat format = new SimpleDateFormat(
                        "yyyy-MM-dd HH:mm:ss" );
                try {
                    date = format.parse( dateStr );
                } catch ( ParseException e ) {
                    e.printStackTrace();
                }
                int seconds = ( int ) ( date.getTime() / 1000 );
                int inc = Integer.parseInt( incStr );
                BSONTimestamp ts = new BSONTimestamp( seconds, inc );
                bson.put( "timestamp", ts );

                str = "12345.06789123456789012345" + i * 100;
                BSONDecimal decimal = new BSONDecimal( str );
                bson.put( "bdecimal", decimal );

                BSONObject arr = new BasicBSONList();
                arr.put( "0", i % 3 );
                bson.put( "arrtype", arr );

                this.insertRecods.add( bson );
            }
            cl.bulkInsert( this.insertRecods, 0 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail();
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
            if ( this.cs.isCollectionExist( this.clName3 ) ) {
                this.cs.dropCollection( this.clName3 );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            this.sdb.disconnect();
        }
    }
}
