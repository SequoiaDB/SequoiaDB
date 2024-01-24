package com.sequoiadb.cappedCL.restartnode;

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
import com.sequoiadb.cappedCL.CappedCLUtils;
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

/**
 * @FileName seqDB-11816: 数据操作时，主节点正常/异常重启
 * @Author liuxiaoxuan
 * @Date 2017-10-16
 */
public class CappedCLRestartNode11816 extends SdbTestBase {

    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private DBCollection cl = null;
    private String clName = "restartNode_cappedcl_11816";
    private String dataGroupName = null;
    private int insertNums = 100000;
    private final int strLength = 968;

    @BeforeClass
    public void setup() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120 ) ) {
            throw new SkipException( "checkBusiness failed" );
        }
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        dataGroupName = groupMgr.getAllDataGroupName().get( 0 );
        System.out.println( "group: " + dataGroupName );
        cl = sdb.getCollectionSpace( cappedCSName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{Capped:true,Size:1024,AutoIndexId:false,Group:'"
                                + dataGroupName + "'}" ) );
        CappedCLUtils.insertRecords( cl, insertNums, strLength );
    }

    @Test
    public void curdAndRestartNodeTest() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( dataGroupName );
        NodeWrapper priNode = dataGroup.getMaster();
        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( priNode, 0,
                5 );
        TaskMgr taskMgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < 5; i++ ) {
            taskMgr.addTask( new InsertTask() );
            taskMgr.addTask( new PopTask() );
        }
        taskMgr.execute();

        Assert.assertEquals( taskMgr.isAllSuccess(), true,
                taskMgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                "check LSN consistency fail" );

        // 环境恢复后，创建集合并检查主备一致
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

                // insert
                insertNums = 32768;
                CappedCLUtils.insertRecords( cl, insertNums, strLength );
            } catch ( BaseException e ) {
                e.printStackTrace();
                System.out.println( "kill master node while inserting: "
                        + e.getErrorCode() );
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

                // pop
                int skip = new Random().nextInt( 90000 );
                long logicalID = CappedCLUtils.getLogicalID( cl, skip );
                int direction = -1;
                if ( skip % 2 != 0 ) {
                    direction = 1;
                }
                CappedCLUtils.pop( cl, logicalID, direction );
            } catch ( BaseException e ) {
                e.printStackTrace();
                System.out.println(
                        "kill master node while poping: " + e.getErrorCode() );
            }
        }
    }
}
