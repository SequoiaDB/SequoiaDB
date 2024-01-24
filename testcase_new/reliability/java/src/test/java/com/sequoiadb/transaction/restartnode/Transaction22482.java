package com.sequoiadb.transaction.restartnode;

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

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;

/**
 * @Description seqDB-22482:事务未提交，重启主节点和删除cs
 * @author luweikang
 * @date 2020-7-23
 *
 */
@Test
public class Transaction22482 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb transDB;
    private String csName = "cs22482";
    private String clName = "cl22482";
    private GroupMgr groupMgr;
    private String groupName;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( coordUrl, "", "" );
        transDB = new Sequoiadb( coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "STANDALONE MODE" );
        }
        if ( !groupMgr.checkBusiness( 120 ) ) {
            throw new SkipException( "GROUP ERROR" );
        }
        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        sdb.createCollectionSpace( csName ).createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {

        // 开启事务插入大量记录
        transDB.beginTransaction();
        DBCollection cl = transDB.getCollectionSpace( csName )
                .getCollection( clName );
        insertData( cl );

        // 停止数据主节点
        sdb.getReplicaGroup( groupName ).getMaster().stop();

        waitGroupReelect( sdb );

        TaskMgr taskMgr = new TaskMgr();
        taskMgr.addTask( new DropCSTask() );
        taskMgr.start();

        sdb.getReplicaGroup( groupName ).getSlave().stop();

        Assert.assertTrue( taskMgr.isAllSuccess(), taskMgr.getErrorMsg() );

        sdb.getReplicaGroup( groupName ).start();

        waitTransRollback( sdb );

        Assert.assertTrue( groupMgr
                .checkBusinessWithLSN( TransUtil.ClusterRestoreTimeOut ) );
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( transDB != null ) {
                transDB.close();
            }
        }
    }

    private void insertData( DBCollection cl ) {
        List< BSONObject > datas = new ArrayList<>();
        for ( int i = 0; i < 200; i++ ) {
            for ( int j = 0; j < 10000; j++ ) {
                datas.add( new BasicBSONObject( "a", j ).append( "b",
                        "abcdefighoijklmnopqrst" ) );
            }
            cl.insert( datas );
            datas.clear();
        }
    }

    private void waitGroupReelect( Sequoiadb db ) throws InterruptedException {

        boolean hasPrimary = false;
        int times = 0;
        while ( !hasPrimary && times < 600 ) {
            DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_DATABASE,
                    "{RawData:true,IsPrimary:true,GroupName:'" + groupName
                            + "'}",
                    "", null );
            while ( cur.hasNext() ) {
                BasicBSONObject info = ( BasicBSONObject ) cur.getNext();
                if ( info.containsField( "IsPrimary" ) ) {
                    if ( info.getBoolean( "IsPrimary" ) ) {
                        hasPrimary = true;
                        break;
                    }
                }
            }
            cur.close();
            Thread.sleep( 100 );
            times++;
        }
        if ( times == 600 ) {
            Assert.fail( "no master after 60s." );
        }
    }

    private void waitTransRollback( Sequoiadb db ) throws InterruptedException {
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );
        for ( int i = 0; i < 6000; i++ ) {
            if ( cl.getCount() == 0 ) {
                break;
            }
            Thread.sleep( 100 );
            if ( i == 5999 ) {
                Assert.fail(
                        "the transaction has not been rollback after 600s." );
            }
        }

    }

    class DropCSTask extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( coordUrl, "", "" ) ;) {
                db.dropCollectionSpace( csName );
                Assert.fail( "drop cs must fail -104" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -104 && e.getErrorCode() != -191 ) {
                    throw e;
                }
            }
        }
    }
}
