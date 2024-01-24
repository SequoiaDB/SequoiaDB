package com.sequoiadb.datasrc.killnode;

import java.util.ArrayList;
import java.util.List;

import org.bson.BasicBSONObject;
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
import com.sequoiadb.datasrc.DataSrcUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22917:创建数据源过程中编目主节点异常
 * @author wuyan
 * @Date 2021.1.2
 * @version 1.10
 */

public class DataSource22917 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private GroupMgr groupMgr = null;
    private GroupMgr srcGroupMgr = null;
    private String dataSrcName = "datasource22917";
    private int dataSrcNum = 50;
    private List< String > dataSrcNames = new ArrayList<>();
    private String srcCSName = "cssrc_22917";
    private String csName = "cs_22917";
    private String clName = "cl_22917";

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        groupMgr = GroupMgr.getInstance();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        srcGroupMgr = GroupMgr.getInstance( DataSrcUtils.getSrcUrl() );
        if ( !srcGroupMgr.checkBusiness( DataSrcUtils.getSrcUrl() ) ) {
            throw new SkipException( "checkBusiness failed" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) )
            sdb.dropCollectionSpace( csName );

        for ( int i = 0; i < dataSrcNum; i++ ) {
            String name = dataSrcName + "_" + i;
            if ( sdb.isDataSourceExist( name ) )
                sdb.dropDataSource( name );
        }
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, clName );
        sdb.createCollectionSpace( csName );

    }

    @Test
    public void test() throws Exception {
        GroupWrapper cataGroup = groupMgr.getGroupByName( "SYSCatalogGroup" );
        NodeWrapper priNode = cataGroup.getMaster();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( priNode.hostName(),
                priNode.svcName(), 0 );

        TaskMgr mgr = new TaskMgr( faultTask );
        for ( int i = 0; i < dataSrcNum; i++ ) {
            String name = dataSrcName + "_" + i;
            mgr.addTask( new CreateDataSource( name ) );
            dataSrcNames.add( name );
        }

        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( srcGroupMgr.checkBusinessWithLSN( 600,
                DataSrcUtils.getSrcUrl() ), true );
        createDataSourceAgainAndCheckResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            srcdb.dropCollectionSpace( srcCSName );
            sdb.dropCollectionSpace( csName );
            for ( int i = 0; i < dataSrcNum; i++ ) {
                String name = dataSrcNames.get( i );
                if ( sdb.isDataSourceExist( name ) )
                    sdb.dropDataSource( name );
            }

        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
    }

    private class CreateDataSource extends OperateTask {
        private String name;

        private CreateDataSource( String name ) {
            this.name = name;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject obj = new BasicBSONObject();
                // 创建数据源较快，随机等待3s内再创建
                int time = new java.util.Random().nextInt( 3000 );
                sleep( time );
                db.createDataSource( name, DataSrcUtils.getSrcUrl(),
                        DataSrcUtils.getUser(), DataSrcUtils.getPasswd(), "",
                        obj );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_INVALID_ROUTEID
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NODE_BSFAULT
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private void createDataSourceAgainAndCheckResult() {
        for ( int i = 0; i < dataSrcNames.size(); i++ ) {
            String name = dataSrcNames.get( i );
            if ( !sdb.isDataSourceExist( name ) ) {
                DataSrcUtils.createDataSource( sdb, name );
            }
            // 验证创建的数据源信息，检查是否可用
            BasicBSONObject options = new BasicBSONObject();
            options.put( "DataSource", name );
            options.put( "Mapping", srcCSName + "." + clName );
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            DBCollection cl = cs.createCollection( clName + "_" + i, options );
            String record = "{a:" + i + "}";
            cl.insert( record );
            long count = cl.getCount( record );
            Assert.assertEquals( count, 1, "act record is " + record );
        }

    }
}
