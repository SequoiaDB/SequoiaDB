package com.sequoiadb.split;

import java.util.ArrayList;
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
 * @FileName:SEQDB-10528 切分过程中删除CL :1、向cl中循环插入数据记录 2、执行split，设置范围切分条件
 *                       3、切分过程中执行删除CL操作，分别在如下阶段删除CL:
 *                       a、任务已下发还未开始执行（如执行split后，通过listTasks查看无任务，在此过程中删除cl）
 *                       b、迁移数据过程中（如直连目标组节点查看数据持续插入，可count查询数据量在增加，删除cl）
 *                       c、目标组更新编目信息后删除cs（如直连目标组查看数据已迁移完成，或者直连编目节点查看cl信息中存在目标组，
 *                       删除cl） 4、查看切分和删除cl操作结果 备注此用例验证A
 * @author huangqiaohui
 * @version 1.00
 *
 */

public class Split10528A extends SdbTestBase {
    private String clName = "testcaseCL_10528A";
    private String srcGroupName;
    private String destGroupName;
    private Sequoiadb sdb = null;

    @BeforeClass()
    public void setUp() {
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

        CollectionSpace customCS = sdb.getCollectionSpace( SdbTestBase.csName );
        DBCollection cl = customCS.createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ShardingKey:{'sk':1},Partition:4096,ShardingType:'hash',Group:'"
                                + srcGroupName + "'}" ) );

        // 写入待切分的记录（30000）
        insertData( cl );
    }

    @Test
    public void test() {
        // 切分线程启动
        Split splitThread = new Split();
        splitThread.start();

        // 删除CL
        DropCL dropcl = new DropCL();
        dropcl.start();

        // 检测任务执行结果
        Assert.assertEquals( splitThread.isSuccess(), true,
                splitThread.getErrorMsg() );
        Assert.assertEquals( dropcl.isSuccess(), true, dropcl.getErrorMsg() );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.getCollectionSpace( SdbTestBase.csName )
                    .isCollectionExist( clName ) ) {
                sdb.getCollectionSpace( SdbTestBase.csName )
                        .dropCollection( clName );
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
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                if ( cl == null ) {// 若cl已被删除，此时cl为空，
                    return;
                }
                cl.split( srcGroupName, destGroupName, 90 );
                // 如果删除成功，本次测试未碰撞到测试点
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_DPS_TRANS_LOCK_INCOMPATIBLE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_TASK_HAS_CANCELED
                                .getErrorCode() ) {
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

    class DropCL extends SdbThreadBase {
        @SuppressWarnings("resource")
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                // 删除CL
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( clName );
                Assert.assertEquals( db.getCollectionSpace( SdbTestBase.csName )
                        .isCollectionExist( clName ), false );
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
        for ( int i = 0; i < 3; i++ ) {
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
