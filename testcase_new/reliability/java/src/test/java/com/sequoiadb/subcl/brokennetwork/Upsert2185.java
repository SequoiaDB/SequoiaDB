package com.sequoiadb.subcl.brokennetwork;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.subcl.brokennetwork.commlib.Utils;
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

import java.util.Date;

/**
 * @FileName seqDB-2185: 在主表upsert大量数据时dataRG主节点断网
 * @Author linsuqiang
 * @Date 2017-03-16
 * @Version 1.00
 */

/*
 * 1、创建主表和子表 2、在主表更新大量数据，更新数据过程中将dataRG主节点网络断掉（如：使用cutnet.sh工具，命令格式为nohup
 * ./cutnet.sh &），检查upser/update执行结果 3、将dataRG主节点网络恢复，检查dataRG各节点数据是否完整一致；
 * 4、对原操作的主表重新更新数据，检查返回结果
 */

public class Upsert2185 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String mclName = "cl_2185";
    private String clGroup = null;
    private static final int SCLNUM = Utils.SCLNUM;
    private static final int RANGE_WIDTH = Utils.RANGE_WIDTH;
    private GroupWrapper dataGroup = null;
    private String dataPriHost = null;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            clGroup = groupMgr.getAllDataGroupName().get( 0 );
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            String cataPriHost = cataGroup.getMaster().hostName();
            dataGroup = groupMgr.getGroupByName( clGroup );
            dataPriHost = dataGroup.getMaster().hostName();
            if ( cataPriHost.equals( dataPriHost )
                    && !cataGroup.changePrimary() ) {
                throw new SkipException(
                        cataGroup.getGroupName() + " reelect fail" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            Utils.createMclAndScl( db, mclName, clGroup );
            Utils.attachAllScl( db, mclName );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            FaultMakeTask faultTask = BrokenNetwork
                    .getFaultMakeTask( dataPriHost, 0, 18 );
            TaskMgr mgr = new TaskMgr( faultTask );
            String safeUrl = CommLib.getSafeCoordUrl( dataPriHost );
            UpsertTask iTask = new UpsertTask( safeUrl );
            mgr.addTask( iTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            if ( !dataGroup.checkInspect( 1 ) ) {
                Assert.fail(
                        "data is different on " + dataGroup.getGroupName() );
            }
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            checkUpserted( db, iTask.getInsertedCnt() );
            checkUsable( db );
            runSuccess = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( !runSuccess ) {
            throw new SkipException( "to save environment" );
        }
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            Utils.dropMclAndScl( db, mclName );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    class UpsertTask extends OperateTask {
        private int insertedCnt = 0;
        private String safeUrl = null;
        private static final int RECORD_TOTAL = 10000;

        public UpsertTask( String safeUrl ) {
            this.safeUrl = safeUrl;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( safeUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( csName );
                DBCollection mcl = cs.getCollection( mclName );
                int mclRange = SCLNUM * RANGE_WIDTH;
                for ( int i = 0; i < RECORD_TOTAL; i++ ) {
                    int valueInRange = i % mclRange;
                    BSONObject matcher = ( BSONObject ) JSON
                            .parse( "{ i: " + i + " }" );
                    BSONObject modifier = ( BSONObject ) JSON
                            .parse( "{ $set: { a: " + valueInRange + " } }" );
                    BSONObject setOnIns = ( BSONObject ) JSON
                            .parse( "{ b: " + valueInRange + " }" );
                    mcl.upsert( matcher, modifier, null, setOnIns );
                    insertedCnt++;
                }
            } catch ( BaseException e ) {
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }

        public int getInsertedCnt() {
            return insertedCnt;
        }
    }

    private void checkUpserted( Sequoiadb db, int insertedCnt ) {
        DBCollection mcl = db.getCollectionSpace( csName )
                .getCollection( mclName );
        if ( mcl.getCount() < insertedCnt ) {
            Assert.fail( "records count is less then the expected." );
        }
        DBCursor cursor = mcl.query( null, null, "{ _id: 1 }", null );
        int mclRange = SCLNUM * RANGE_WIDTH;
        for ( int i = 0; i < insertedCnt; i++ ) {
            BSONObject res = cursor.getNext();
            int expValue = i % mclRange;
            int iField = ( int ) res.get( "i" );
            int aField = ( int ) res.get( "a" );
            int bField = ( int ) res.get( "b" );
            if ( iField != i ) {
                Assert.fail( "fail to checkInserted. i field expected: " + i
                        + " but found: " + iField );
            }
            if ( aField != expValue ) {
                Assert.fail( "fail to checkInserted. a field expected: "
                        + expValue + " but found: " + aField );
            }
            if ( bField != expValue ) {
                Assert.fail( "fail to checkInserted. b field expected: "
                        + expValue + " but found: " + bField );
            }
        }
        cursor.close();
    }

    private void checkUsable( Sequoiadb db ) throws ReliabilityException {
        try {
            DBCollection mcl = db.getCollectionSpace( csName )
                    .getCollection( mclName );
            for ( int i = 0; i < SCLNUM; i++ ) {
                int lowBound = i * RANGE_WIDTH;
                int upBound = ( i + 1 ) * RANGE_WIDTH - 1;
                mcl.insert( "{ a: " + lowBound + ", d: " + i + " }" );
                mcl.insert( "{ a: " + upBound + ", d: " + i + " }" );
                if ( mcl.getCount( "{ d: " + i + " }" ) != 2 ) {
                    Assert.fail( "scl " + i + " is not usable" );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}