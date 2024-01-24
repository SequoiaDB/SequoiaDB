package com.sequoiadb.metaopr.diskfull;

import java.util.Date;

import com.sequoiadb.metaopr.Utils;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-2409: 删除CS时catalog主节点所在服务器磁盘满
 * @Author linsuqiang
 * @Date 2017-03-31
 * @Version 1.00
 */

/*
 * 1、创建CS，构造脚本循环执行创建CS操作db.createCS（） 2、执行删除CS操作（构造脚本循环执行删除CS操作）
 * 3、删除CS时catalog主节点所在主机磁盘满（构造主机磁盘满故障） 3、查看CS信息和catalog主节点状态 4、恢复故障（清理磁盘空间）
 * 5、再次执行删除CS操作 6、查看CS信息（执行listCollections（）命令查看CS信息） 7、查看catalog主备节点是否存在该CS相关信息
 */

public class DropCS2409 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String csNameBase = "cs_2409";
    private static final int CS_NUM = 100;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            for ( int i = 0; i < CS_NUM; i++ ) {
                String csName = csNameBase + "_" + i;
                db.createCollectionSpace( csName );
            }
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            NodeWrapper priNode = cataGroup.getMaster();
            Sequoiadb cataDB = priNode.connect();
            DBCollection sysCataCL = cataDB.getCollectionSpace( "SYSCAT" )
                    .getCollection( "SYSCOLLECTIONS" );

            FaultMakeTask faultTask = DiskFull.getFaultMakeTask(
                    priNode.hostName(), SdbTestBase.reservedDir, 0, 10,
                    sysCataCL );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new DropCSTask() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dropCSAgain( db );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }
            MyUtil.checkListCS( db, csNameBase, 0 );
            Utils.checkConsistency( groupMgr );
            runSuccess = true;
        } catch ( ReliabilityException | InterruptedException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( !runSuccess ) {
            throw new SkipException( "to save environment" );
        }
        Sequoiadb db = null;
        try {
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private class DropCSTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                for ( int i = 0; i < CS_NUM; i++ ) {
                    String csName = csNameBase + "_" + i;
                    db.dropCollectionSpace( csName );
                }
            } catch ( BaseException e ) {
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void dropCSAgain( Sequoiadb db ) {
        for ( int i = 0; i < CS_NUM; i++ ) {
            try {
                String csName = csNameBase + "_" + i;
                db.dropCollectionSpace( csName );
            } catch ( BaseException e ) {
                // -34 SDB_DMS_CS_NOTEXIST 集合空间不存在
                if ( e.getErrorCode() != -34 ) {
                    throw e;
                }
            }
        }
    }

}