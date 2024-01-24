package com.sequoiadb.rename.killnode;

import java.util.ArrayList;
import java.util.List;

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
 * @Description RenameCLKillMainNode16297.java seqDB-16297:执行rename
 *              cl过程中，数据主节点故障
 * @author luweikang
 * @date 2018年11月7日
 */
public class RenameCLKillMainNode16297 extends SdbTestBase {

    private String csName = "newCS_16297A";
    private List< String > oldCLNameList = new ArrayList<>();
    private List< String > newCLNameList = new ArrayList<>();
    private String oldCLName = "oldCL_16297A";
    private String newCLName = "newCL_16297A";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private int clNum = 20;
    private int completeTimes = 0;

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
        for ( int i = 0; i < clNum; i++ ) {
            cs.createCollection( oldCLName + i,
                    new BasicBSONObject( "Group", groupName ) );
            oldCLNameList.add( oldCLName + i );
            newCLNameList.add( newCLName + i );
        }
        sdb.sync();
    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper dataMaster = dataGroup.getMaster();

        // 建立并行任务
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                dataMaster.hostName(), dataMaster.svcName(), 0 );
        TaskMgr mgr = new TaskMgr( faultTask );
        Rename renameTask = new Rename();
        mgr.addTask( renameTask );
        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        for ( int i = 0; i < oldCLNameList.size(); i++ ) {
            if ( completeTimes < i + 1 ) {
                RenameUtils.retryRenameCL( csName, oldCLNameList.get( i ),
                        newCLNameList.get( i ) );
            }
            RenameUtils.checkRenameCLResult( sdb, csName,
                    oldCLNameList.get( i ), newCLNameList.get( i ) );
        }

        // 插入数据
        for ( int i = 0; i < newCLNameList.size(); i++ ) {
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( newCLNameList.get( i ) );
            RenameUtils.insertData( cl, 1000 );
            long actNum = cl.getCount();
            Assert.assertEquals( actNum, 1000, "check record num" );
        }

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
                for ( int i = 0; i < oldCLNameList.size(); i++ ) {
                    db.getCollectionSpace( csName ).renameCollection(
                            oldCLNameList.get( i ), newCLNameList.get( i ) );
                    completeTimes++;
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }
}
