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
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.rename.RenameUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description MainNodeErrRename.java: seqDB-16296:rename cs未同步到备节点时主节点异常
 * @author luweikang
 * @date 2018年11月7日
 */
public class RenameCSKillMainNode16296 extends SdbTestBase {

    private String oldCSName = "oldcs_16296";
    private String newCSName = "newcs_16296";
    private String clName = "cl_16296";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() throws ReliabilityException {

        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }
        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.createCollectionSpace( oldCSName );
        cl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper dataMaster = dataGroup.getMaster();

        // stop slave node
        NodeWrapper slave = dataGroup.getSlave();
        slave.stop();

        // 建立并行任务
        TaskMgr task = new TaskMgr();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                dataMaster.hostName(), dataMaster.svcName(), 0 );
        task.addTask( faultTask );

        Rename renameTask = new Rename();
        renameTask.start();
        renameTask.join();
        if ( renameTask.isSuccess() ) {
            task.execute();
        } else {
            Assert.fail( renameTask.getErrorMsg() );
        }

        Assert.assertTrue( task.isAllSuccess(), task.getErrorMsg() );
        slave.start();
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        RenameUtils.retryRenameCS( oldCSName, newCSName );
        RenameUtils.checkRenameCSResult( sdb, oldCSName, newCSName, 1 );

        // 插入数据
        cl = sdb.getCollectionSpace( newCSName ).getCollection( clName );
        RenameUtils.insertData( cl, 1000 );
        long actNum = cl.getCount();
        Assert.assertEquals( actNum, 1000, "check record num" );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( newCSName );
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
                db.renameCollectionSpace( oldCSName, newCSName );
            }
        }
    }
}
