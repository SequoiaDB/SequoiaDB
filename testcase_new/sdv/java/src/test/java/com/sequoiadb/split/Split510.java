package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-510 切分范围不冲突，并发切分1.在CS下创建cl，指定分区方式为range
 *                     2、向cl中插入大量数据，如插入1百万条记录
 *                     3、并发执行多个split，其中切分范围不相同，如一个切分范围为(0,10]，另一个切分范围为(80,100]
 *                     4、查看数据切分是否正确
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split510 extends SdbTestBase {
    private String clName = "testcaseCL510";
    private CollectionSpace commCS;
    private DBCollection cl;
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;

    @BeforeClass
    public void setUp() {
        commSdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( commSdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        if ( CommLib.getDataGroupNames( commSdb ).size() < 2 ) {
            throw new SkipException(
                    "Ncurrent environment less than tow groups " );
        }

        commSdb.setSessionAttr(
                ( BSONObject ) JSON.parse( "{PreferedInstance: 'M'}" ) );
        commCS = commSdb.getCollectionSpace( csName );
        cl = commCS.createCollection( clName, ( BSONObject ) JSON
                .parse( "{ShardingKey:{\"a\":1},ShardingType:\"range\"}" ) );
        ArrayList< String > tmp = SplitUtils.getGroupName( commSdb, csName,
                clName );
        srcGroupName = tmp.get( 0 );
        destGroupName = tmp.get( 1 );

        // 写入待切分的记录（40000）
        prepareData( cl );
    }

    @Test
    private void testSplit() {
        // 3个切分任务：(a:0,a:22000) (a:25000,a:45000) (a:50000,a:70000)
        int lowBound1 = 0;
        int upBound1 = 22000;
        int lowBound2 = 25000;
        int upBound2 = 45000;
        int lowBound3 = 50000;
        int upBound3 = 70000;
        SplitCL splitCL1 = new SplitCL( lowBound1, upBound1 );
        SplitCL splitCL2 = new SplitCL( lowBound2, upBound2 );
        SplitCL splitCL3 = new SplitCL( lowBound3, upBound3 );
        splitCL1.start();
        splitCL2.start();
        splitCL3.start();

        Assert.assertTrue( splitCL1.isSuccess(), splitCL1.getErrorMsg() );
        Assert.assertTrue( splitCL2.isSuccess(), splitCL2.getErrorMsg() );
        Assert.assertTrue( splitCL3.isSuccess(), splitCL3.getErrorMsg() );

        // check the result
        checkAllResult();
    }

    private class SplitCL extends SdbThreadBase {
        private int lowBound;
        private int upBound;

        public SplitCL( int lowBound, int upBound ) {
            this.lowBound = lowBound;
            this.upBound = upBound;
        }

        @Override
        @SuppressWarnings({ "resource", "deprecation" })
        public void exec() throws BaseException {
            Sequoiadb sdb = null;
            Sequoiadb dataNode = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{a:" + lowBound + "}" ),
                        ( BSONObject ) JSON.parse( "{a:" + upBound + "}" ) );
                // 直连目标组主节点，检测目标组切分后数据
                dataNode = sdb.getReplicaGroup( destGroupName ).getMaster()
                        .connect();
                DBCollection destGroupCl = dataNode.getCollectionSpace( csName )
                        .getCollection( clName );
                long count = destGroupCl.getCount(
                        "{a:{$gte:" + lowBound + ",$lt:" + upBound + "}}" );

                // 目标组应当含有上述范围数据
                Assert.assertEquals( count, upBound - lowBound );
            } finally {
                if ( sdb != null )
                    sdb.disconnect();
                if ( dataNode != null )
                    dataNode.disconnect();
            }
        }
    }

    private void checkAllResult() {
        // 直连源组主节点，检测源组切分后数据范围
        try ( Sequoiadb srcDataNode = commSdb.getReplicaGroup( srcGroupName )
                .getMaster().connect()) {
            DBCollection srcGroupCl = srcDataNode
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            int lowBound1 = 22000;
            int upBound1 = 25000;
            int lowBound2 = 45000;
            int upBound2 = 50000;
            long count1 = srcGroupCl.getCount(
                    "{a:{$gte:" + lowBound1 + ",$lt:" + upBound1 + "}}" );
            long count2 = srcGroupCl.getCount(
                    "{a:{$gte:" + lowBound2 + ",$lt:" + upBound2 + "}}" );
            long expSrcNums = 8000;
            long actCount = count1 + count2;
            Assert.assertEquals( actCount, expSrcNums );
        }

        // coord上检查记录总数
        long count = cl.getCount( "{_id:{$isnull:0}}" );
        long expected = 70000;
        Assert.assertEquals( count, expected );
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            commCS.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    // insert 7W records
    private void prepareData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 7; i++ ) {
            List< BSONObject > list = new ArrayList<>();
            for ( int j = i + 0; j < i + 10000; j++ ) {
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
