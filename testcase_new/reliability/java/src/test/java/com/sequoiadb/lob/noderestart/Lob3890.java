package com.sequoiadb.lob.noderestart;

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
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-5-11
 * @Version 1.00
 * @modify luweikang
 * @Date 2019-10-24
 */
public class Lob3890 extends SdbTestBase {
    private String csName = "cs3890";
    private String clName = "cl3890";
    private GroupMgr groupMgr = null;
    private String groupName1 = null;
    private String groupName2 = null;
    private Sequoiadb sdb = null;
    private DBCollection cl;
    private int writeLobSize = 1024 * 500;
    private byte[] lobBuff;
    private String safeCoordUrl;
    private List< ObjectId > readLobIds;
    private List< ObjectId > removeLobIds;
    private ObjectId lastOid;

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
        options.put( "Compressed", true );
        options.put( "CompressionType", "lzw" );
        cl = sdb.createCollectionSpace( csName ).createCollection( clName,
                options );
        cl.split( groupName1, groupName2, 50 );
        lobBuff = LobUtil.getRandomBytes( writeLobSize );
        readLobIds = LobUtil.createAndWriteLob( cl, lobBuff );
        removeLobIds = LobUtil.createAndWriteLob( cl, lobBuff );

        safeCoordUrl = CommLib.getSafeCoordUrl(
                groupMgr.getGroupByName( groupName1 ).getMaster().hostName() );
    }

    @Test
    public void test() throws ReliabilityException {

        // 建立并行任务
        TaskMgr mgr = new TaskMgr();
        mgr.addTask( new RestartRG( groupName1 ) );
        mgr.addTask( new RestartRG( groupName2 ) );
        mgr.addTask( new PutLob() );
        mgr.addTask( new ReadLob() );
        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        redoPutLob( cl, lastOid );

        List< ObjectId > lobIds1 = new ArrayList<>();
        DBCursor cur = cl.listLobs();
        while ( cur.hasNext() ) {
            BSONObject lobInfo = cur.getNext();
            ObjectId lobId = ( ObjectId ) lobInfo.get( "Oid" );
            lobIds1.add( lobId );
        }
        LobUtil.checkLobMD5( cl, lobIds1, lobBuff );

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

    class RestartRG extends OperateTask {

        private String groupName;

        public RestartRG( String groupName ) {
            this.groupName = groupName;
        }

        @Override
        public void exec() {
            try ( Sequoiadb db = new Sequoiadb( safeCoordUrl, "", "" )) {
                System.err.println( "stop group: " + groupName );
                try {
                    db.getReplicaGroup( this.groupName ).stop();
                } catch ( BaseException e ) {
                    // -160 停止节点超时，但节点最终仍然停止成功，捕获此错误码规避该错误。
                    if ( e.getErrorCode() != -160 ) {
                        throw e;
                    }
                }
                System.err.println( "start group: " + groupName );
                db.getReplicaGroup( this.groupName ).start();
            }
        }
    }

    class PutLob extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( safeCoordUrl, "", "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 100; i++ ) {
                    lastOid = null;
                    lastOid = dbcl.createLobID();
                    DBLob lob = dbcl.createLob( lastOid );
                    lob.write( lobBuff );
                    lob.close();
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

    class ReadLob extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( safeCoordUrl, "", "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 5; i++ ) {
                    LobUtil.checkLobMD5( dbcl, readLobIds, lobBuff );
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

    class RemoveLob extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( safeCoordUrl, "", "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( ObjectId oid : removeLobIds ) {
                    dbcl.removeLob( oid );
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

    private void redoPutLob( DBCollection cl, ObjectId lobOid ) {
        if ( lobOid != null ) {
            try {
                DBLob lob = cl.createLob( lobOid );
                lob.close();
            } catch ( BaseException e ) {
            }
            DBLob lob = cl.openLob( lobOid, DBLob.SDB_LOB_WRITE );
            lob.write( lobBuff );
            lob.close();
        } else {
            DBLob lob = cl.createLob();
            lob.write( lobBuff );
            lob.close();
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
