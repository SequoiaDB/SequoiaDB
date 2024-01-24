package com.sequoiadb.lob.killnode;

import java.util.Date;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.types.ObjectId;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
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
 * @Description seqDB-19067 主表进行lob操作过程中，其中一个子表主节点异常重启
 * @author luweikang
 * @date 2019年9月4日
 */
public class LobSubCL19067 extends SdbTestBase {
    private String csName = "cs_19067";
    private String mainCLName = "mainCL_19067";
    private String subCLName = "subCL_19067";
    private GroupMgr groupMgr = null;
    private String groupName1 = null;
    private String groupName2 = null;
    private Sequoiadb sdb = null;
    private DBCollection mainCL;
    private int writeLobSize = 1024 * 1024 * 10;
    private byte[] lobBuff;
    private List< ObjectId > lobIds;

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
        mainCL = createMainCLAndAttachCL();
        lobBuff = LobUtil.getRandomBytes( writeLobSize );
    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName1 );
        NodeWrapper dataMaster = dataGroup.getMaster();

        // 建立并行任务
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                dataMaster.hostName(), dataMaster.svcName(), 0 );
        TaskMgr mgr = new TaskMgr( faultTask );

        PutLob puLobTask = new PutLob();
        mgr.addTask( puLobTask );
        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        LobUtil.checkLobMD5( mainCL, lobIds, lobBuff );
        List< ObjectId > lobIds2 = LobUtil.createAndWriteLob( mainCL, lobBuff );
        LobUtil.checkLobMD5( mainCL, lobIds2, lobBuff );
        for ( ObjectId lobId : lobIds2 ) {
            mainCL.removeLob( lobId );
        }
        checkRemoveLobResult( lobIds2 );

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
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                lobIds = LobUtil.createAndWriteLob( mainCL, lobBuff );
            }
        }
    }

    private DBCollection createMainCLAndAttachCL() {
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        DBCollection mainCL = cs.createCollection( mainCLName, options );

        cs.createCollection( subCLName + "_1",
                new BasicBSONObject( "Group", groupName1 ) );
        cs.createCollection( subCLName + "_2",
                new BasicBSONObject( "Group", groupName2 ) );

        BSONObject bound1 = new BasicBSONObject();
        bound1.put( "LowBound", new BasicBSONObject( "date", new MinKey() ) );
        bound1.put( "UpBound", new BasicBSONObject( "date", "20000101" ) );
        mainCL.attachCollection( csName + "." + subCLName + "_1", bound1 );

        BSONObject bound2 = new BasicBSONObject();
        bound2.put( "LowBound", new BasicBSONObject( "date", "20000101" ) );
        bound2.put( "UpBound", new BasicBSONObject( "date", new MaxKey() ) );
        mainCL.attachCollection( csName + "." + subCLName + "_2", bound2 );

        return mainCL;
    }

    private void checkRemoveLobResult( List< ObjectId > lobIds ) {
        for ( ObjectId lobId : lobIds ) {
            try {
                mainCL.openLob( lobId );
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
