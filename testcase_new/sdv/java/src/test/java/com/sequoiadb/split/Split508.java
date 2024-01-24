package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:SEQDB-508 指定多个分区键并发切分1.在CS下创建cl，指定多个分区键，如设置ShardingKey：{"a"：1，"b":-
 *                     1} 2、向cl中插入大量数据，如插入1千万条记录 3、并发执行多个split，设置不同的分区键值进行数据切分
 *                     4、查看数据切分是否正确
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split508 extends SdbTestBase {
    private String clName = "testcaseCL508";
    private DBCollection cl;
    private CollectionSpace commCS;
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;

    @BeforeClass
    public void setUp() {
        commSdb = new Sequoiadb( coordUrl, "", "" );
        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( commSdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        if ( CommLib.getDataGroupNames( commSdb ).size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }

        commCS = commSdb.getCollectionSpace( csName );
        cl = commCS.createCollection( clName, ( BSONObject ) JSON.parse(
                "{ReplSize:0,ShardingKey:{\"a\":1,\"b\":-1},ShardingType:\"range\"}" ) );
        ArrayList< String > tmp = SplitUtils.getGroupName( commSdb, csName,
                clName );
        srcGroupName = tmp.get( 0 );
        destGroupName = tmp.get( 1 );
        prepareData( cl );// 写入待切分的记录（30000）

    }

    // 切分范围{a:0,a:10000} - {b:10000,b:0} 切分范围{a:20000,b:30000} {b:30000,b:20000}
    @DataProvider(name = "rangeProvider", parallel = true)
    public Object[][] rangeProvider() {
        return new Object[][] { { 0, 10000, 10000, 0 },
                { 20000, 30000, 30000, 20000 } };
    }

    // 切分{a:0,b:10000} - {a:10000,b:0} 切分{a:20000,b:30000} {a:30000,b:20000}
    @Test(dataProvider = "rangeProvider")
    public void splitCLAndCheckResult( int aLowBound, int aUpBound,
            int bLowBound, int bUpBound ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName );
            dbcl.split( srcGroupName, destGroupName,
                    ( BSONObject ) JSON.parse(
                            "{a:" + aLowBound + ",b:" + bLowBound + "}" ),
                    ( BSONObject ) JSON.parse(
                            "{a:" + aUpBound + ",b:" + bUpBound + "}" ) );

            // 检查切分结果，比较范围内数据量
            checkResult( sdb, aLowBound, aUpBound );
        }
    }

    @Test(dependsOnMethods = "splitCLAndCheckResult")
    private void checkAllResult() {
        commSdb.setSessionAttr(
                ( BSONObject ) JSON.parse( "{PreferedInstance: 'M'}" ) );
        DBCollection cl = commSdb.getCollectionSpace( csName )
                .getCollection( clName );
        // coord上检查记录总数
        long count = cl.getCount( "{_id:{$isnull:0}}" );
        long expected = 30000;
        Assert.assertEquals( count, expected );

        // 目标组上检查边界值数据
        try ( Sequoiadb destDataNode = commSdb.getReplicaGroup( destGroupName )
                .getMaster().connect()) {
            DBCollection dbcl = destDataNode
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            // 目标组含有{a:10000,b:10000}边界值记录
            Assert.assertEquals( dbcl.getCount( "{a:10000,b:10000}" ), 1 );
            Assert.assertEquals( dbcl.getCount( "{a:20000,b:20000}" ), 1 );
            Assert.assertEquals( dbcl.getCount( "{a:29999,b:29999}" ), 1 );

            // 目标组不含切分范围外的数据
            long destDataCount1 = destDataNode
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName ).getCount(
                            "{$or:[{a:{$lt:0}},{a:{$gte:10000,$lt:20000},{a:{$gt:30000}}]}" );
            Assert.assertEquals( destDataCount1, 0 );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            commCS.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.close();
            }
        }
    }

    // 比对目标组数据量
    private void checkResult( Sequoiadb sdb, int alowBound, int aUpBound ) {
        DBCursor dbc = null;
        try ( Sequoiadb destDataNode = sdb.getReplicaGroup( destGroupName )
                .getMaster().connect()) {
            DBCollection dbcl = destDataNode
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );

            // 逐条比对记录
            dbc = dbcl.query(
                    "{a:{$gte:" + alowBound + ",$lt:" + aUpBound + "}}", null,
                    "{a:1}", null );
            int fieldValue = alowBound; // 记录key值累加器
            int count = 0;
            while ( dbc.hasNext() ) {
                BSONObject actual = dbc.getNext();
                BSONObject expect = ( BSONObject ) JSON
                        .parse( "{a:" + fieldValue + ",b:" + fieldValue
                                + ", test:" + "'testasetatatatt'}" );
                actual.removeField( "_id" );
                Assert.assertEquals( actual.equals( expect ), true,
                        actual.toString() + " " + expect.toString() );
                ++fieldValue;
                count++;
            }
            dbc.close();
            // 目标组含有本线程切分范围的数据
            int expRecords = 10000;
            Assert.assertEquals( count, expRecords );
        }
    }

    // insert 3W records
    private void prepareData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 3; i++ ) {
            List< BSONObject > list = new ArrayList<>();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON
                        .parse( "{a:" + value + ", b:" + value + ", test:"
                                + "'testasetatatatt'" + "}" );
                list.add( obj );
            }
            cl.insert( list );
        }
    }
}
