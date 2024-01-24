package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicBoolean;

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
 * @FileName:SEQDB-10528 切分过程中删除CL :1、向cl中循环插入数据记录 2、执行split，设置范围切分条件
 *                       3、切分过程中执行删除CL操作，在如下阶段删除CL: *
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组，
 *                       删除cl） 4、查看切分和删除cl操作结果
 *                       备注：通过并发多个切分任务，同时随机等待时间dropCL测试（参考评审意见，这种方式可以随机并发测试该流程）
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10528C extends SdbTestBase {
    private String clName = "testcaseCL_10528C";
    private CollectionSpace cs;
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb commSdb = null;
    private AtomicBoolean flag = new AtomicBoolean( false );

    @BeforeClass()
    public void setUp() {
        commSdb = new Sequoiadb( coordUrl, "", "" );
        // 跳过 standAlone 和数据组不足的环境
        CommLib commlib = new CommLib();
        if ( commlib.isStandAlone( commSdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
        List< String > groupsName = commlib.getDataGroupNames( commSdb );
        if ( groupsName.size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups " );
        }
        srcGroupName = groupsName.get( 0 );
        destGroupName = groupsName.get( 1 );
        cs = commSdb.getCollectionSpace( SdbTestBase.csName );
        DBCollection cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'sk':1},ShardingType:'range',Group:'"
                                + srcGroupName + "'}" ) );
        // 写入待切分的记录（200000）
        insertData( cl );
    }

    @Test(timeOut = 30 * 60 * 1000)
    public void test() throws InterruptedException {
        int condition = 0;
        int endCondition = 5000;
        List< SplitTask > splitTasks = new ArrayList<>( 5 );
        Random random = new Random();
        for ( int i = 0; i < 5; i++ ) {
            splitTasks.add( new SplitTask( condition, endCondition ) );
            condition = endCondition + 5000;
            endCondition = condition + 25000;
        }

        for ( SplitTask splitTask : splitTasks ) {
            splitTask.start();
        }

        // 删除CL,随机覆盖：1、数据迁移完成，编目未更新；2、数据迁移完成，编目已更新
        Thread.sleep( random.nextInt( 2000 ) );

        cs.dropCollection( clName );
        Assert.assertEquals( cs.isCollectionExist( clName ), false );

        // 检测切分线程
        for ( SplitTask splitTask : splitTasks ) {
            Assert.assertTrue( splitTask.isSuccess(), splitTask.getErrorMsg() );
        }

    }

    @AfterClass(enabled = true)
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + "\r\n"
                    + SplitUtils.getKeyStack( e, this ) );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
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
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.split( srcGroupName, destGroupName,
                        ( BSONObject ) JSON.parse( "{sk:" + beginNo + "}" ),
                        ( BSONObject ) JSON.parse( "{sk:" + endNo + "}" ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -147
                        && e.getErrorCode() != -190
                        && e.getErrorCode() != -243 ) {
                    e.printStackTrace();
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }

    }

    // insert 3W records
    private void insertData( DBCollection cl ) {
        int count = 0;
        for ( int i = 0; i < 20; i++ ) {
            List< BSONObject > list = new ArrayList< BSONObject >();
            for ( int j = 0; j < 10000; j++ ) {
                int value = count++;
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + value
                        + ", test:"
                        + "'testasetaASDGAA111111111DGADGADGADGtatatatat'"
                        + "}" );
                list.add( obj );
            }
            cl.insert( list );
        }
    }

}
