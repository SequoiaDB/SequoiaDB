package com.sequoiadb.transaction.brokennetwork;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.commlib.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.task.OperateTask;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;
import com.sequoiadb.transaction.common.TransferTh;

/**
 * @descreption seqDB-26327:事务回滚过程中，主节点网络异常
 * @author ZhangYanan
 * @date 2022/04/06
 * @updateUser ZhangYanan
 * @updateDate 2022/04/06
 * @updateRemark
 * @version 1.0
 */

public class Transaction26327 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb safeSdb;
    private String clName = "cl26327";
    private GroupMgr groupMgr;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String clGroupName = null;
    private String safeCoordUrl = null;
    private int insertNum = 1000;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusinessWithLSN() ) {
            throw new SkipException( "checkBusiness failed" );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject options = new BasicBSONObject();
        clGroupName = groupMgr.getAllDataGroupName().get( 0 );
        options.put( "Group", clGroupName );
        cl = cs.createCollection( clName, options );
        cl.createIndex( "noIndex", new BasicBSONObject( "no", 1 ), true,
                false );
    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
        String dataHost = dataGroup.getMaster().hostName();
        // 获取安全的coordUrl
        safeCoordUrl = getSafeCoordUrl( sdb, dataHost );
        safeSdb = new Sequoiadb( safeCoordUrl, "", "" );
        cs = safeSdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.getCollection( clName );
        // 开始事务，插入数据
        safeSdb.beginTransaction();
        insertData( cl );

        FaultMakeTask faultTask = BrokenNetwork.getFaultMakeTask( dataHost, 0,
                10 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new threadRollback() );
        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 180 ), true );

        DBCursor cursor1 = cl.query( "", "", "", "" );
        ArrayList< BSONObject > queryList1 = TransUtil
                .getReadActList( cursor1 );
        // 校验数据是否被回滚
        Assert.assertEquals( queryList1.size(), 0 );
        // 开启事务再次插入数据
        safeSdb.beginTransaction();
        ArrayList< BSONObject > insertRecords = insertData( cl );
        safeSdb.commit();
        DBCursor cursor2 = cl.query( "", "", "", "" );
        ArrayList< BSONObject > queryList2 = TransUtil
                .getReadActList( cursor2 );
        Assert.assertEqualsNoOrder( insertRecords.toArray(),
                queryList2.toArray() );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) )
                cs.dropCollection( clName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( safeSdb != null ) {
                safeSdb.close();
            }
        }
    }

    private class threadRollback extends OperateTask {
        @Override
        public void exec() throws Exception {
            // 线程sleep5秒，确保能够与断网异常并发
            sleep( 5000 );
            safeSdb.rollback();
        }
    }

    public ArrayList< BSONObject > insertData( DBCollection dbcl ) {
        ArrayList< BSONObject > batchRecords = new ArrayList<>();
        for ( int i = 0; i < insertNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "no", i );
            obj.put( "a", i );
            batchRecords.add( obj );
        }
        dbcl.bulkInsert( batchRecords );
        return batchRecords;
    }

    public String getSafeCoordUrl( Sequoiadb sdb, String exceptionHost )
            throws ReliabilityException {
        String coordUrl = null;
        List< String > nodeAddress = CommLib.getNodeAddress( sdb, "SYSCoord" );
        for ( String nodeAddr : nodeAddress ) {
            String hostName = nodeAddr.split( ":" )[ 0 ];
            String svcName = nodeAddr.split( ":" )[ 1 ];
            if ( !hostName.equals( exceptionHost ) ) {
                coordUrl = hostName + ":" + svcName;
                break;
            }
        }
        return coordUrl;
    }
}
