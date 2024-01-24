package com.sequoiadb.lob.killnode;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
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
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-7854:写lob过程中data节点异常
 * @author luweikang
 * @Date 2020.05.07
 * @version 1.00
 */
public class Lob7854 extends SdbTestBase {

    private String clName = "cl_7854";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private DBCollection cl;
    private int writeLobSize = 1024 * 1024;
    private byte[] lobBuff;
    private int lobNums = 100;
    private List< ObjectId > lobIds = new ArrayList<>();

    @BeforeClass
    public void setUp() throws ReliabilityException {
        // CheckBusiness(true),检测当前集群环境
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        CollectionSpace cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName, ( BSONObject ) JSON
                .parse( "{Group: '" + groupName + "', ReplSize: 0}" ) );
        lobBuff = LobUtil.getRandomBytes( writeLobSize );
    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();

        // 建立并行任务
        FaultMakeTask faultMakeTask = KillNode.getFaultMakeTask( master, 3 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );

        PutLob puLobTask = new PutLob();
        mgr.addTask( puLobTask );
        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        checkPutLobResult( cl );
        sdb.sync();
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.getCollectionSpace( csName ).dropCollection( clName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class PutLob extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < lobNums; i++ ) {
                    ObjectId lobId = putLob( cl );
                    lobIds.add( lobId );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -134 && e.getErrorCode() != -79
                        && e.getErrorCode() != -81 ) {
                    throw e;
                }
            }
        }
    }

    private ObjectId putLob( DBCollection dbcl ) {
        DBLob lob = dbcl.createLob();
        lob.write( lobBuff );
        lob.close();
        ObjectId lobId = lob.getID();
        return lobId;
    }

    private void checkPutLobResult( DBCollection dbcl ) {
        for ( ObjectId lobId : lobIds ) {
            byte[] data = new byte[ lobBuff.length ];
            DBLob rlob = null;
            try {
                rlob = dbcl.openLob( lobId );
                rlob.read( data );
            } finally {
                if ( rlob != null ) {
                    rlob.close();
                }
            }
        }

        checkLobNums( cl, lobIds );
        LobUtil.checkLobMD5( cl, lobIds, lobBuff );
        // 再次创建lob，读取lob信息正确
        ObjectId lobId = putLob( cl );
        List< ObjectId > lobIds2 = new ArrayList<>();
        lobIds2.add( lobId );
        LobUtil.checkLobMD5( cl, lobIds2, lobBuff );
    }

    private void checkLobNums( DBCollection dbcl,
            List< ObjectId > explobOids ) {
        List< ObjectId > actLobIds = new ArrayList<>();
        try ( DBCursor listLob = dbcl.listLobs()) {
            while ( listLob.hasNext() ) {
                BSONObject obj = listLob.getNext();
                ObjectId existOid = ( ObjectId ) obj.get( "Oid" );
                Boolean isAvailable = ( Boolean ) obj.get( "Available" );
                Assert.assertTrue( isAvailable,
                        "the lob oid is " + obj.toString() );
                actLobIds.add( existOid );
            }
        }
        // 验证lobId数量，实际数量比预期多且小于lobNums，则返回实际的lobId
        if ( actLobIds.size() >= explobOids.size()
                && actLobIds.size() <= lobNums ) {
            Assert.assertTrue( actLobIds.containsAll( explobOids ),
                    "actLobOid is :" + actLobIds.toString() + "\n expLobIds:"
                            + explobOids.toString() );
            lobIds = actLobIds;
        } else if ( actLobIds.size() > lobNums ) {
            Assert.fail(
                    "actual lobnum more than lobNums, lobId = " + actLobIds );
        }
    }
}
