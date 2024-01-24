package com.sequoiadb.lob.noderestart;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
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
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName 测试用例 seqDB-3885 :: 版本: 2 :: 切分表写lob时备节点正常重启
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 * @modify luweikang
 * @Date 2019-10-24
 */
public class Lob3885 extends SdbTestBase {
    private String csName = "cs3885";
    private String clName = "cl3885";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private DBCollection cl;
    private int writeLobSize = 1024 * 500;
    private byte[] lobBuff;
    private List< ObjectId > lobIds;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 120 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }
        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cl = sdb.createCollectionSpace( csName ).createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );
        lobBuff = LobUtil.getRandomBytes( writeLobSize );
        lobIds = LobUtil.createAndWriteLob( cl, lobBuff );

    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper slaveMaster = dataGroup.getSlave();

        // 建立并行任务
        FaultMakeTask faultTask = NodeRestart.getFaultMakeTask( slaveMaster, 1,
                10 );
        TaskMgr mgr = new TaskMgr( faultTask );

        mgr.addTask( new PutLob() );
        mgr.addTask( new PutLob() );
        mgr.addTask( new PutLob() );
        mgr.addTask( new ReadLob() );
        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        List< ObjectId > lobIds1 = new ArrayList< ObjectId >();
        DBCursor cur = cl.listLobs();
        while ( cur.hasNext() ) {
            BSONObject lobInfo = cur.getNext();
            ObjectId lobId = ( ObjectId ) lobInfo.get( "Oid" );
            lobIds1.add( lobId );
        }
        LobUtil.checkLobMD5( cl, lobIds1, lobBuff );

        Assert.assertEquals( lobIds1.size(), 320, "check lob num" );

        for ( ObjectId lobId : lobIds1 ) {
            cl.removeLob( lobId );
        }
        checkRemoveLobResult( lobIds1 );

        sdb.sync();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    class PutLob extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 100; i++ ) {
                    DBLob lob = dbcl.createLob();
                    lob.write( lobBuff );
                    lob.close();
                }
            }
        }
    }

    class ReadLob extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 5; i++ ) {
                    LobUtil.checkLobMD5( dbcl, lobIds, lobBuff );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -104 && e.getErrorCode() != -134
                        && e.getErrorCode() != -79 && e.getErrorCode() != -81
                        && e.getErrorCode() != -17 && e.getErrorCode() != -36 ) {
                    e.printStackTrace();
                    throw e;
                }
            }
        }
    }

    private void checkRemoveLobResult( List< ObjectId > lobIds ) {
        for ( ObjectId lobId : lobIds ) {
            try {
                cl.openLob( lobId );
                Assert.fail( "the lob: " + lobId
                        + " has been deleted and the read should fail" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -4 ) {
                    throw e;
                }
            }
        }
    }
}
