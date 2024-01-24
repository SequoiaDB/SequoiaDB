package com.sequoiadb.subcl.diskfull;

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
 * @FileName seqDB-2328: attachCLs when Catalog Primary Node is diskfull
 * @Author liuxiaoxuan
 * @Date 2017-08-22
 * @Version 1.00
 */
public class DiskFullSubcl2328 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private boolean clearFlag = false;
    private String csName = "cs_2328";
    private String mainCLName = "maincl_2328";
    private String subCLName = "subcl_2328";
    private String clGroupName = null;
    private CollectionSpace cs = null;
    private final int MAINCL_NUMS = 10;
    private final int SUBCL_NUMS = 100;
    private final int INSERT_NUMS = 100;
    private int bound = 0;
    private int lastSuccessMainCL = 0;
    private int lastSuccessSubCL = 0;

    @BeforeClass
    public void setUp() {

        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            createMainAndSubCLs();
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        try {
            if ( clearFlag ) {
                for ( int i = 0; i < 30; i++ ) {
                    try {
                        sdb.dropCollectionSpace( csName );
                        break;
                    } catch ( BaseException e ) {
                        if ( e.getErrorCode() == -147 && i < 29 ) {
                            Thread.sleep( 1000 );
                        } else {
                            throw e;
                        }
                    }
                }
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    @Test
    public void test() {
        try {
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            NodeWrapper cataPrimary = cataGroup.getMaster();

            FaultMakeTask faultTask = DiskFull.getFaultMakeTask(
                    cataPrimary.hostName(), SdbTestBase.reservedDir, 1, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            AttachCLTask attachCLTask = new AttachCLTask();
            mgr.addTask( attachCLTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 600S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );

            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            Assert.assertEquals( dataGroup.checkInspect( 1 ), true,
                    "data is different on " + dataGroup.getGroupName() );

            checkAttachResult();
            checkInsert();
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void createMainAndSubCLs() {
        try {
            cs = sdb.createCollectionSpace( csName );
            for ( int i = 0; i < MAINCL_NUMS; i++ ) {
                cs.createCollection( mainCLName + "_" + i, ( BSONObject ) JSON
                        .parse( "{ShardingKey:{'key':1},ShardingType:'range',IsMainCL:true}" ) );
                for ( int j = 0; j < SUBCL_NUMS; j++ ) {
                    BSONObject subclOption = new BasicBSONObject();
                    subclOption.put( "Group", clGroupName );
                    cs.createCollection( subCLName + "_" + i + "_" + j,
                            subclOption );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "CreateAndAttach cl failed, errMsg:" + e.getMessage() );
        }
    }

    private class AttachCLTask extends OperateTask {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace newCS = db.getCollectionSpace( csName );

                for ( int i = 0; i < MAINCL_NUMS; i++ ) {
                    lastSuccessSubCL = 0;
                    DBCollection mainCL = newCS
                            .getCollection( mainCLName + "_" + i );
                    for ( int j = 0; j < SUBCL_NUMS; j++ ) {
                        String sclFullName = csName + "." + subCLName + "_" + i
                                + "_" + j;
                        mainCL.attachCollection( sclFullName,
                                ( BSONObject ) JSON.parse( "{ LowBound: { key: "
                                        + ( bound * INSERT_NUMS ) + " }, "
                                        + "UpBound: { key: "
                                        + ( bound + 1 ) * INSERT_NUMS
                                        + " } }" ) );
                        ++bound;
                        ++lastSuccessSubCL;
                    }

                    lastSuccessMainCL++;
                }
            } catch ( BaseException e ) {
                System.out.println( "success attach cl num is = "
                        + ( lastSuccessMainCL * lastSuccessSubCL ) );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }

    }

    public void checkAttachResult() {
        int lastMainCLNo = ( lastSuccessMainCL > 0 ) ? ( lastSuccessMainCL - 1 )
                : 0;
        int lastSubCLNo = ( lastSuccessSubCL > 0 ) ? ( lastSuccessSubCL - 1 )
                : 0;
        DBCollection mainCL = cs
                .getCollection( mainCLName + "_" + lastMainCLNo );
        try {
            if ( 0 == lastMainCLNo && 0 == lastSubCLNo ) {
                return;
            }
            String sclFullName = csName + "." + subCLName + "_" + lastMainCLNo
                    + "_" + lastSubCLNo;
            mainCL.attachCollection( sclFullName,
                    ( BSONObject ) JSON.parse(
                            "{ LowBound: { key: " + ( bound * INSERT_NUMS )
                                    + " }, " + "UpBound: { key: "
                                    + ( bound * INSERT_NUMS ) + " } }" ) );

        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -235,
                    "the error code is not -235: " + e.getErrorCode() );
        }

    }

    public void checkInsert() {
        try {
            for ( int i = 0; i < lastSuccessMainCL; i++ ) {
                DBCollection mainCL = sdb.getCollectionSpace( csName )
                        .getCollection( mainCLName + "_" + i );
                int lowBound = i * SUBCL_NUMS * INSERT_NUMS;
                int upBound = ( i + 1 ) * SUBCL_NUMS * INSERT_NUMS - 1;
                mainCL.insert( "{ key: " + lowBound + "}" );
                mainCL.insert( "{ key: " + upBound + "}" );
                if ( 2 != mainCL.getCount() ) {
                    Assert.fail( "subcl_" + i + " is disabled" );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( "check insert again failed: " + e.getMessage() );
        }
    }
}
