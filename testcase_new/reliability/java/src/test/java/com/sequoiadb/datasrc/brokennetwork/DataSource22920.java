package com.sequoiadb.datasrc.brokennetwork;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

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
 * @Description seqDB-22920:删除数据源过程中网络异常
 * @author wuyan
 * @Date 2020.12.31
 * @version 1.10
 */

public class DataSource22920 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private GroupMgr srcGroupMgr = null;
    private String dataSrcName = "datasource22920";
    private int dataSrcNum = 100;
    private List< String > dataSrcNames = new ArrayList<>();
    private List< String > deleteSuccessNums = Collections
            .synchronizedList( new ArrayList< String >() );

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

        for ( int i = 0; i < dataSrcNum; i++ ) {
            String name = dataSrcName + "_" + i;
            if ( sdb.isDataSourceExist( name ) ) {
                sdb.dropDataSource( name );
            }
            DataSrcUtils.createDataSource( sdb, name );
        }
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( DataSrcUtils.getSrcIp(), 0, 10 );
        TaskMgr mgr = new TaskMgr( faultTask );
        for ( int i = 0; i < dataSrcNum; i++ ) {
            String name = dataSrcName + "_" + i;
            mgr.addTask( new DeleteDataSource( name ) );
            dataSrcNames.add( name );
        }

        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( srcGroupMgr.checkBusinessWithLSN( 600,
                DataSrcUtils.getSrcUrl() ), true );
        deleteDataSourceAgainAndCheckResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < dataSrcNum; i++ ) {
                String name = dataSrcName + "_" + i;
                if ( sdb.isDataSourceExist( name ) )
                    sdb.dropDataSource( name );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DeleteDataSource extends OperateTask {
        private String name;

        private DeleteDataSource( String name ) {
            this.name = name;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 删除数据源较快，随机等待5s内时间以验证在不同阶段断网
                int time = new java.util.Random().nextInt( 5000 );
                sleep( time );
                db.dropDataSource( name );
                deleteSuccessNums.add( name );
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

    private void deleteDataSourceAgainAndCheckResult() {
        dataSrcNames.removeAll( deleteSuccessNums );
        for ( int i = 0; i < dataSrcNames.size(); i++ ) {
            String name = dataSrcNames.get( i );
            sdb.dropDataSource( name );
        }

        // 验证删除后数据源是否存在
        for ( int i = 0; i < dataSrcNum; i++ ) {
            String name = dataSrcName + "_" + i;
            Assert.assertFalse( sdb.isDataSourceExist( name ),
                    name + "  exists!!" );
        }

    }
}
