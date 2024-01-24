package com.sequoiadb.datasync.brokennetwork;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
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
import com.sequoiadb.datasync.CreateCLTask;

import java.util.*;

/**
 * FileName: CreateCLAndPrimaryNodeCutNet2933.java test content:when create cl,
 * the data group master node breaks the net testlink case:seqDB-2933
 * 
 * @author wuyan
 * @Date 2017.5.2
 * @version 1.00
 */

public class CreateCLAndPrimaryNodeCutNet2933 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private boolean clearFlag = false;
    private String preCLName = "cl_2933";
    private String clGroupName = null;
    private static final int CL_NUM = 500;
    private String connectUrl;
    private String brokenNetHost;
    private int count = 0;

    @BeforeClass
    public void setUp() {
        try {
            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            String cataPriHost = cataGroup.getMaster().hostName();
            brokenNetHost = groupMgr.getGroupByName( clGroupName ).getMaster()
                    .hostName();
            if ( cataPriHost.equals( brokenNetHost )
                    && !cataGroup.changePrimary() ) {
                throw new SkipException(
                        cataGroup.getGroupName() + " reelect fail" );
            }

            connectUrl = CommLib.getSafeCoordUrl( brokenNetHost );
            sdb = new Sequoiadb( connectUrl, "", "" );
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() {
        try {
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( brokenNetHost, 3, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            CreateCLTask dTask = new CreateCLTask(preCLName, clGroupName, CL_NUM);
            mgr.addTask( dTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 600S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );
            
            // clear context
            String match = preCLName;
            CommLib.waitContextClose( sdb, match, 300, false );
            // check result
            count = dTask.getCreateNum();
            checkCreateCLResult();

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 600S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );
            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            Utils.checkConsistencyCL(dataGroup, csName, preCLName);
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
     * check the result of create cl,create cl after recovery from failure
     */
    private void checkCreateCLResult() {
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ ShardingKey: { a: 1 }," + "ShardingType: 'hash', "
                        + "Partition: 2048, " + "ReplSize: 2, "
                        + "Compressed: true, " + "CompressionType: 'lzw',"
                        + "IsMainCL: false, " + "AutoSplit: false, "
                        + "Group: '" + clGroupName + "', "
                        + "AutoIndexId: true, "
                        + "EnsureShardingIndex: true }" );

        // when the last cl fails,which may have actually been created
        // successfully,may fail again
        try {
            cs.createCollection( preCLName + "_" + count, option );
        } catch ( BaseException e ) {
            // -22 SDB_DMS_EXIST
            if ( e.getErrorCode() != -22 ) {
                Assert.fail( "the error not -22: " + e.getErrorCode()
                        + e.getErrorType() );
            }
        }

        // create remaining cl
        try {
            int beginNo = count + 1;
            for ( int i = beginNo; i < CL_NUM; i++ ) {
                cs.createCollection( preCLName + "_" + i, option );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "create cl fail: " + e.getErrorCode() + e.getErrorType() );
        }

        // randomly select a cl insert data
        String clName = "";
        try {
            Random random = new Random();
            clName = preCLName + "_" + random.nextInt( CL_NUM );
            DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            cl.insert( "{a:1,b:2}" );
        } catch ( BaseException e ) {
            Assert.fail( clName + " insert fail: " + e.getErrorCode()
                    + e.getErrorType() );
        }
    }

    private void dropCL() {
        CollectionSpace commCS = sdb.getCollectionSpace( SdbTestBase.csName );
        for ( int i = 0; i < CL_NUM; i++ ) {
            String clName = preCLName + "_" + i;
            try {
                commCS.dropCollection( clName );
            } catch ( BaseException e ) {
                Assert.fail( "drop cl fail: " + e.getErrorCode()
                        + e.getErrorType() );
            }
        }
    }
}
