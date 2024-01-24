package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @FileName:SEQDB-509 切分范围有交集，并发切分1.在CS下创建cl，指定分区方式为range
 *                     2、向cl中插入大量数据，如插入1百万条记录
 *                     3、并发执行多个split，其中切分范围存在交集部分，如一个切分范围为(10,30]，另一个切分范围为(20,
 *                     100] 4、查看数据切分是否正确
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split509 extends SdbTestBase {
    @DataProvider(name = "rangeProvider", parallel = true)
    public Object[][] rangeProvider() {
        return new Object[][] { { 0, 15000 }, { 10000, 20000 } };
    }

    private String clName = "testcaseCL509";
    private CollectionSpace commCS;
    private String srcGroupName;
    private String destGroupName;
    private Vector< Integer > successRange = new Vector< >(); // 保存成功切分的上下限范围
    private Sequoiadb commSdb = null;

    @BeforeClass(enabled = true)
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
        DBCollection cl = commCS.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"a\":1},ShardingType:\"range\"}" ) );
        ArrayList< String > tmp = SplitUtils.getGroupName( commSdb, csName,
                clName );
        srcGroupName = tmp.get( 0 );
        destGroupName = tmp.get( 1 );
        // 写入待切分的记录（20000）
        prepareData( cl );

    }

    // 切分(a:0,a:15000] (a:10000,a:20000]
    @Test(enabled = true, dataProvider = "rangeProvider")
    public void splitCL( int lowBound, int upBound ) {
        try ( Sequoiadb sdb = new Sequoiadb( coordUrl, "", "" )) {
            DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );

            // 切分(a:0,a:15000) (a:10000,a:20000)
            cl.split( srcGroupName, destGroupName,
                    ( BSONObject ) JSON.parse( "{a:" + lowBound + "}" ),
                    ( BSONObject ) JSON.parse( "{a:" + upBound + "}" ) );
            synchronized ( this ) {
                // 两个切分线程不能同时成功
                if ( successRange.size() != 0 ) {
                    Assert.fail(
                            "parallel execute split range(0,100) and range(50,150),successRange.size() = "
                                    + successRange.size() );
                }
                // 本线程切分成功，记录切分范围
                successRange.add( lowBound );
                successRange.add( upBound );
            }

            // 检查切分后目标组数据，再次插入数据，检测落入情况
            checkResult( sdb );
        } catch ( BaseException e ) {
            Assert.assertEquals(
                    e.getErrorCode() == -175 || e.getErrorCode() == -176, true,
                    e.getMessage() + "\r\n"
                            + SplitUtils.getKeyStack( e, this ) );
            return;
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

    // 检查目标组的数据，插入数据，检查落入情况
    public void checkResult( Sequoiadb sdb ) {
        Sequoiadb destDataNode = null;
        Sequoiadb srcDataNode = null;
        try {
            destDataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                    .connect();// 获得目标组主节点链接
            srcDataNode = sdb.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();// 获得源组主节点链接
            // 检查切分后目标组数据正确性
            checkDestGroupData( sdb, destDataNode );
            // 插入数据并检查落入情况
            insertDataAndCheckAgain( sdb, srcDataNode, destDataNode );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( destDataNode != null ) {
                destDataNode.close();
            }
            if ( srcDataNode != null ) {
                srcDataNode.close();
            }
        }
    }

    // insert 2W records
    private void prepareData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 2; i++ ) {
            List< BSONObject > list = new ArrayList< >();
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

    public void insertDataAndCheckAgain( Sequoiadb sdb, Sequoiadb srcDataNode,
            Sequoiadb destDataNode ) {
        // 检查切分结果
        try {
            // 插入数据，检查落入情况
            DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            cl.insert( ( BSONObject ) JSON
                    .parse( "{b:1,a:" + successRange.get( 0 ) + "}" ) );// 期望此数据落入目标组
            cl.insert( ( BSONObject ) JSON.parse(
                    "{b:-1,a:" + ( successRange.get( 0 ) - 1 ) + "}" ) );// 期望此数据落入源数据组

            // 目标组落入情况
            DBCollection destGroupCL = destDataNode
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( destGroupCL,
                    "{b:1,a:" + successRange.get( 0 ) + "}" ) ) {
                Assert.fail( "check query data not pass(b:1)" );
            }

            // 源组落入情况
            DBCollection srcGroupCL = srcDataNode.getCollectionSpace( csName )
                    .getCollection( clName );
            if ( !SplitUtils.isCollectionContainThisJSON( srcGroupCL,
                    "{b:-1,a:" + ( successRange.get( 0 ) - 1 ) + "}" ) ) {
                Assert.fail( "check query data not pass(b:-1)" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }

    }

    public void checkDestGroupData( Sequoiadb sdb, Sequoiadb destDataNode ) {
        try {
            long destDataCount = destDataNode.getCollectionSpace( csName )
                    .getCollection( clName )
                    .getCount( "{$and:[{a:{$gte:" + successRange.get( 0 )
                            + "}},{a:{$lt:" + successRange.get( 1 ) + "}}]}" );
            // 检查指定范围数据是否均已落入目标组
            Assert.assertEquals( destDataCount,
                    successRange.get( 1 ) - successRange.get( 0 ) );
            // 检查目标组是否仅含指定范围数据
            Assert.assertEquals(
                    destDataNode.getCollectionSpace( csName )
                            .getCollection( clName ).getCount(),
                    successRange.get( 1 ) - successRange.get( 0 ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        }

    }
}
