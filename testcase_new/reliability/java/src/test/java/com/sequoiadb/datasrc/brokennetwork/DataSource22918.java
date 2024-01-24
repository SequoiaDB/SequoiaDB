package com.sequoiadb.datasrc.brokennetwork;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DataSource;
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
 * @Description seqDB-22918:修改数据源过程中网络异常
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22918 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private GroupMgr groupMgr = null;
    private GroupMgr srcGroupMgr = null;
    private String dataSrcName = "datasource22918";
    private int dataSrcNum = 50;
    private List< String > dataSrcNames = new ArrayList<>();
    private List< String > dataSrcNewNames = new ArrayList<>();
    private List< String > alterSuccessNums = Collections
            .synchronizedList( new ArrayList< String >() );
    private String srcCSName = "cssrc_22918";
    private String csName = "cs_22918";
    private String clName = "cl_22918";

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

        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, clName );
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        for ( int i = 0; i < dataSrcNum; i++ ) {
            String name = dataSrcName + "_" + i;
            if ( sdb.isDataSourceExist( name ) ) {
                sdb.dropDataSource( name );
            }
            DataSrcUtils.createDataSource( sdb, name );

            // 创建cl使用数据源
            BasicBSONObject options = new BasicBSONObject();
            options.put( "DataSource", name );
            options.put( "Mapping", srcCSName + "." + clName );
            DBCollection cl = cs.createCollection( clName + "_" + i, options );
            cl.insert( "{a:" + i + "}" );
        }
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( DataSrcUtils.getSrcIp(), 0, 20 );
        TaskMgr mgr = new TaskMgr( faultTask );
        for ( int i = 0; i < dataSrcNum; i++ ) {
            String name = dataSrcName + "_" + i;
            String newName = dataSrcName + "_new" + i;
            mgr.addTask( new AlterDataSource( name, newName ) );
            dataSrcNames.add( name );
        }

        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( srcGroupMgr.checkBusinessWithLSN( 600,
                DataSrcUtils.getSrcUrl() ), true );
        alterDataSourceAgainAndCheckResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            srcdb.dropCollectionSpace( srcCSName );
            sdb.dropCollectionSpace( csName );
            for ( int i = 0; i < dataSrcNum; i++ ) {
                String name = dataSrcNewNames.get( i );
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

    private class AlterDataSource extends OperateTask {
        private String name;
        private String newName;

        private AlterDataSource( String name, String newName ) {
            this.name = name;
            this.newName = newName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BasicBSONObject obj = new BasicBSONObject();
                obj.put( "Name", newName );
                DataSource getDataSrc = db.getDataSource( name );
                // 修改数据源较快，随机等待1s内时间以验证在不同阶段断网
                int time = new java.util.Random().nextInt( 1000 );
                sleep( time );
                getDataSrc.alterDataSource( obj );
                alterSuccessNums.add( name );
                dataSrcNewNames.add( newName );
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

    private void alterDataSourceAgainAndCheckResult() {
        dataSrcNames.removeAll( alterSuccessNums );
        for ( int i = 0; i < dataSrcNames.size(); i++ ) {
            String name = dataSrcNames.get( i );
            String newName = dataSrcName + "_alterAgain_" + i;
            BasicBSONObject obj = new BasicBSONObject();
            obj.put( "Name", newName );
            DataSource getDataSrc = sdb.getDataSource( name );
            getDataSrc.alterDataSource( obj );
            dataSrcNewNames.add( newName );
        }

        // 验证修改后数据源是否可用
        for ( int i = 0; i < dataSrcNewNames.size(); i++ ) {
            DBCollection cl = sdb.getCollectionSpace( csName )
                    .getCollection( clName + "_" + i );
            String queryCond = "{a:" + i + "}";
            long count = cl.getCount( queryCond );
            Assert.assertEquals( count, 1, "act record is " + queryCond );
            // 插入新记录
            String record = "{a:" + "'test_" + i + "'}";
            cl.insert( record );
            long count1 = cl.getCount( record );
            // bug:count功能未修复
            Assert.assertEquals( count1, 1, "act record is " + record );
        }
    }
}
