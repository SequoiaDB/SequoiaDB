package com.sequoiadb.subcl.brokennetwork;

import java.util.ArrayList;
import java.util.List;

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

/**
 * @FileName seqDB-2189: 在主表做aggregate时dataRG主节点断网
 * @Author linsuqiang
 * @Date 2017-03-20
 * @Version 1.00
 */

/*
 * 1、创建主表和子表 2、在主表使用aggregate查询数据（使用多个聚集符组合查询），
 * 聚集操作过程中将dataRG主节点网络断掉（如：使用cutnet.sh工具，命令格式为nohup ./cutnet.sh &），检查聚集操作结果
 * 3、将dataRG主节点网络恢复，检查dataRG各节点数据是否完整一致，并在此做聚集操作，检查返回结果
 */

public class Aggregate2190 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String mclName = "cl_2189";
    private String clGroup = null;
    private static final int SCLNUM = Utils.SCLNUM;
    private static final int RANGE_WIDTH = Utils.RANGE_WIDTH;
    private static final int MCL_RANGE = SCLNUM * RANGE_WIDTH;
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
            insertData( db );
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
                    .getFaultMakeTask( dataPriHost, 0, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            String safeUrl = CommLib.getSafeCoordUrl( dataPriHost );
            mgr.addTask( new AggregateTask( safeUrl ) );
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
            checkAggregate( db );
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

    private class AggregateTask extends OperateTask {
        private String safeUrl = null;

        public AggregateTask( String safeUrl ) {
            this.safeUrl = safeUrl;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( safeUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( csName );
                DBCollection mcl = cs.getCollection( mclName );
                List< BSONObject > obj = new ArrayList< BSONObject >();
                BSONObject match = ( BSONObject ) JSON
                        .parse( "{ $match: {} }" );
                obj.add( match );
                BSONObject groupAndAvg = ( BSONObject ) JSON.parse(
                        "{ $group: { _id: '$a', avg_val: { $avg: '$i' }, a: { $first: '$a' } } }" );
                obj.add( groupAndAvg );
                BSONObject sort = ( BSONObject ) JSON
                        .parse( "{ $sort: { a: -1 } }" );
                obj.add( sort );
                DBCursor cursor = mcl.aggregate( obj );

                int expA = MCL_RANGE - 1; // expected value of field "a"
                double expAvgVal = expA + 475000.00; // expected value of field
                                                     // "avg_val"
                while ( cursor.hasNext() ) {
                    BSONObject res = cursor.getNext();
                    int actA = ( int ) res.get( "a" );
                    double actAvgVal = ( double ) res.get( "avg_val" );
                    if ( actA != expA || actAvgVal != expAvgVal ) {
                        throw new ReliabilityException( "aggregate failed: "
                                + "actual: " + actA + " " + actAvgVal + " "
                                + "expect: " + expA + " " + expAvgVal );
                    }
                    // update expect value
                    --expA;
                    --expAvgVal;
                }
                cursor.close();
            } catch ( BaseException e ) {
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void insertData( Sequoiadb db ) {
        DBCollection mcl = db.getCollectionSpace( csName )
                .getCollection( mclName );
        int recTotal = 1000000;
        int batchRecs = 100000;
        for ( int k = 0; k < recTotal; k += batchRecs ) {
            List< BSONObject > recs = new ArrayList< BSONObject >();
            for ( int i = 0 + k; i < batchRecs + k; i++ ) {
                recs.add( new BasicBSONObject( "i", i ).append( "a",
                        i % MCL_RANGE ) );
            }
            mcl.insert( recs, DBCollection.FLG_INSERT_CONTONDUP );
        }
    }

    private void checkAggregate( Sequoiadb db ) {
        DBCollection mcl = db.getCollectionSpace( csName )
                .getCollection( mclName );
        List< BSONObject > obj = new ArrayList< BSONObject >();
        BSONObject match = ( BSONObject ) JSON.parse( "{ $match: {} }" );
        obj.add( match );
        BSONObject group = ( BSONObject ) JSON.parse(
                "{ $group: { _id: '$a', avg_val: { $avg: '$i' }, a: { $first: '$a' } } }" );
        obj.add( group );
        BSONObject sort = ( BSONObject ) JSON.parse( "{ $sort: { a: -1 } }" );
        obj.add( sort );
        DBCursor cursor = mcl.aggregate( obj );

        int expA = MCL_RANGE - 1; // expected value of field "a"
        double expAvgVal = expA + 475000.00; // expected value of field
                                             // "avg_val"
        while ( cursor.hasNext() ) {
            BSONObject res = cursor.getNext();
            int actA = ( int ) res.get( "a" );
            Assert.assertEquals( actA, expA );
            ;
            double actAvgVal = ( double ) res.get( "avg_val" );
            Assert.assertEquals( actAvgVal, expAvgVal );
            // update expect value
            --expA;
            --expAvgVal;
        }
        cursor.close();
    }
}