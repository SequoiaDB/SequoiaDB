package com.sequoiadb.lob.networkfail;

import java.util.ArrayList;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-11437:事务中执行增删改记录操作和写lob操作，写lob过程中数据主节点所在主机断网
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 * @modify luweikang
 * @Date 2019-10-24
 */
public class Lob11437 extends SdbTestBase {
    private String csName = "cs11437";
    private String clName = "cl11437";
    private GroupMgr groupMgr = null;
    private String groupName1 = null;
    private String groupName2 = null;
    private Sequoiadb sdb = null;
    private DBCollection cl;
    private int writeLobSize = 1024 * 1024 * 10;
    private byte[] lobBuff;
    private String safeCoordUrl;

    @BeforeClass
    public void setUp() throws ReliabilityException {
        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 120 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }
        groupName1 = groupMgr.getAllDataGroupName().get( 0 );
        groupName2 = groupMgr.getAllDataGroupName().get( 1 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        BSONObject options = new BasicBSONObject();
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "ShardingType", "hash" );
        options.put( "Group", groupName1 );
        cl = sdb.createCollectionSpace( csName ).createCollection( clName,
                options );
        cl.split( groupName1, groupName2, 50 );
        lobBuff = LobUtil.getRandomBytes( writeLobSize );

        safeCoordUrl = CommLib.getSafeCoordUrl(
                groupMgr.getGroupByName( groupName1 ).getMaster().hostName() );
    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName1 );
        NodeWrapper dataMaster = dataGroup.getMaster();

        // 建立并行任务
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( dataMaster.hostName(), 0, 10 );
        TaskMgr mgr = new TaskMgr( faultTask );

        PutLob puLobTask = new PutLob();
        mgr.addTask( puLobTask );
        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        String match = "Name\\\\:" + csName + "\\\\." + clName;
        CommLib.waitContextClose( sdb, match, 300, true );

        List< ObjectId > lobIds1 = new ArrayList< ObjectId >();
        DBCursor cur = cl.listLobs();
        while ( cur.hasNext() ) {
            BSONObject lobInfo = cur.getNext();
            ObjectId lobId = ( ObjectId ) lobInfo.get( "Oid" );
            lobIds1.add( lobId );
        }
        LobUtil.checkLobMD5( cl, lobIds1, lobBuff );

        List< ObjectId > lobIds2 = LobUtil.createAndWriteLob( cl, lobBuff );
        LobUtil.checkLobMD5( cl, lobIds2, lobBuff );
        for ( ObjectId lobId : lobIds2 ) {
            cl.removeLob( lobId );
        }
        checkRemoveLobResult( lobIds2 );

        sdb.sync();
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    class PutLob extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( safeCoordUrl, "", "" )) {
                db.beginTransaction();
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                LobUtil.createAndWriteLob( dbcl, lobBuff );
                db.commit();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -104 && e.getErrorCode() != -134
                        && e.getErrorCode() != -79
                        && e.getErrorCode() != -81 ) {
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
