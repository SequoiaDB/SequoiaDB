package com.sequoiadb.cappedCL.killnode;

import org.bson.BSONObject;
import org.bson.util.JSON;
import java.util.Random;
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
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.cappedCL.CappedCLUtils;

/**
 * @FileName seqDB-15794: 正向pop，pop操作同时数据组所有节点异常重启
 * @Author liuxiaoxuan
 * @Date 2019-07-23
 */

public class CappedCLKillNode15794B extends SdbTestBase {

    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private String clName = "cappedCL_killNode_15794B";
    private String groupName = null;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupName = groupMgr.getAllDataGroupName().get( 0 );
        System.out.println( "group: " + groupName );
        cl = sdb.getCollectionSpace( cappedCSName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{Capped:true,Size:1024,AutoIndexId:false,Group:'"
                                + groupName + "'}" ) );
    }

    @Test
    public void createCLAndKillNodeTest() throws ReliabilityException {
        // 插入大量数据
        int insertNums = 300000;
        int strLength = 1024;
        CappedCLUtils.insertRecords( cl, insertNums, strLength );

        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        TaskMgr mgr = new TaskMgr();
        for ( NodeWrapper node : dataGroup.getNodes() ) {
            FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask( node, 0 );
            mgr.addTask( faultMakeTask );
        }
        for ( int i = 0; i < 10; i++ ) {
            mgr.addTask( new PopTask() );
        }
        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 1200 ), true,
                "check LSN consistency fail" );

        // 环境恢复后，执行insert/pop并检查主备一致
        CappedCLUtils.insertRecords( cl, 10000, 8 );
        CappedCLUtils.pop( cl, CappedCLUtils.getLogicalID( cl, 100 ), 1 );
        Assert.assertEquals( dataGroup.checkInspect( 120 ), true,
                "data is different on " + dataGroup.getGroupName() );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.getCollectionSpace( cappedCSName ).dropCollection( clName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class PopTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( cappedCSName );
                DBCollection cl = cs.getCollection( clName );
                // 正向pop
                long logicalID = CappedCLUtils.getLogicalID( cl,
                        new Random().nextInt( 299999 ) );
                int direction = 1;
                CappedCLUtils.pop( cl, logicalID, direction );
            } catch ( BaseException e ) {
                System.out.println(
                        "kill all nodes while poping: " + e.getErrorCode() );
            }
        }
    }
}
