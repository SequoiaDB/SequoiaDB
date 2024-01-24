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
 * @FileName seqDB-15790: 插入记录扩数据文件，执行pop操作，再次插入记录的同时主节点异常重启
 * @Author liuxiaoxuan
 * @Date 2019-07-23
 */

public class CappedCLKillNode15790B extends SdbTestBase {

    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private String clName = "cappedCL_killNode_15790B";
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
        // 插入数据扩文件
        int insertNums = 200000;
        int strLength = 512;
        CappedCLUtils.insertRecords( cl, insertNums, strLength );

        // 正向pop
        long logicalID = CappedCLUtils.getLogicalID( cl,
                new Random().nextInt( 100000 ) );
        int direction = 1;
        CappedCLUtils.pop( cl, logicalID, direction );

        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper masterNode = dataGroup.getMaster();

        FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask(
                masterNode.hostName(), masterNode.svcName(), 1 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < 10; i++ ) {
            mgr.addTask( new InsertTask() );
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

    private class InsertTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( cappedCSName );
                DBCollection cl = cs.getCollection( clName );
                int insertNums = 10000;
                int strLength = 32;
                CappedCLUtils.insertRecords( cl, insertNums, strLength );
            } catch ( BaseException e ) {
                e.printStackTrace();
                System.out.println( "kill master node while inserting: "
                        + e.getErrorCode() );
            }
        }
    }
}
