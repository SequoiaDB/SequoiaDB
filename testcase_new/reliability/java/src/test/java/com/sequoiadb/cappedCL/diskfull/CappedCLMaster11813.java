package com.sequoiadb.cappedCL.diskfull;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
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

/**
 * @FileName seqDB-11813:创建集合时，主节点磁盘满
 * @Author liuxiaoxuan
 * @Date 2017-07-31
 */
public class CappedCLMaster11813 extends SdbTestBase {

    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private String cappedCSName = "cappedCS_MasterDIskFull_11813";
    private String cappedCLName = "cappedCL_MasterDIskFull_11813";
    private int successCLCounts = 0;
    private String dataGroupName;

    @BeforeClass
    public void setUp() {
        try {
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            if ( sdb.isCollectionSpaceExist( cappedCSName ) ) {
                sdb.dropCollectionSpace( cappedCSName );
            }
            sdb.createCollectionSpace( cappedCSName,
                    ( BSONObject ) JSON.parse( "{Capped:true}" ) );
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
            NodeWrapper primaryNode = dataGroup.getMaster();

            FaultMakeTask faultMakeTask = DiskFull.getFaultMakeTask(
                    primaryNode.hostName(), SdbTestBase.reservedDir, 1, 10 );
            TaskMgr taskMgr = new TaskMgr( faultMakeTask );
            taskMgr.addTask( new createCappedCLTask() );
            taskMgr.execute();

            Assert.assertEquals( taskMgr.isAllSuccess(), true,
                    taskMgr.getErrorMsg() );
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "checkBusinessWithLSN() occurs timeout" );
            Assert.assertEquals( dataGroup.checkInspect( 60 ), true,
                    "data is different on " + dataGroup.getGroupName() );

            for ( int num = 0; num < successCLCounts; num++ ) {
                CollectionSpace cappedCS = sdb
                        .getCollectionSpace( cappedCSName );
                cappedCS.getCollection( cappedCLName + "_" + num )
                        .insert( "{a:'Check cl'}" );
            }
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( "test reliabilityException: " + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( cappedCSName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class createCappedCLTask extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cappedCS = db
                        .getCollectionSpace( cappedCSName );
                BSONObject options = new BasicBSONObject();
                options.put( "Capped", true );
                options.put( "Size", 1024 );
                options.put( "Group", dataGroupName );
                for ( int num = 0; num < 400; num++ ) {
                    cappedCS.createCollection( cappedCLName + "_" + num,
                            options );
                    successCLCounts++;
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
                System.out.println(
                        "success create cl num is = " + successCLCounts );
            }
        }
    }

}
