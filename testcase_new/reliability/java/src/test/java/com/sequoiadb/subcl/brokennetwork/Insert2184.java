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
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.Date;

/**
 * @FileName seqDB-2184: 在主表insert大量数据时dataRG备节点断网
 * @Author linsuqiang
 * @Date 2017-03-15
 * @Version 1.00
 */

/*
 * 1、创建主表和子表 2、在主表插入大量数据（如每个子表插入10万条数据），
 * 插入数据过程中将cl所在dataRG主节点网络断掉（如：使用cutnet.sh工具，命令格式为nohup ./cutnet.sh &），
 * 检查insert执行结果 3、将dataRG备节点网络恢复，检查dataRG各节点数据是否完整一致； 4、对原操作的主表重新插入数据，检查返回结果
 */

public class Insert2184 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String mclName = "cl_2184";
    private String clGroup = null;
    private static final int SCLNUM = Utils.SCLNUM;
    private static final int RANGE_WIDTH = Utils.RANGE_WIDTH;
    private GroupWrapper dataGroup = null;
    private String dataSlvHost = null;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            String cataPriHost = cataGroup.getMaster().hostName();
            clGroup = groupMgr.getAllDataGroupName().get( 0 );
            dataGroup = groupMgr.getGroupByName( clGroup );
            dataSlvHost = dataGroup.getSlave().hostName();
            if ( cataPriHost.equals( dataSlvHost )
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
                    .getFaultMakeTask( dataSlvHost, 0, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            String safeUrl = CommLib.getSafeCoordUrl( dataSlvHost );
            InsertTask iTask = new InsertTask( safeUrl );
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
            checkInserted( db, iTask.getInsertedCnt() );
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

    class InsertTask extends OperateTask {
        private int insertedCnt = 0;
        private String safeUrl = null;
        private static final int RECORD_TOTAL = 100000;

        public InsertTask( String safeUrl ) {
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
                    mcl.insert( "{ a: " + valueInRange + " }" );
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

    private void checkInserted( Sequoiadb db, int insertedCnt ) {
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
            int actValue = ( int ) res.get( "a" );
            if ( actValue != expValue ) {
                Assert.fail( "fail to checkInserted. expected: " + expValue
                        + " but found: " + actValue );
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
                mcl.insert( "{ a: " + lowBound + ", b: " + i + " }" );
                mcl.insert( "{ a: " + upBound + ", b: " + i + " }" );
                if ( mcl.getCount( "{ b: " + i + " }" ) != 2 ) {
                    Assert.fail( "scl " + i + " is not usable" );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }
}