package com.sequoiadb.metaopr.noderestart;

import java.util.Date;

import com.sequoiadb.metaopr.Utils;
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
import com.sequoiadb.metaopr.commons.MyUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.datasync.CreateCLTask ;
/**
 * FileName: CreateCLAndRestartPrimaryCatalog2294.java test content:when create
 * cl , restart the catalog group master node testlink case:seqDB-2294
 * 
 * @author wuyan
 * @Date 2017.4.20
 * @version 1.00
 */

public class CreateCLAndRestartPrimaryCatalog2294 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private boolean clearFlag = false;
    private String preCLName = "cl_2294";
    private final int CL_NUM = 1000;
    private int count = 0;

    @BeforeClass
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
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
            CreateCLTask cTask = new CreateCLTask( preCLName, CL_NUM);
            mgr.addTask( cTask );
            mgr.execute();

            // TaskMgr check if there is any exception
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );

            // check result
            count = cTask.getCreateNum() ;
            checkCreateCLResult();
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
            if ( clearFlag ) {
                dropCL();
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


    /**
     * check the result of create cl the result: 1. to create cl success,create
     * the same cl again failed 2. to create cl fail,create the same cl again
     * successfully 3. to create cl fail,the actual cl created successfully did
     * not return, creating the same cl failed again
     */
    private void checkCreateCLResult() {
        if ( CL_NUM == count ) {
            try {
                String sameCLName = preCLName + "_" + ( count - 1 );
                cs.createCollection( sameCLName );
                Assert.fail( "create the same cl should be fail" );
            } catch ( BaseException e ) {
                // -22 SDB_DMS_EXIST
                if ( e.getErrorCode() != -22 ) {
                    Assert.fail( "the error not -22: " + e.getErrorType() );
                }
            }
            try {
                String newclName = preCLName + "_" + count;
                cs.createCollection( newclName );
            } catch ( BaseException e ) {
                Assert.fail( "create new cl fail: " + e.getErrorCode()
                        + e.getMessage() );

            }
        } else {
            // create cl fail,the count is not equals CL_NUM
            try {
                cs.createCollection( preCLName + "_" + count );
            } catch ( BaseException e ) {
                // -22 SDB_DMS_EXIST
                if ( e.getErrorCode() != -22 ) {
                    Assert.fail( "the error not -22: " + e.getErrorType() );
                }
            }
        }
        MyUtil.checkListCL( sdb, csName, preCLName, count + 1 );
        insertByCL();
    }

    private void insertByCL() {
        for ( int i = 0; i < count; i++ ) {
            String clName = preCLName + "_" + i;
            DBCollection cl = cs.getCollection( clName );
            cl.insert( "{ a: 1 }" );
            Assert.assertEquals( cl.getCount( "{a:1}" ), 1,
                    "the insert data is error" );
        }
    }

    private void dropCL() {
        int clnums = count + 1;
        for ( int i = 0; i < clnums; i++ ) {
            String clName = preCLName + "_" + i;
            cs.dropCollection( clName );
        }
    }
}
