package com.sequoiadb.datasync.killnode;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.*;

/**
 * @FileName seqDB-3213: 删除CL过程中主节点节点异常重启，该主节点为同步的源节点 seqDB-9335:
 *           删除cs/cl过程中主节点异常重启，选出新主节点，该故障节点为同步的目的节点
 * @Author linsuqiang
 * @Date 2017-03-27
 * @Version 1.00
 */

/*
 * 1.批量删除CL 2.过程中构造节点异常重启(kill -9) 3.选主成功后，继续删除部分CL 4.过程中故障恢复 ，检查CL信息是否一致
 * 注：ReplSize = 2,断主节点时，该节点有可能是同步的源节点，也有可能不是。若是，覆盖3213，若不是，覆盖9335
 */

public class DropCL3213 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clNameBase = "cl_3213";
    private String clGroupName = null;
    private static final int CL_NUM = 500;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            createCLs( db );
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
            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            NodeWrapper priNode = dataGroup.getMaster();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    priNode.hostName(), priNode.svcName(), 1 );
            TaskMgr mgr = new TaskMgr( faultTask );
            DropCLTask dTask = new DropCLTask();
            mgr.addTask( dTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            Utils.checkConsistencyCL(dataGroup, csName, clNameBase);
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
            dropRemainCLs( db );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private void createCLs( Sequoiadb db ) {
        CollectionSpace commCS = db.getCollectionSpace( csName );
        for ( int i = 0; i < CL_NUM; i++ ) {
            String clName = clNameBase + "_" + i;
            BSONObject option = ( BSONObject ) JSON
                    .parse( "{ Group: '" + clGroupName + "', ReplSize: 2 }" );
            commCS.createCollection( clName, option );
        }
    }

    private class DropCLTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                CollectionSpace commCS = db.getCollectionSpace( csName );
                for ( int i = 0; i < CL_NUM; i++ ) {
                    String clName = clNameBase + "_" + i;
                    commCS.dropCollection( clName );
                }
            } catch ( BaseException e ) {
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void dropRemainCLs( Sequoiadb db ) {
        CollectionSpace commCS = db.getCollectionSpace( csName );
        for ( int i = 0; i < CL_NUM; i++ ) {
            String clName = clNameBase + "_" + i;
            try {
                commCS.dropCollection( clName );
            } catch ( BaseException e ) {
                // -23 SDB_DMS_NOTEXIST 集合不存在
                if ( !( e.getErrorCode() == -23 ) ) {
                    throw e;
                }
            }
        }
    }
}