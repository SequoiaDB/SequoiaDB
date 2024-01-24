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

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
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
 * @Description seqDB-19062 主表挂载子表过程中，编目主节点网络故障
 * @author luweikang
 * @date 2019年9月4日
 */
public class LobSubCL19062 extends SdbTestBase {
    private String csName = "cs_19062";
    private String mainCLName = "mainCL_19062";
    private String subCLName = "subCL_19062";
    private int clNum = 100;
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private DBCollection mainCL;
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

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        mainCL = createMainCL();
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        for ( int i = 0; i < clNum; i++ ) {
            cs.createCollection( subCLName + "_" + i );
        }
        lobBuff = LobUtil.getRandomBytes( writeLobSize );
    }

    @Test
    public void test() throws ReliabilityException {
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        NodeWrapper cataMaster = cataGroup.getMaster();

        safeCoordUrl = CommLib.getSafeCoordUrl( cataMaster.hostName() );

        // 建立并行任务
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( cataMaster.hostName(), 0, 10 );
        TaskMgr mgr = new TaskMgr( faultTask );

        AttachCL attchCLTask = new AttachCL();
        mgr.addTask( attchCLTask );
        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );
        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );

        String match = "Name\\\\\": \\\\\"" + csName + ".";
        CommLib.waitContextClose( sdb, match, 300, false );

        redoAttachCL();
        List< ObjectId > lobIds = new ArrayList<>();
        // 插入lob时有可能其他连接会刷新catalogInfo，重试一次插入
        lobIds.addAll( LobUtil.createAndWriteLob( mainCL, lobBuff, "YYYYMMDD",
                25, 1, "20190101" ) );
        lobIds.addAll( LobUtil.createAndWriteLob( mainCL, lobBuff, "YYYYMMDD",
                25, 1, "20190201" ) );
        lobIds.addAll( LobUtil.createAndWriteLob( mainCL, lobBuff, "YYYYMMDD",
                25, 1, "20190301" ) );
        lobIds.addAll( LobUtil.createAndWriteLob( mainCL, lobBuff, "YYYYMMDD",
                25, 1, "20190401" ) );
        LobUtil.checkLobMD5( mainCL, lobIds, lobBuff );

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

    class AttachCL extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( safeCoordUrl, "", "" )) {
                DBCollection mainCL = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                int date = 20190101;
                int k = 0;
                for ( int i = 0; i < clNum; i++ ) {
                    if ( i != 0 && i % 25 == 0 ) {
                        date = date + 100;
                        k = 0;
                    }
                    BSONObject bound = new BasicBSONObject();
                    bound.put( "LowBound",
                            new BasicBSONObject( "date", ( date + k ) + "" ) );
                    bound.put( "UpBound", new BasicBSONObject( "date",
                            ( date + 1 + k ) + "" ) );
                    mainCL.attachCollection( csName + "." + subCLName + "_" + i,
                            bound );
                    k++;
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

    private DBCollection createMainCL() {
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        DBCollection mainCL = cs.createCollection( mainCLName, options );

        return mainCL;
    }

    private void redoAttachCL() {
        int k = 0;
        int date = 20190101;
        for ( int i = 0; i < clNum; i++ ) {
            try {
                if ( i != 0 && i % 25 == 0 ) {
                    date = date + 100;
                    k = 0;
                }
                BSONObject bound = new BasicBSONObject();
                bound.put( "LowBound",
                        new BasicBSONObject( "date", ( date + k ) + "" ) );
                bound.put( "UpBound",
                        new BasicBSONObject( "date", ( date + 1 + k ) + "" ) );
                mainCL.attachCollection( csName + "." + subCLName + "_" + i,
                        bound );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -235 ) {
                    e.printStackTrace();
                    throw e;
                }
            }
            k++;
        }

    }

}
