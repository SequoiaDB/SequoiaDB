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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SEQDB-10529 切分过程中修改CL :1、向cl中循环插入数据记录 2、执行split，设置范围切分条件
 *                       3、切分过程中执行修改CL操作，分别在如下阶段修改CL:
 *                       a、任务已下发还未开始执行（如执行split后，通过listTasks查看无任务，在此过程中修改cl）
 *                       b、迁移数据过程中（如直连目标组节点查看数据持续插入，可count查询数据量在增加，修改cl中副本数）
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组，
 *                       修改cl） 4、查看切分和修改cl操作结果
 *                       备注：此CALSS验证A,修改分区表Shardingtype，Partition会报-32，为合理报错，
 *                       问题单：1697
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10529A extends SdbTestBase {
    private String csName = "split_10529A";
    private String clName = "cl";
    private CollectionSpace cs;
    private DBCollection cl;
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private List< BSONObject > insertedData = new ArrayList< BSONObject >();

    @BeforeClass()
    public void setUp() {
        commSdb = new Sequoiadb( coordUrl, "", "" );

        // 跳过 standAlone 和数据组不足的环境
        if ( CommLib.isStandAlone( commSdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        List< String > groupsName = CommLib.getDataGroupNames( commSdb );
        if ( groupsName.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }

        srcGroupName = groupsName.get( 0 );
        destGroupName = groupsName.get( 1 );

        try {
            commSdb.dropCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -34 );
        }
        cs = commSdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                + srcGroupName + "'}" ) );
        // 写入待切分的记录（20000）
        insertData( cl );
    }

    @Test
    public void splitAndAlterCL() {
        Split splitThread = null;
        try {
            // 切分线程启动
            splitThread = new Split();
            splitThread.start();

            splitThread.join();

            // 修改CL
            try {
                cl.alterCollection(
                        ( BSONObject ) JSON.parse( "{ShardingKey:{nsk:1}}" ) );
                Assert.fail( "alter cl must be fail!" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -32 ) {
                    Assert.fail( "the alter cl with sharding must be fail! "
                            + e.getErrorCode() + e.getMessage() );
                }
            }

            Assert.assertEquals( splitThread.isSuccess(), true,
                    splitThread.getErrorMsg() );

        } catch ( Exception e ) {
            Assert.fail( "split and alter task failed: " + e );
        }
    }

    @AfterClass()
    public void tearDown() {
        try {
            commSdb.dropCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.close();
            }
        }
    }

    class Split extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace cs = sdb.getCollectionSpace( csName );
                DBCollection cl = cs.getCollection( clName );
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{sk:1000}" ),
                        ( BSONObject ) JSON.parse( "{sk:20000}" ) );
                // 期望目标组有19000条符合{sk:{$gte:1000,$lt:20000}}查询条件的数据,期望目标组共有19000条数据
                checkGroupData( sdb, 19000, "{sk:{$gte:1000,$lt:20000}}", 19000,
                        destGroupName );
                // 校验源组
                checkGroupData( sdb, 1000, "{sk:{$gte:0,$lt:1000}}", 1000,
                        srcGroupName );
            } catch ( Exception e ) {
                throw e;
            } finally {
                if ( sdb != null ) {
                    sdb.close();
                }
            }
        }
    }

    private void checkGroupData( Sequoiadb sdb, int expectedCount,
            String macher, int expectTotalCount, String groupName )
            throws Exception {
        Sequoiadb dataNode = null;
        DBCursor cusor = null;
        try {
            dataNode = sdb.getReplicaGroup( groupName ).getMaster().connect();// 获得目标组主节点链接
            CollectionSpace cs = dataNode.getCollectionSpace( csName );
            DBCollection cl = cs.getCollection( clName );
            cusor = cl.query();
            while ( cusor.hasNext() ) {
                BSONObject obj = cusor.getNext();
                if ( !insertedData.contains( obj ) ) {
                    throw new Exception(
                            "inserted data can not find this record:" + obj );
                }
                insertedData.remove( obj );
            }

            long count = cl.getCount( macher );
            if ( count != expectedCount ) {// 目标组应当含有上述查询数据
                throw new Exception(
                        groupName + " getCount(" + macher + "):expected "
                                + expectedCount + " but found " + count );
            }
            if ( cl.getCount() != expectTotalCount ) {// 目标组应当含有的数据量
                throw new Exception( groupName + " getCount:expected "
                        + expectTotalCount + " but found " + cl.getCount() );
            }
        } catch ( Exception e ) {
            throw e;
        } finally {
            if ( dataNode != null ) {
                dataNode.close();
            }
        }
    }

    // insert 2W records
    private void insertData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 2; i++ ) {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + value
                        + ", test:" + "'testasetatatatatat'" + "}" );
                list.add( obj );
                insertedData.add( obj );
            }
            cl.insert( list );
        }
    }

}
