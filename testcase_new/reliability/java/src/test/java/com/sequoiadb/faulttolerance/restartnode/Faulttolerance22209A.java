package com.sequoiadb.faulttolerance.restartnode;

import java.util.ArrayList;

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
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22209 所有备节点未完全同步主节点的lsn，正常停止主节点，任意一个备节点跟上主节点lsn后，主节点停止成功
                该用例与 22209B 的区别在于参数：shutdownwaittimeout 设置为 120
                此用例禁用，原因是主备同步速度极快 1s/512MB 需要长时间持续插入数据拉大 LSN 才能构造出合适场景
 * @author luweikang
 * @date 2020年6月5日
 */
public class Faulttolerance22209A extends SdbTestBase {

    private String csName = "cs22209A";
    private String clName = "cl22209A_";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private int clNum = 10;
    private boolean shutoff = false;

    @BeforeClass
    public void setUp() throws ReliabilityException {

        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }

        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        cs = sdb.createCollectionSpace( csName );
        for ( int i = 0; i < clNum; i++ ) {
            cs.createCollection( clName + i, ( BSONObject ) JSON
                    .parse( "{'Group': '" + groupName + "'}" ) );
        }

        BSONObject config = new BasicBSONObject();
        config.put( "shutdownwaittimeout", 120 );
        sdb.updateConfig( config,
                new BasicBSONObject( "GroupName", groupName ) );

    }

    @Test(enabled = false)
    public void test() throws ReliabilityException, InterruptedException {

        TaskMgr mgr = new TaskMgr();
        for ( int i = 0; i < clNum; i++ ) {
            mgr.addTask( new Insert( clName + i ) );
            mgr.addTask( new Update( clName + i ) );
        }
        mgr.addTask( new TestStopNode() );

        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        sdb.getReplicaGroup( groupName ).start();

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            BSONObject config = new BasicBSONObject();
            config.put( "shutdownwaittimeout", 1 );
            sdb.deleteConfig( config, new BasicBSONObject() );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }

        }
    }

    class Insert extends OperateTask {
        private String clName = null;

        public Insert( String clName ) {
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( this.clName );
                for ( int i = 0; i < 1000; i++ ) {
                    if ( shutoff ) {
                        break;
                    }
                    ArrayList< BSONObject > records = new ArrayList<>();
                    for ( int j = 0; j < 5000; j++ ) {
                        BSONObject record = new BasicBSONObject();
                        record.put( "a", j );
                        record.put( "b", j );
                        record.put( "order", j );
                        record.put( "str",
                                "fjsldkfjlksdjflsdljfhjdshfjksdhfsdfhsdjkfhjkdshfj"
                                        + "kdshfkjdshfkjsdhfkjshafdkhasdikuhsdjfls"
                                        + "hsdjkfhjskdhfkjsdhfjkdshfjkdshfkjhsdjkf"
                                        + "hsdkjfhsdsafnweuhfuiwnqefiuokdjf" );
                        records.add( record );
                    }
                    cl.insert( records );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -17 && e.getErrorCode() != -134
                        && e.getErrorCode() != -104 ) {
                    throw e;
                }
            }
        }
    }

    class Update extends OperateTask {
        private String clName = null;

        public Update( String clName ) {
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( this.clName );
                for ( int i = 0; i < 1000; i++ ) {
                    if ( shutoff ) {
                        break;
                    }
                    cl.update( null,
                            "{$inc:{a:1, b:1}, $set:{'str':'update str times "
                                    + i + "'}}",
                            null );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -17 && e.getErrorCode() != -134
                        && e.getErrorCode() != -104 ) {
                    throw e;
                }
            }
        }
    }

    class TestStopNode extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                long masterLsn = 0;
                long slaveLsn = 0;
                for ( int i = 0; i < 6000; i++ ) {
                    DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                            "{GroupName:'" + groupName + "', RawData: true}",
                            null, null );
                    while ( cur.hasNext() ) {
                        BSONObject obj = cur.getNext();
                        BSONObject currentLSN = ( BSONObject ) obj
                                .get( "CurrentLSN" );
                        if ( ( boolean ) obj.get( "IsPrimary" ) ) {
                            masterLsn = ( long ) currentLSN.get( "Offset" );
                        } else {
                            slaveLsn = ( long ) currentLSN.get( "Offset" );
                        }
                    }
                    long differ = masterLsn - slaveLsn;

                    if ( differ >= 134217728 ) {
                        db.getReplicaGroup( groupName ).getMaster().stop();
                        break;
                    } else {
                        Thread.sleep( 100 );
                    }
                    if ( i == 5999 ) {
                        Assert.fail( "600 seconds lsn still no distance." );
                    }
                }
            } finally {
                shutoff = true;
            }
        }
    }

}
