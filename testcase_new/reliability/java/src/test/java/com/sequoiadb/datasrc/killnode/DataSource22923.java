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
 * @Description seqDB-22923:创建使用数据源的集合空间/集合过程中数据源节点故障
 * @author wuyan
 * @Date 2021.1.3
 * @version 1.10
 */

public class DataSource22923 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private GroupMgr groupMgr = null;
    private GroupMgr srcGroupMgr = null;
    private String dataSrcName = "datasource22923";
    private int num = 20;
    private List< String > csNames = new ArrayList<>();
    private List< String > clNames = new ArrayList<>();
    private List< String > createSuccessCSNames = new ArrayList<>();
    private List< String > createSuccessCLNames = new ArrayList<>();
    private String srcCSName = "cssrc_22923";
    private String srcCLName = "clsrc_22923";
    private String csName = "cs_22923";
    private String clName = "cl_22923";
    private BasicBSONObject csoptions = new BasicBSONObject();
    private BasicBSONObject cloptions = new BasicBSONObject();

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

        for ( int i = 0; i < num; i++ ) {
            String name = csName + "_" + i;
            if ( sdb.isCollectionSpaceExist( name ) )
                sdb.dropCollectionSpace( name );
        }
        if ( sdb.isDataSourceExist( dataSrcName ) ) {
            sdb.dropDataSource( dataSrcName );
        }
        DataSrcUtils.createDataSource( sdb, dataSrcName );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, srcCLName );

        sdb.createCollectionSpace( csName );

        csoptions.put( "DataSource", dataSrcName );
        csoptions.put( "Mapping", srcCSName );
        cloptions.put( "DataSource", dataSrcName );
        cloptions.put( "Mapping", srcCSName + "." + srcCLName );
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                DataSrcUtils.getSrcIp(), DataSrcUtils.getSrcPort(), 0 );
        TaskMgr mgr = new TaskMgr( faultTask );
        for ( int i = 0; i < num; i++ ) {
            String subCSName = csName + "_" + i;
            String subCLName = clName + "_" + i;
            mgr.addTask( new CreateCS( subCSName ) );
            mgr.addTask( new CreateCL( subCLName ) );
            csNames.add( csName + "_" + i );
            clNames.add( clName + "_" + i );
        }

        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( srcGroupMgr.checkBusinessWithLSN( 600,
                DataSrcUtils.getSrcUrl() ), true );
        createCSCLAgainAndCheckResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(),
                    DataSrcUtils.getUser(), DataSrcUtils.getPasswd() );
            srcdb.dropCollectionSpace( srcCSName );
            sdb.dropCollectionSpace( csName );
            for ( int i = 0; i < num; i++ ) {
                String subCSName = csName + "_" + i;
                sdb.dropCollectionSpace( subCSName );
            }
            sdb.dropDataSource( dataSrcName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
    }

    private class CreateCS extends OperateTask {
        private String name;

        private CreateCS( String name ) {
            this.name = name;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 创建数据源较快，随机等待10s内时间以验证在不同阶段断网
                int time = new java.util.Random().nextInt( 10000 );
                sleep( time );
                db.createCollectionSpace( name, csoptions );
                createSuccessCSNames.add( name );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK_CLOSE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NODE_BSFAULT
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class CreateCL extends OperateTask {
        private String name;

        private CreateCL( String name ) {
            this.name = name;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                // 创建数据源较快，随机等待10s内时间以验证在不同阶段断网
                int time = new java.util.Random().nextInt( 10000 );
                sleep( time );
                cs.createCollection( name, cloptions );
                createSuccessCLNames.add( name );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NOT_CONNECTED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK_CLOSE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NODE_BSFAULT
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private void createCSCLAgainAndCheckResult() {
        csNames.removeAll( createSuccessCSNames );
        for ( int i = 0; i < csNames.size(); i++ ) {
            String name = csNames.get( i );
            if ( !sdb.isCollectionSpaceExist( name ) ) {
                sdb.createCollectionSpace( name, csoptions );
            }
        }

        clNames.removeAll( createSuccessCLNames );
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        for ( int i = 0; i < clNames.size(); i++ ) {
            String name = clNames.get( i );
            if ( !cs.isCollectionExist( name ) ) {
                cs.createCollection( name, cloptions );
            }
        }

        for ( int i = 0; i < num; i++ ) {
            // 使用创建的集合空间执行数据操作
            CollectionSpace cSpace = sdb.getCollectionSpace( csName + "_" + i );
            DBCollection cl = cSpace.getCollection( srcCLName );
            String record = "{a:" + i + "}";
            cl.insert( record );
            long count = cl.getCount( record );
            Assert.assertEquals( count, 1, "act record is " + record );

            // 使用创建的集合执行数据操作
            CollectionSpace cSpace1 = sdb.getCollectionSpace( csName );
            DBCollection cl1 = cSpace1.getCollection( clName + "_" + i );
            String record1 = "{b:" + i + "}";
            cl1.insert( record1 );
            long count1 = cl1.getCount( record1 );
            Assert.assertEquals( count1, 1, "act record1 is " + record1 );
        }

    }
}
