package com.sequoiadb.cappedCL.killnode;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
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
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-15789:插入记录同时备节点异常重启
 * @Author zhaoyu
 * @Date 2019-7-24
 */
public class CappedCLKillNode15789A extends SdbTestBase {

    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String cappedCLName = "cappedCL_killNode_15789A";
    private String dataGroupName;
    private StringBuffer strBuffer = null;
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 2000 );
    private int threadNum = 10;

    @BeforeClass
    public void setUp() {
        try {
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = sdb.getCollectionSpace( cappedCSName );
            cl = cs.createCollection( cappedCLName,
                    ( BSONObject ) JSON.parse( "{Capped:true,Size:1024}" ) );
            // 构造插入的字符串
            strBuffer = new StringBuffer();
            for ( int len = 0; len < stringLength; len++ ) {
                strBuffer.append( "a" );
            }
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( this.getClass().getName()
                    + "setUp error, error description:" + e.getMessage() );
        }

    }

    @Test
    public void test() {
        try {
            dataGroupName = groupMgr.getAllDataGroupName().get( 0 );
            GroupWrapper dataGroup = groupMgr.getGroupByName( dataGroupName );
            NodeWrapper slaveNode = dataGroup.getSlave();

            FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask(
                    slaveNode.hostName(), slaveNode.svcName(),
                    1 + ( int ) Math.random() * 10 );
            TaskMgr mgr = new TaskMgr( faultMakeTask );
            for ( int i = 0; i < threadNum; i++ ) {
                mgr.addTask( new InsertTask() );
            }

            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "checkBusinessWithLSN() occurs timeout" );
            Assert.assertEquals( dataGroup.checkInspect( 60 ), true,
                    "data is different on " + dataGroup.getGroupName() );

            BasicBSONObject insertObj = new BasicBSONObject();
            insertObj.put( "a", strBuffer.toString() );
            cl.insert( insertObj );

            // 校验主节点id字段
            Assert.assertTrue( CappedCLUtils.checkLogicalID( sdb, cappedCSName,
                    cappedCLName, stringLength ) );
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "checkBusinessWithLSN() occurs timeout" );
            Assert.assertEquals( dataGroup.checkInspect( 60 ), true,
                    "data is different on " + dataGroup.getGroupName() );

        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( "test reliabilityException: " + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( cappedCLName );
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
                DBCollection cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( cappedCLName );
                for ( int i = 0; i < 10000; i++ ) {
                    BasicBSONObject insertObj = new BasicBSONObject();
                    insertObj.put( "a", strBuffer.toString() );
                    cl.insert( insertObj );
                }
            } catch ( BaseException e ) {
                throw e;
            }
        }
    }

}
