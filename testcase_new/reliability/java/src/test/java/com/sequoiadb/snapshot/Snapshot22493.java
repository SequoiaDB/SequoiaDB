package com.sequoiadb.snapshot;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.OperateTask;

/**
 * @Description: seqDB-22493:同步策略为keepall情况下停止备节点并持续向集合中插入数据，检查会话快照中IsBlocked和Doing字段信息
 * @Author Zhao Xiaoni
 * @Date 2020.7.30
 */
public class Snapshot22493 extends SdbTestBase {
    private Sequoiadb sdb;
    private GroupMgr groupMgr;
    private String groupName;
    private String lobSb;
    private String clName = "cl_22493";
    private int times = 0;
    private int totalTimes = 300;
    private static boolean isSuccess = false;

    @BeforeClass
    public void setup() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusiness return false" );
        }

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        lobSb = LobUtil.getRandomString( 1024 * 1024 * 10 );
        groupName = groupMgr.getAllDataGroupName().get( 0 );
        sdb.getCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse(
                        "{ 'ReplSize': 1, Group: '" + groupName + "' }" ) );
    }

    @Test
    public void test() throws Exception {
        try {
            sdb.updateConfig( ( BSONObject ) JSON
                    .parse( "{ 'syncstrategy': 'keepall' }" ) );
            sdb.getReplicaGroup( groupName ).getSlave().stop();

            WriteLob writeLob = new WriteLob();
            writeLob.start();

            do {
                Thread.sleep( 100 );
                try ( DBCursor cursor = sdb.getSnapshot(
                        Sequoiadb.SDB_SNAP_SESSIONS,
                        "{ 'NodeSelect': 'master', "
                                + "'IsBlocked': true, 'Doing': 'Waiting for sync control' }",
                        null, null )) {
                    if ( cursor.hasNext() ) {
                        isSuccess = true;
                        break;
                    }
                }
            } while ( times < totalTimes );

            Assert.assertTrue( writeLob.isSuccess() );

            sdb.getReplicaGroup( groupName ).start();
            Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
        } catch ( Exception e ) {
            e.printStackTrace();
            throw e;
        }
    }

    public class WriteLob extends OperateTask {
        DBCollection cl = null;

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                do {
                    times++;
                    DBLob lob = cl.createLob();
                    lob.write( lobSb.getBytes() );
                    lob.close();
                } while ( !isSuccess && times < totalTimes );
                if ( times >= totalTimes ) {
                    Assert.fail( "Insert time out!" );
                }
            }
        }
    }

    @AfterClass
    public void tearDown() {
        sdb.deleteConfig( ( BSONObject ) JSON.parse( "{ 'syncstrategy': 1 }" ),
                ( BSONObject ) JSON.parse( "{Global:true}" ) );
        sdb.getCollectionSpace( csName ).dropCollection( clName );
        sdb.close();
    }
}
