package com.sequoiadb.subcl.killnode;

import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
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

/**
 * @FileName seqDB-2440: MainCL remove/truncate datas when dataGroup Primary
 *           Node is killed
 * @Author liuxiaoxuan
 * @Date 2017-08-22
 * @Version 1.00
 */
public class KillNodeSubcl2440 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private boolean clearFlag = false;
    private String csName = "cs_2440";
    private String mainCLName = "maincl_2440";
    private String subCLName = "subcl_2440";
    private String clGroupName = null;
    private String domainName = "domain_2440";
    private CollectionSpace cs = null;
    private final int MAINCL_NUMS = 10;
    private final int SUBCL_NUMS = 50;
    private final int INSERT_NUMS = 100;
    private int bound = 0;

    @BeforeClass()
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();

            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            createDomain( groupMgr.getAllDataGroupName() );
            createAndAttachCLs();
            insertData();
        } catch ( ReliabilityException e ) {
            if ( sdb != null ) {
                sdb.close();
            }
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        try {
            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            NodeWrapper primaryNode = dataGroup.getMaster();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    primaryNode.hostName(), primaryNode.svcName(), 1 );
            TaskMgr mgr = new TaskMgr( faultTask );
            mgr.addTask( new RemoveTask() );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );
            Assert.assertEquals( dataGroup.checkInspect( 1 ), true,
                    "data is different on " + dataGroup.getGroupName() );

            checkRemovedResult();
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
                sdb.dropCollectionSpace( csName );
                sdb.dropDomain( domainName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }

        }
    }

    private void createDomain( List< String > groupNames ) {
        BSONObject domainOption = new BasicBSONObject();
        BSONObject groups = new BasicBSONList();
        for ( int i = 0; i < groupNames.size(); i++ ) {
            groups.put( "" + i, groupNames.get( i ) );
        }
        domainOption.put( "Groups", groups );
        domainOption.put( "AutoSplit", true );
        sdb.createDomain( domainName, domainOption );
    }

    public void createAndAttachCLs() {
        try {
            cs = sdb.createCollectionSpace( csName, ( BSONObject ) JSON
                    .parse( "{Domain: '" + domainName + "'}" ) );
            for ( int i = 0; i < MAINCL_NUMS; i++ ) {
                DBCollection mainCL = cs.createCollection( mainCLName + "_" + i,
                        ( BSONObject ) JSON.parse(
                                "{ShardingKey:{'a':1},ShardingType:'range',IsMainCL:true}" ) );
                for ( int j = 0; j < SUBCL_NUMS; j++ ) {
                    DBCollection subCL = cs.createCollection(
                            subCLName + "_" + i + "_" + j,
                            ( BSONObject ) JSON.parse(
                                    "{ShardingKey:{b:1},ShardingType:'hash',"
                                            + "Group:'" + clGroupName
                                            + "'}" ) );
                    String sclFullName = csName + "." + subCLName + "_" + i
                            + "_" + j;
                    mainCL.attachCollection( sclFullName,
                            ( BSONObject ) JSON.parse( "{ LowBound: { a: "
                                    + ( bound * INSERT_NUMS ) + " }, "
                                    + "UpBound: { a: "
                                    + ( ( bound + 1 ) * INSERT_NUMS )
                                    + " } }" ) );
                    ++bound;
                }

            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "CreateAndAttach cl failed, errMsg:" + e.getMessage() );
        }
    }

    public void insertData() {
        for ( int i = 0; i < MAINCL_NUMS; i++ ) {
            DBCollection mainCL = cs.getCollection( mainCLName + "_" + i );
            try {
                int num = 0;
                int insertNums = ( i + 1 ) * SUBCL_NUMS * INSERT_NUMS;
                for ( num = i * SUBCL_NUMS
                        * INSERT_NUMS; num < insertNums; num++ ) {
                    BSONObject insrtObj = new BasicBSONObject();
                    insrtObj.put( "a", num );
                    insrtObj.put( "b", "test" + num );
                    mainCL.insert( insrtObj );
                }
            } catch ( BaseException e ) {
                Assert.fail( "insert failed: " + e.getMessage() );
            }
        }

    }

    class RemoveTask extends OperateTask {

        @Override
        public void exec() throws Exception {
            long leftCount = 0;
            Sequoiadb db = null;
            for ( int i = 0; i < MAINCL_NUMS; i++ ) {
                try {
                    db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                    CollectionSpace newCS = db.getCollectionSpace( csName );
                    DBCollection mainCL = newCS
                            .getCollection( mainCLName + "_" + i );
                    mainCL.delete( "{}" );
                    leftCount = mainCL.getCount();
                } catch ( BaseException e ) {
                    System.out.println( mainCLName + "_" + i
                            + " left record count: " + leftCount );
                } finally {
                    if ( db != null ) {
                        db.close();
                    }
                }
            }
        }
    }

    public void checkRemovedResult() {
        for ( int i = 0; i < MAINCL_NUMS; i++ ) {
            DBCollection mainCL = sdb.getCollectionSpace( csName )
                    .getCollection( mainCLName + "_" + i );
            mainCL.delete( "{}" );
            long leftCount = mainCL.getCount();
            Assert.assertEquals( leftCount, 0,
                    mainCLName + "_" + i + "  removed data failed" );
        }
    }

}
