package com.sequoiadb.metaopr.noderestart;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.metaopr.Utils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * FileName: DropCLAndRestartPrimaryCatalog2298.java test content:when drop cl ,
 * restart the catalog group master node testlink case:seqDB-2298
 * 
 * @author wuyan
 * @Date 2017.4.18
 * @version 1.00
 */

public class DropCLAndRestartPrimaryCatalog2298 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String preCLName = "cl2298";
    private final int CL_NUM = 1000;
    private boolean clearFlag = false;
    private int count = 0;

    @BeforeClass
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            createCL();
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() throws InterruptedException {
        try {
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            NodeWrapper priNode = cataGroup.getMaster();

            FaultMakeTask faultTask = NodeRestart.getFaultMakeTask( priNode, 1,
                    10, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            DropCLTask dTask = new DropCLTask();
            mgr.addTask( dTask );
            mgr.execute();

            // TaskMgr check if there is any exception
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 120S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );

            // remove cl after fault recovery
            dropCL();

            // check result
            checkDropCLResult();
            Utils.checkConsistency( groupMgr );

            // Normal operating environment
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( !clearFlag ) {
                throw new SkipException( "to save environment" );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DropCLTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace commCS = db
                        .getCollectionSpace( SdbTestBase.csName );
                for ( int i = 0; i < CL_NUM; i++ ) {
                    String clName = preCLName + "_" + i;
                    commCS.dropCollection( clName );
                    count++;
                }
                throw new ReliabilityException( "drop cl should be fail" );
            } catch ( BaseException e ) {
                System.out.println( "the error i is =" + count );
            }
        }
    }

    public void createCL() {
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            for ( int i = 0; i < CL_NUM; i++ ) {
                String clName = preCLName + "_" + i;
                cs.createCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "create cl fail " + e.getErrorType() + ":"
                    + e.getMessage() );
        }
    }

    private void dropCL() {
        for ( int i = count; i < CL_NUM; i++ ) {
            String clName = preCLName + "_" + i;
            try {
                cs.dropCollection( clName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 ) {
                    Assert.assertTrue( false, "drop cl fail " + e.getErrorType()
                            + ":" + e.getMessage() );
                }
            }
        }
    }

    private void checkDropCLResult() {
        for ( int i = 0; i < CL_NUM; i++ ) {
            String clName = preCLName + "_" + i;
            Assert.assertFalse( cs.isCollectionExist( clName ),
                    "expect cl not exist, but cl exist." );
        }
    }

}