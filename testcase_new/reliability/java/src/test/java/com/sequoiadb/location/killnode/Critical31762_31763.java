package com.sequoiadb.location.killnode;

import java.util.List;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;

/**
 * @version 1.10
 * @Description seqDB-31762:指定Enforced为false，切主失败
 *              seqDB-31763:集群正常，指定Enforced为true强制切主
 * @Author liuli
 * @Date 2023.05.29
 * @UpdateAuthor liuli
 * @UpdateDate 2023.05.29
 */

@Test(groups = "location")
public class Critical31762_31763 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private GroupMgr groupMgr;
    private String csName = "cs_31762_31763";
    private String clName = "cl_31762_31763";
    private CollectionSpace dbcs = null;
    private int recordNum = 10000;
    private String groupName = null;
    private boolean startCriticalEnd = false;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupName = SdbTestBase.expandGroupNames.get( 0 );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness( 120, true, SdbTestBase.coordUrl ) ) {
            throw new SkipException( "checkBusiness return false" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        dbcs = sdb.createCollectionSpace( csName );
        dbcs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );

    }

    @Test
    public void test() throws Exception {
        // 停一个备节点
        ReplicaGroup group = sdb.getReplicaGroup( groupName );
        Node slaveNode = group.getSlave();
        String slaveNodeName = slaveNode.getHostName() + ":"
                + slaveNode.getPort();
        slaveNode.stop();

        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new Insert() );
        es.addWorker( new StartCritical( groupName, slaveNodeName ) );

        es.run();

        // 校验group主节点
        Node masterNode = group.getMaster();
        String masterNodeName = masterNode.getHostName() + ":"
                + masterNode.getPort();
        Assert.assertEquals( masterNodeName, slaveNodeName,
                "the primary node is not a node that initiates critical mode" );

        // 删除原集合，创建同名集合插入数据并校验
        dbcs.dropCollection( clName );
        DBCollection dbcl = dbcs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );
        List< BSONObject > batchRecords1 = CommLib.insertData( dbcl,
                recordNum );
        BasicBSONObject orderBy = new BasicBSONObject( "a", 1 );
        CommLib.checkRecords( dbcl, batchRecords1, orderBy );

        // 停止Critical模式
        group.stopCriticalMode();

        // 再次插入数据并校验
        dbcl.truncate();
        List< BSONObject > batchRecords2 = CommLib.insertData( dbcl,
                recordNum );
        CommLib.checkRecords( dbcl, batchRecords2, orderBy );
    }

    @AfterClass
    public void tearDown() throws ReliabilityException {
        sdb.getReplicaGroup( expandGroupNames.get( 0 ) ).stopCriticalMode();
        sdb.getReplicaGroup( expandGroupNames.get( 0 ) ).start();
        Assert.assertTrue(
                groupMgr.checkBusiness( 600, true, SdbTestBase.coordUrl ),
                "failed to restore business" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        if ( sdb != null ) {
            sdb.close();
        }
    }

    private class Insert extends ResultStore {

        @ExecuteOrder(step = 1)
        private void insert() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                while ( !startCriticalEnd ) {
                    CommLib.insertData( cl, recordNum );
                }
            }
        }
    }

    private class StartCritical extends ResultStore {
        private String groupName;
        private String nodeName;

        public StartCritical( String groupName, String nodeName ) {
            this.groupName = groupName;
            this.nodeName = nodeName;
        }

        @ExecuteOrder(step = 1)
        private void startCritical() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // sleep一段时间，使备节点数据落后
                try {
                    Thread.sleep( 30000 );
                } catch ( InterruptedException e ) {
                    throw new RuntimeException( e );
                }
                ReplicaGroup group = db.getReplicaGroup( groupName );
                Node node = group.getNode( nodeName );
                node.start();
                BasicBSONObject options = new BasicBSONObject();
                options.put( "MinKeepTime", 5 );
                options.put( "MaxKeepTime", 10 );
                options.put( "NodeName", nodeName );

                // 启动Critical模式，不指定Enforced
                try {
                    group.startCriticalMode( options );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_TIMEOUT
                            .getErrorCode() ) {
                        throw e;
                    }
                }

                // 启动Critical模式，指定Enforced为false
                options.put( "Enforced", false );
                try {
                    group.startCriticalMode( options );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_TIMEOUT
                            .getErrorCode() ) {
                        throw e;
                    }
                }

                // 启动Critical模式，指定Enforced为true
                options.put( "Enforced", true );
                try {
                    group.startCriticalMode( options );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_TIMEOUT
                            .getErrorCode() ) {
                        throw e;
                    }
                }
            } finally {
                startCriticalEnd = true;
            }
        }
    }
}
