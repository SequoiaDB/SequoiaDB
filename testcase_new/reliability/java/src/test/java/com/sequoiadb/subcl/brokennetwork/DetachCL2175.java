package com.sequoiadb.subcl.brokennetwork;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.subcl.brokennetwork.commlib.DetachCLTask;
import com.sequoiadb.subcl.brokennetwork.commlib.Utils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * @FileName seqDB-2175: detachCL过程中catalog主节点断网
 * @Author linsuqiang
 * @Date 2017-03-15
 * @Version 1.00
 */

/*
 * 1、创建主表和子表 2、批量执行db.collectionspace.collection.detachCL()分离多个子表
 * 3、子表分离过程中将catalog主节点网络断掉（如：使用cutnet.sh工具，命令格式为nohup ./cutnet.sh
 * &），检查detachCL执行结果 4、将catalog主节点网络恢复，检查catalog主节点CL编目信息跟备节点编目信息是否完整一致
 * 5、在已经分离子表（普通表）做基本操作（如insert)，检查CL功能正确性
 */

public class DetachCL2175 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String mclName = "cl_2175";
    private boolean isPriChanged = true;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            Utils.createMclAndScl( db, mclName );
            Utils.attachAllScl( db, mclName );
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
            String cataPriHost = cataGroup.getMaster().hostName();
            String cataPriHostBefore = cataPriHost;

            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( cataPriHost, 1, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            String safeUrl = CommLib.getSafeCoordUrl( cataPriHost );
            DetachCLTask aTask = new DetachCLTask( mclName, safeUrl );
            mgr.addTask( aTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            Utils.checkConsistency( groupMgr );
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            Utils.checkIntegrated( db, mclName );
            Utils.checkDetached( db, mclName, aTask.getDetachedSclCnt() );

            String cataPriHostAfter = cataGroup.getMaster().hostName();
            if ( cataPriHostBefore.equals( cataPriHostAfter ) ) {
                isPriChanged = false;
            }
            runSuccess = true;
        } catch ( ReliabilityException e ) {
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
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            if ( isPriChanged ) {
                Utils.dropMclAndScl( db, mclName );
            } else {
                dropCLRepeatly( db );
            }
        } catch ( ReliabilityException | BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private void dropCLRepeatly( Sequoiadb db ) throws ReliabilityException {
        CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
        // drop all sub cl repeatly in 5min
        int timeout = 300000; // 5min
        int dropInterval = 15000; // 15s
        int dropTimes = timeout / dropInterval;
        for ( int i = 0; i < Utils.SCLNUM; ++i ) {
            String sclName = mclName + "_" + i;
            int j;
            for ( j = 0; j < dropTimes; ++j ) {
                try {
                    cs.dropCollection( sclName );
                    break;
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -147 ) {
                        e.printStackTrace();
                        throw new ReliabilityException( "fail to drop "
                                + sclName + " rc: " + e.getErrorCode() );
                    }
                }

                try {
                    Thread.sleep( dropInterval );
                } catch ( InterruptedException e ) {
                }
            }
            if ( j == dropTimes ) {
                throw new ReliabilityException(
                        "dropCLRepeatly occurs timeout" );
            }
        }
        // drop the main cl finally
        cs.dropCollection( mclName );
    }
}