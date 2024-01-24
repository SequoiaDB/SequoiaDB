package com.sequoiadb.subcl.brokennetwork;

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
import com.sequoiadb.subcl.brokennetwork.commlib.Utils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * @FileName seqDB-2189: 在主表remove大量数据时dataRG备节点断网
 * @Author linsuqiang
 * @Date 2017-03-20
 * @Version 1.00
 */

/*
 * 1、创建主表和子表（分区方式：主表range，子表hash，AutoSplit：false）
 * 2、在主表删除大量数据，删除数据过程中将dataRG备节点网络断掉（如：使用cutnet.sh工具，命令格式为nohup ./cutnet.sh
 * &），检查remove/truncate执行结果 3、将dataRG备节点网络恢复，查询dataRG备节点数据是否完整一致
 */

// TODO: 用例暂时停用，待JIRA-2479问题解决后再启用
public class Remove2189 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String domainName = "domain_2189";
    private String csName = "cs_2189";
    private String mclName = "cl_2189";
    private String clGroup = null;
    private static final int SCLNUM = Utils.SCLNUM;
    private static final int RANGE_WIDTH = Utils.RANGE_WIDTH;
    private static final int TOTAL_REC_CNT = 1000000;
    private GroupWrapper dataGroup = null;
    private String dataSlvHost = null;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusinessWithLSN( 300 ) ) {
                throw new SkipException( "checkBusiness failed" );
            }

            clGroup = groupMgr.getAllDataGroupName().get( 0 );
            GroupWrapper cataGroup = groupMgr
                    .getGroupByName( "SYSCatalogGroup" );
            String cataPriHost = cataGroup.getMaster().hostName();
            dataGroup = groupMgr.getGroupByName( clGroup );
            dataSlvHost = dataGroup.getSlave().hostName();
            if ( cataPriHost.equals( dataSlvHost )
                    && !cataGroup.changePrimary() ) {
                throw new SkipException(
                        cataGroup.getGroupName() + " reelect fail" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            createDomainAndCs( db, groupMgr.getAllDataGroupName() );
            createMclAndScl( db );
            attachAllScl( db );
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
                    .getFaultMakeTask( dataSlvHost, 0, 10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            String safeUrl = CommLib.getSafeCoordUrl( dataSlvHost );
            RemoveTask rTask = new RemoveTask( safeUrl );
            mgr.addTask( rTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 1200 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            if ( !dataGroup.checkInspect( 1 ) ) {
                Assert.fail(
                        "data is different on " + dataGroup.getGroupName() );
            }
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            checkRemoved( db );
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
            dropDomainAndCs( db );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    class RemoveTask extends OperateTask {
        private String safeUrl = null;

        public RemoveTask( String safeUrl ) {
            this.safeUrl = safeUrl;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( safeUrl, "", "" );
                CollectionSpace cs = db.getCollectionSpace( csName );
                DBCollection mcl = cs.getCollection( mclName );
                mcl.delete( "{}" );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void createDomainAndCs( Sequoiadb db, List< String > dataRGNames ) {
        BSONObject domainOpt = new BasicBSONObject();
        BSONObject groups = new BasicBSONList();
        for ( int i = 0; i < dataRGNames.size(); i++ ) {
            groups.put( "" + i, dataRGNames.get( i ) );
        }
        domainOpt.put( "Groups", groups );
        domainOpt.put( "AutoSplit", false );
        db.createDomain( domainName, domainOpt );

        BSONObject csOpt = new BasicBSONObject();
        csOpt.put( "Domain", domainName );
        db.createCollectionSpace( csName, csOpt );
    }

    private void createMclAndScl( Sequoiadb db ) {
        CollectionSpace cs = db.getCollectionSpace( csName );
        cs.createCollection( mclName,
                ( BSONObject ) JSON.parse( "{ ShardingKey: { a: 1 }, "
                        + "ShardingType: 'range', IsMainCL: true, Group: '"
                        + clGroup + "', ReplSize: 1 }" ) );
        BSONObject sclOpt = ( BSONObject ) JSON.parse(
                "{ ShardingKey: { a: 1 }, " + "ShardingType: 'hash', Group: '"
                        + clGroup + "', ReplSize: 1 }" );
        for ( int i = 0; i < SCLNUM; i++ ) {
            String sclName = mclName + "_" + i;
            cs.createCollection( sclName, sclOpt );
        }
    }

    private void attachAllScl( Sequoiadb db ) {
        DBCollection mcl = db.getCollectionSpace( csName )
                .getCollection( mclName );
        int rangeStart = 0;
        for ( int i = 0; i < SCLNUM; i++ ) {
            int rangeEnd = rangeStart + RANGE_WIDTH;
            String sclFullName = csName + "." + mclName + "_" + i;
            mcl.attachCollection( sclFullName,
                    ( BSONObject ) JSON.parse( "{ LowBound: { a: " + rangeStart
                            + " }, " + "UpBound: { a: " + rangeEnd + " } }" ) );
            rangeStart += RANGE_WIDTH;
        }
    }

    private void insertData( Sequoiadb db ) {
        DBCollection mcl = db.getCollectionSpace( csName )
                .getCollection( mclName );
        int mclRange = SCLNUM * RANGE_WIDTH;
        List< BSONObject > recs = new ArrayList< BSONObject >();
        for ( int i = 0; i < TOTAL_REC_CNT; i++ ) {
            int valueInRange = i % mclRange;
            recs.add( ( BSONObject ) JSON
                    .parse( "{ i: " + i + ", a: " + valueInRange + " }" ) );
        }
        mcl.insert( recs, DBCollection.FLG_INSERT_CONTONDUP );
    }

    private void checkRemoved( Sequoiadb db ) {
        DBCollection mcl = db.getCollectionSpace( csName )
                .getCollection( mclName );
        long leftRecCnt = mcl.getCount();
        Assert.assertEquals( leftRecCnt, 0, "data is not removed" );
    }

    private void dropDomainAndCs( Sequoiadb db ) {
        db.dropCollectionSpace( csName );
        db.dropDomain( domainName );
    }
}