package com.sequoiadb.subcl.brokennetwork;

import java.util.Date;

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
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-2188: MainCL remove/truncate datas when dataGroup Primary
 *           Node is brokennetwork
 * @Author liuxiaoxuan
 * @Date 2017-08-18
 * @Version 1.00
 */

public class RemoveAndDataBroken2188 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean clearFlag = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String csName = "cs_2188";
    private String mainCLName = "maincl_2188";
    private String subCLName = "subcl_2188";
    private final int SUBCL_NUMS = 50;
    private String clGroupName = null;
    private String brokenNetHost = null;
    private String connectUrl = null;
    private final int INSERT_NUMS = 10000;

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

            System.out.println( "clGroupName: " + clGroupName + " cataPriHost: "
                    + cataPriHost + " brokenNetHost: " + brokenNetHost );
            connectUrl = CommLib.getSafeCoordUrl( brokenNetHost );
            sdb = new Sequoiadb( connectUrl, "", "" );
            createAndAttachCLs();
            insertData();
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( clearFlag ) {
                sdb.dropCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    @Test
    public void test() {

        try {
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( brokenNetHost, 1, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            RemoveTask removeTask = new RemoveTask();
            mgr.addTask( removeTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 600S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );

            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            Assert.assertEquals( dataGroup.checkInspect( 1 ), true,
                    "data is different on " + dataGroup.getGroupName() );
            checkRemovedResult();

            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void createAndAttachCLs() {
        try {
            cs = sdb.createCollectionSpace( csName );
            DBCollection mainCL = cs.createCollection( mainCLName,
                    ( BSONObject ) JSON.parse(
                            "{ShardingKey:{'a':1},ShardingType:'range',IsMainCL:true}" ) );
            BSONObject option = new BasicBSONObject();
            option.put( "Group", clGroupName );
            for ( int i = 0; i < SUBCL_NUMS; i++ ) {
                String sclFullName = csName + "." + subCLName + "_" + i;
                cs.createCollection( subCLName + "_" + i, option );
                mainCL.attachCollection( sclFullName,
                        ( BSONObject ) JSON.parse( "{ LowBound: { a: "
                                + i * INSERT_NUMS + " }, " + "UpBound: { a: "
                                + ( ( i + 1 ) * INSERT_NUMS ) + " } }" ) );
            }

        } catch ( BaseException e ) {
            Assert.fail(
                    "CreateAndAttach cl failed, errMsg:" + e.getMessage() );
        }
    }

    private void insertData() {
        DBCollection mainCL = sdb.getCollectionSpace( csName )
                .getCollection( mainCLName );
        BSONObject insrtObj = null;
        for ( int i = 0; i < SUBCL_NUMS * INSERT_NUMS; i++ ) {
            insrtObj = ( BSONObject ) JSON
                    .parse( "{ a: " + i + ", b: 'testaaaaaaaaaaa' }" );
            mainCL.insert( insrtObj );
        }
    }

    class RemoveTask extends OperateTask {

        @Override
        public void exec() throws Exception {
            long leftCount = 0;
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( connectUrl, "", "" );
                CollectionSpace newCS = db.getCollectionSpace( csName );
                DBCollection mainCL = newCS.getCollection( mainCLName );
                mainCL.delete( "{}" );
                leftCount = mainCL.getCount();
            } catch ( BaseException e ) {
                System.out.println( "left record count: " + leftCount );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    public void checkRemovedResult() {
        DBCollection mainCL = sdb.getCollectionSpace( csName )
                .getCollection( mainCLName );
        mainCL.delete( "{}" );
        long leftCount = mainCL.getCount();
        Assert.assertEquals( leftCount, 0, "data removed failed" );
    }

}
