package com.sequoiadb.cappedCL.diskfull;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.cappedCL.CappedCLUtils;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
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

import java.util.Random;

/**
 * @FileName seqDB-11819 数据操作时，主节点磁盘空间满
 * @author xiejianhong
 * @Date 2017-08-16
 * @version 1.0
 */
public class CappedCLMaster11819 extends SdbTestBase {

    private GroupMgr groupMgr = null;
    private GroupWrapper dataGroup = null;
    private Sequoiadb db = null;
    private DBCollection cl = null;
    private final String CLNAME = "cl_11819_master";

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120 ) ) {
            throw new SkipException( "checkBusiness failed" );
        }

        db = new Sequoiadb( coordUrl, "", "" );
        dataGroup = groupMgr.getAllDataGroup().get( 0 );

        cl = db.getCollectionSpace( cappedCSName ).createCollection( CLNAME,
                ( BSONObject ) JSON.parse(
                        "{Capped:true,Size:1024,AutoIndexId:false,Group:'"
                                + dataGroup.getGroupName() + "'}" ) );
        int insertNums = 300000;
        int strLength = 128;
        CappedCLUtils.insertRecords( cl, insertNums, strLength );
    }

    @Test
    public void test() throws ReliabilityException {
        NodeWrapper masterNode = dataGroup.getMaster();
        FaultMakeTask faultTask = DiskFull.getFaultMakeTask(
                masterNode.hostName(), SdbTestBase.reservedDir, 1, 10 );
        TaskMgr taskMgr = new TaskMgr( faultTask );
        for ( int i = 0; i < 5; i++ ) {
            taskMgr.addTask( new InsertTask() );
        }
        taskMgr.addTask( new PopTask() );
        taskMgr.execute();
        // 检查环境
        Assert.assertEquals( taskMgr.isAllSuccess(), true,
                taskMgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );

        // 环境恢复后，执行insert/pop并检查主备一致
        CappedCLUtils.insertRecords( cl, 10000, 16 );
        long logicalID = CappedCLUtils.getLogicalID( cl,
                new Random().nextInt( 100 ) );
        int direction = -1;
        CappedCLUtils.pop( cl, logicalID, direction );
        Assert.assertEquals( dataGroup.checkInspect( 120 ), true );
    }

    @AfterClass()
    public void tearDown() {
        try {
            db.getCollectionSpace( cappedCSName ).dropCollection( CLNAME );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private class InsertTask extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( CLNAME );
                int insertNums = 100000;
                int strLength = 256;
                CappedCLUtils.insertRecords( cl, insertNums, strLength );
            } catch ( BaseException e ) {
                e.printStackTrace();
                System.out.println( "master node disk full while inserting: "
                        + e.getErrorCode() );
            }
        }
    }

    private class PopTask extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( CLNAME );
                long logicalID = CappedCLUtils.getLogicalID( cl,
                        new Random().nextInt( 200000 ) );
                int direction = 1;
                CappedCLUtils.pop( cl, logicalID, direction );
            } catch ( BaseException e ) {
                e.printStackTrace();
                System.out.println( "master node disk full while poping: "
                        + e.getErrorCode() );
            }
        }
    }
}
