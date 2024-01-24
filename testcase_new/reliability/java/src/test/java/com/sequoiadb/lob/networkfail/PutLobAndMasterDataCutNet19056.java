package com.sequoiadb.lob.networkfail;

import java.util.ArrayList;
import java.util.Collections;
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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
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
 * @Description seqDB-19056:主子表插入lob过程中，子表所在数据组的主节点网络故障
 * @author wuyan
 * @Date 2019.9.10
 * @version 1.00
 */
public class PutLobAndMasterDataCutNet19056 extends SdbTestBase {
    private String csName = "cs_19056";
    private String mainCLName = "mainCL_19056";
    private String subCLName = "subCL_19056";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private DBCollection mainCL;
    private int writeLobSize = 1024 * 1024 * 10;
    private byte[] lobBuff;
    private int lobNums = 20;
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
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        mainCL = createMainCLAndAttachCL();
        lobBuff = LobUtil.getRandomBytes( writeLobSize );
    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();

        FaultMakeTask faultMakeTask = BrokenNetwork
                .getFaultMakeTask( master.hostName(), 0, 10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );

        PutLob puLobTask = new PutLob();
        mgr.addTask( puLobTask );
        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        String match = "Name\\\\:" + csName + "\\\\.";
        CommLib.waitContextClose( sdb, match, 300, true );

        checkPutLobResult( mainCL );
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

    private class PutLob extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                for ( int i = 0; i < lobNums; i++ ) {
                    System.out.println( "---begin to put " + i );
                    ObjectId lobId = putLob( mainCL );
                    lobIds.add( lobId );
                    System.out.println( "---end to put " + i );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -81 && e.getErrorCode() != -79
                        && e.getErrorCode() != -134
                        && e.getErrorCode() != -15 ) {
                    throw e;
                }
            }
        }
    }

    private DBCollection createMainCLAndAttachCL() {
        CollectionSpace cs = null;
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            cs = sdb.getCollectionSpace( csName );
        } else {
            cs = sdb.createCollectionSpace( csName );
        }

        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        DBCollection mainCL = cs.createCollection( mainCLName, options );

        BSONObject clOptions = new BasicBSONObject();
        clOptions.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        clOptions.put( "ShardingType", "hash" );
        clOptions.put( "Group", groupName );
        cs.createCollection( subCLName, clOptions );

        BSONObject bound = new BasicBSONObject();
        bound.put( "LowBound", new BasicBSONObject( "date", new MinKey() ) );
        bound.put( "UpBound", new BasicBSONObject( "date", new MaxKey() ) );
        mainCL.attachCollection( csName + "." + subCLName, bound );

        return mainCL;
    }

    private ObjectId putLob( DBCollection dbcl ) {
        DBLob lob = dbcl.createLob();
        lob.write( lobBuff );
        lob.close();
        ObjectId lobId = lob.getID();
        return lobId;
    }

    private void checkPutLobResult( DBCollection dbcl ) {
        // 检查故障前创建lob结果
        LobUtil.checkLobMD5( mainCL, lobIds, lobBuff );
        checkLobNums( mainCL, lobIds );
        // 再次创建lob，读取lob信息正确
        ObjectId lobId = putLob( mainCL );
        List< ObjectId > lobIds2 = new ArrayList<>();
        lobIds2.add( lobId );
        LobUtil.checkLobMD5( mainCL, lobIds2, lobBuff );
    }

    private void checkLobNums( DBCollection dbcl, List< ObjectId > lobOids ) {
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
        Collections.sort( actLobIds );
        Collections.sort( lobOids );
        Assert.assertEquals( actLobIds, lobOids, "actLobOid is :"
                + actLobIds.toString() + "\n expLobIds:" + lobOids.toString() );
    }
}
