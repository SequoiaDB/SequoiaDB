package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

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
 * @FileName:SEQDB-10527 切分过程中删除CS :1、向cl中循环插入数据记录 2、执行split，设置范围切分条件
 *                       3、切分过程中执行删除cs操作，分别在如下阶段删除CS: *
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组）
 *                       通过多个切分任务，随机sleep时间删除cs来测试（参考评审意见方法） 4、查看切分和删除cs操作结果
 *                       备注：验证C场景
 * @author huangqiaohui
 * @update [wuyan 2018/01/03 增加insert记录数,多个任务并发切分]
 * @version 1.00
 *
 */

public class Split10527C extends SdbTestBase {
    private String clName = "testcaseCL_10527C";
    private String customCSName = "testcaseCS_10527C";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb sdb = null;

    @BeforeClass()
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );

        // 跳过 standAlone 和数据组不足的环境
        CommLib commlib = new CommLib();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        List< String > groupsName = commlib.getDataGroupNames( sdb );
        if ( groupsName.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }
        srcGroupName = groupsName.get( 0 );
        destGroupName = groupsName.get( 1 );

        DBCollection dbCollection = createCSAndCL();
        insertData( dbCollection );// 写入待切分的记录（200000）
    }

    @Test(timeOut = 30 * 60 * 1000)
    public void test() throws InterruptedException {
        int condition = 0;
        int endCondition = 1024;
        List< SplitTask > splitTasks = new ArrayList<>( 5 );
        Random random = new Random();
        for ( int i = 0; i < 5; i++ ) {
            splitTasks.add( new SplitTask( condition, endCondition ) );
            condition = endCondition + 1024;
            endCondition = condition + 2048;
        }

        for ( SplitTask splitTask : splitTasks ) {
            splitTask.start();
        }

        Thread.sleep( random.nextInt( 1000 ) );
        try {
            sdb.dropCollectionSpace( customCSName );
            Assert.assertFalse( sdb.isCollectionSpaceExist( customCSName ) );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                Assert.assertTrue( sdb.isCollectionSpaceExist( customCSName ) );
            }
        }

        // 检测切分线程
        for ( SplitTask splitTask : splitTasks ) {
            Assert.assertTrue( splitTask.isSuccess(), splitTask.getErrorMsg() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( customCSName ) ) {
                sdb.dropCollectionSpace( customCSName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    class SplitTask extends SdbThreadBase {
        private int beginNo, endNo;

        public SplitTask( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( customCSName )
                        .getCollection( clName );
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{sk:" + beginNo + "}" ),
                        ( BSONObject ) JSON.parse( "{sk:" + endNo + "}" ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -34 && e.getErrorCode() != -23
                        && e.getErrorCode() != -147 && e.getErrorCode() != -190
                        && e.getErrorCode() != -243 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }

    }

    private DBCollection createCSAndCL() {
        if ( sdb.isCollectionSpaceExist( customCSName ) ) {
            sdb.dropCollectionSpace( customCSName );
        }
        CollectionSpace customCS = sdb.createCollectionSpace( customCSName );
        DBCollection cl = customCS.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'sk':1},Partition:16384,ShardingType:'hash',Group:'"
                                + srcGroupName + "'}" ) );
        return cl;
    }

    // insert 5W records
    private void insertData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 20; i++ ) {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + value
                        + ", test:" + "'testasetatatatatat'" + "}" );
                list.add( obj );
            }
            cl.insert( list );
        }
    }

}
