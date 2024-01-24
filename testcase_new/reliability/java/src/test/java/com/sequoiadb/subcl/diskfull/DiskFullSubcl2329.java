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
 * @FileName seqDB-2329: detachCLs when Catalog Primary Node is diskfull
 * @Author liuxiaoxuan
 * @Date 2017-08-18
 * @Version 1.00
 */

public class DiskFullSubcl2329 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private boolean clearFlag = false;
    private String csName = "cs_2329";
    private String mainCLName = "maincl_2329";
    private String subCLName = "subcl_2329";
    private String clGroupName = null;
    private CollectionSpace cs = null;
    private final int MAINCL_NUMS = 10;
    private final int SUBCL_NUMS = 100;
    private int lastDetachedMainCL = 0;
    private int lastDetachSubCL = 0;

    @BeforeClass
    public void setUp() {

        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            createAndAttachCLs();
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
            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            NodeWrapper primaryNode = dataGroup.getMaster();

            FaultMakeTask faultTask = DiskFull.getFaultMakeTask(
                    primaryNode.hostName(), SdbTestBase.reservedDir, 1, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            DetachCLTask detachCLTask = new DetachCLTask();
            mgr.addTask( detachCLTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 600S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "check LSN consistency fail" );
            Assert.assertEquals( dataGroup.checkInspect( 1 ), true,
                    "data is different on " + dataGroup.getGroupName() );

            checkDetachResult();
            checkInsert();
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void createAndAttachCLs() {
        try {
            cs = sdb.createCollectionSpace( csName );
            for ( int i = 0; i < MAINCL_NUMS; i++ ) {
                DBCollection mainCL = cs.createCollection( mainCLName + "_" + i,
                        ( BSONObject ) JSON.parse(
                                "{ShardingKey:{'key':1},ShardingType:'range',IsMainCL:true}" ) );
                for ( int j = 0; j < SUBCL_NUMS; j++ ) {
                    BSONObject subclOption = new BasicBSONObject();
                    subclOption.put( "Group", clGroupName );
                    cs.createCollection( subCLName + "_" + i + "_" + j,
                            subclOption );
                    String sclFullName = csName + "." + subCLName + "_" + i
                            + "_" + j;
                    mainCL.attachCollection( sclFullName,
                            ( BSONObject ) JSON.parse( "{ LowBound: { key: "
                                    + ( ( i + j ) * 100 ) + " }, "
                                    + "UpBound: { key: "
                                    + ( ( i + j + 1 ) * 100 ) + " } }" ) );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    "CreateAndAttach cl failed, errMsg:" + e.getMessage() );
        }
    }

    private class DetachCLTask extends OperateTask {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                CollectionSpace newCS = db.getCollectionSpace( csName );
                for ( int i = 0; i < MAINCL_NUMS; i++ ) {
                    lastDetachSubCL = 0;
                    DBCollection mainCL = newCS
                            .getCollection( mainCLName + "_" + i );
                    for ( int j = 0; j < SUBCL_NUMS; j++ ) {
                        String sclFullName = csName + "." + subCLName + "_" + i
                                + "_" + j;
                        mainCL.detachCollection( sclFullName );
                        ++lastDetachSubCL;
                    }

                    ++lastDetachedMainCL;
                }
            } catch ( BaseException e ) {
                System.out.println( "success detach cl num is = "
                        + ( lastDetachedMainCL * lastDetachSubCL ) );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }

    }

    public void checkDetachResult() {
        int lastMainCLNo = ( lastDetachedMainCL > 0 )
                ? ( lastDetachedMainCL - 1 )
                : 0;
        int lastSubCLNo = ( lastDetachSubCL > 0 ) ? ( lastDetachSubCL - 1 ) : 0;
        DBCollection mainCL = cs
                .getCollection( mainCLName + "_" + lastMainCLNo );
        try {
            if ( 0 == lastMainCLNo && 0 == lastSubCLNo ) {
                return;
            }
            String sclFullName = csName + "." + subCLName + "_" + lastMainCLNo
                    + "_" + lastSubCLNo;
            mainCL.detachCollection( sclFullName );
        } catch ( BaseException e ) {

            Assert.assertEquals( e.getErrorCode(), -242,
                    "the error code is not -242: " + e.getErrorCode() );
        }

    }

    public void checkInsert() {
        for ( int i = 0; i < lastDetachedMainCL; i++ ) {
            for ( int j = 0; j < lastDetachSubCL; j++ ) {
                try {
                    String sclFullName = subCLName + "_" + i + "_" + j;
                    DBCollection detachedCL = cs.getCollection( sclFullName );
                    int insertNums = 1000;
                    for ( int num = 0; num < insertNums; num++ ) {
                        BSONObject insrtObj = new BasicBSONObject();
                        insrtObj.put( "key", "testaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                + i * j * insertNums );
                        detachedCL.insert( insrtObj );
                    }
                } catch ( BaseException e ) {
                    Assert.fail( "insert failed: " + e.getMessage() );
                }
            }
        }
    }

}
