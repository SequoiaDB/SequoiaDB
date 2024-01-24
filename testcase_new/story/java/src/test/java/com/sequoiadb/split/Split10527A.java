package com.sequoiadb.split;

import java.util.List;

import com.sequoiadb.exception.SDBError;
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
 *                       3、切分过程中执行删除cs操作，分别在如下阶段删除CS:
 *                       a、任务已下发还未开始执行（如执行split后，通过listTasks查看无任务，在此过程中删除cs）
 *                       b、迁移数据过程中（如直连目标组节点查看数据持续插入，可count查询数据量在增加）
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组）
 *                       4、查看切分和删除cs操作结果 备注：验证A场景
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10527A extends SdbTestBase {
    private String clName = "testcaseCL_10527A";
    private String customCSName = "testcaseCS_10527A";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb sdb = null;

    @BeforeClass()
    public void setUp() {

        try {
            sdb = new Sequoiadb( coordUrl, "", "" );

            // 跳过 standAlone 和数据组不足的环境
            CommLib commlib = new CommLib();
            if ( commlib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip StandAlone" );
            }
            List< String > groupsName = commlib.getDataGroupNames( sdb );
            if ( groupsName.size() < 2 ) {
                throw new SkipException(
                        "current environment less than tow groups " );
            }
            srcGroupName = groupsName.get( 0 );
            destGroupName = groupsName.get( 1 );

            CollectionSpace customCS = sdb
                    .createCollectionSpace( customCSName );
            DBCollection cl = customCS.createCollection( clName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                                    + srcGroupName + "'}" ) );
            insertData( cl );// 写入待切分的记录（10000）
        } catch ( BaseException e ) {
            if ( sdb != null ) {
                sdb.disconnect();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + SplitUtils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        Split splitThread = null;
        try {
            // 启动切分线程
            splitThread = new Split();
            splitThread.start();

            // 删除CS
            db = new Sequoiadb( coordUrl, "", "" );
            try {
                sdb.dropCollectionSpace( customCSName );
                Assert.assertFalse(
                        sdb.isCollectionSpaceExist( customCSName ) );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_LOCK_FAILED.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode() ) {
                    Assert.assertTrue(
                            sdb.isCollectionSpaceExist( customCSName ) );
                }
            }

            // 检测切分线程
            Assert.assertEquals( splitThread.isSuccess(), true,
                    splitThread.getErrorMsg() );
        } finally

        {
            if ( db != null ) {
                db.disconnect();
            }
            if ( splitThread != null ) {
                splitThread.join();
            }
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

    class Split extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( customCSName )
                        .getCollection( clName );
                if ( cl == null ) {// 若cl不存在，cl为空，未碰撞到测试点
                    return;
                }
                cl.split( srcGroupName, destGroupName, 90 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_CS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_TASK_HAS_CANCELED
                                .getErrorCode() ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    public void insertData( DBCollection cl ) {
        try {
            for ( int i = 0; i < 10000; i++ ) {
                BSONObject obj = ( BSONObject ) JSON.parse( "{sk:" + i + "}" );
                cl.insert( obj );
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }

}
