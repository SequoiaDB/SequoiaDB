package com.sequoiadb.datasrc.brokennetwork;

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
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasrc.DataSrcUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22916:创建数据源过程中网络异常
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22916 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private GroupMgr groupMgr = null;
    private GroupMgr srcGroupMgr = null;
    private String dataSrcName = "datasource22916";
    private int dataSrcNum = 20;
    private List< String > dataSrcNames = new ArrayList<>();
    private String srcCSName = "cssrc_22916";
    private String csName = "cs_22916";
    private String clName = "cl_22916";

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }
        groupMgr = GroupMgr.getInstance();
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
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, clName );
        sdb.createCollectionSpace( csName );
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( DataSrcUtils.getSrcIp(), 0, 20 );
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
                // 创建数据源较快，随机等待10s内时间以验证在不同阶段断网
                int time = new java.util.Random().nextInt( 10000 );
                sleep( time );
                db.createDataSource( name, DataSrcUtils.getSrcUrl(),
                        DataSrcUtils.getUser(), DataSrcUtils.getPasswd(), "",
                        obj );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NOT_CONNECTED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK_CLOSE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
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
