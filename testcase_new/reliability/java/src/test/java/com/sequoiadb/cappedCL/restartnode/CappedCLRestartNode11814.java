package com.sequoiadb.cappedCL.restartnode;

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
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.cappedCL.CappedCLUtils;

/**
 * @FileName seqDB-11814: 删除集合时，主节点正常/异常重启
 * @Author liuxiaoxuan
 * @Date 2017-07-31
 */

public class CappedCLRestartNode11814 extends SdbTestBase {

    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private String csName = "story_cappedCS_restartNode_11814";
    private String clName = "cappedCL_restartNode_11814";
    private String groupName = null;
    private int dropCLCounts = 0;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupName = groupMgr.getAllDataGroupName().get( 0 );

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.createCollectionSpace( csName,
                ( BSONObject ) JSON.parse( "{Capped:true}" ) );
        for ( int clNum = 1; clNum <= 1000; clNum++ ) {
            sdb.getCollectionSpace( csName ).createCollection(
                    clName + "_" + clNum,
                    ( BSONObject ) JSON.parse(
                            "{Capped:true,Size:1024,AutoIndexId:false,Group:'"
                                    + groupName + "'}" ) );
        }
    }

    @Test
    public void dropCLAndRestartNodeTest() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper primaryNode = dataGroup.getMaster();

        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( primaryNode,
                1, 10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        mgr.addTask( new DropCappedCLTask() );
        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "check LSN consistency fail" );

        // 环境恢复后，插入/pop并检查主备一致
        DBCollection cl = sdb.getCollectionSpace( csName ).createCollection(
                clName + "_after_startnode",
                ( BSONObject ) JSON.parse(
                        "{Capped:true,Size:1024,AutoIndexId:false,Group:'"
                                + groupName + "'}" ) );
        CappedCLUtils.insertRecords( cl, 1000, 8 );
        CappedCLUtils.pop( cl, CappedCLUtils.getLogicalID( cl, 100 ), 1 );
        Assert.assertEquals( dataGroup.checkInspect( 120 ), true,
                "data is different on " + dataGroup.getGroupName() );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DropCappedCLTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                for ( int clNum = 1; clNum <= 1000; clNum++ ) {
                    cs.dropCollection( clName + "_" + clNum );
                    ++dropCLCounts;
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
                System.out.println( "the drop CL num is = " + dropCLCounts );
            }
        }
    }
}
