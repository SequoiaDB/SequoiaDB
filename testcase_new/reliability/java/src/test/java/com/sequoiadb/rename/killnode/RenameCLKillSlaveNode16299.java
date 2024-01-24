package com.sequoiadb.rename.killnode;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.rename.RenameUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description RenameKillSlaveNode16299.java seqDB-16299:数据备节点已故障，执行rename cl
 * @author luweikang
 * @date 2018年11月7日
 */
public class RenameCLKillSlaveNode16299 extends SdbTestBase {

    private String csName = "cs_16299A";
    private String oldCLName = "oldCL_16299A";
    private String newCLName = "newCL_16299A";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }
        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cs.createCollection( oldCLName,
                new BasicBSONObject( "Group", groupName ) );
    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper dataSlave = dataGroup.getSlave();

        // 建立并行任务
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                dataSlave.hostName(), dataSlave.svcName(), 0 );
        TaskMgr mgr = new TaskMgr( faultTask );

        Rename renameTask = new Rename();
        mgr.addTask( renameTask );
        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        RenameUtils.retryRenameCL( csName, oldCLName, newCLName );
        RenameUtils.checkRenameCLResult( sdb, csName, oldCLName, newCLName );

        // 插入数据
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( newCLName );
        RenameUtils.insertData( cl, 1000 );
        long actNum = cl.getCount();
        Assert.assertEquals( actNum, 1000, "check record num" );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    class Rename extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.getCollectionSpace( csName ).renameCollection( oldCLName,
                        newCLName );
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }
}
