package com.sequoiadb.datasrc.killnode;

import java.util.ArrayList;

import org.bson.BSONObject;
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
 * @Description seqDB-22925:使用数据源的集合执行数据操作过程中数据源节点异常
 * @author wuyan
 * @Date 2020.12.3
 * @version 1.10
 */

public class DataSource22925 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private GroupMgr groupMgr = null;
    private GroupMgr srcGroupMgr = null;
    private String dataSrcName = "datasource22925";

    private ArrayList< BSONObject > insertSuccessRecords = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > expInsertRecords = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > updateSuccessRecords = new ArrayList< BSONObject >();
    private String srcCSName = "cssrc_22925";
    private String csName = "cs_22925";
    private String clName = "cl_22925";

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
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
        DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, clName );
        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName + "." + clName );
        DBCollection dbcl = cs.createCollection( clName, options );
        // 插入数据10000条记录，范围从60000~70000
        insertDatas( dbcl, 60000, 70000 );
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                DataSrcUtils.getSrcIp(), DataSrcUtils.getSrcPort(), 10 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new InsertDatas() );
        mgr.addTask( new UpdateDatas() );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( srcGroupMgr.checkBusinessWithLSN( 600,
                DataSrcUtils.getSrcUrl() ), true );
        checkResult();
    }

    @AfterClass
    public void tearDown() {
        try {
            srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(),
                    DataSrcUtils.getUser(), DataSrcUtils.getPasswd() );
            srcdb.dropCollectionSpace( srcCSName );
            DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
    }

    private class InsertDatas extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                int count = 0;
                for ( int i = 0; i < 100; i++ ) {
                    ArrayList< BSONObject > records = new ArrayList<>();
                    for ( int j = 0; j < 500; j++ ) {
                        int value = count++;
                        BSONObject record = new BasicBSONObject();
                        record.put( "no", value );
                        record.put( "num", value );
                        record.put( "test", "test_" + value );
                        records.add( record );
                    }
                    dbcl.insert( records );
                    insertSuccessRecords.addAll( records );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
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

    private class UpdateDatas extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                int updateNum = 3000;
                int beginNo = 60000;
                for ( int i = beginNo; i < beginNo + updateNum; i++ ) {
                    String matcher = "{no:" + i + "}}";
                    String modifier = "{$set:{test:'updatetest" + i + "'}}";
                    dbcl.update( matcher, modifier, "" );
                    BSONObject obj = expInsertRecords.get( i - 60000 );
                    obj.put( "test", "updatetest" + i );
                    updateSuccessRecords.add( obj );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
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

    private void checkResult() {
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        // 验证异常前插入和更新数据信息正确
        for ( int i = 0; i < insertSuccessRecords.size(); i++ ) {
            BSONObject obj = insertSuccessRecords.get( i );
            long count = dbcl.getCount( obj );
            Assert.assertEquals( count, 1,
                    "find insert record is " + obj.toString() );
        }
        for ( int i = 0; i < updateSuccessRecords.size(); i++ ) {
            BSONObject obj = updateSuccessRecords.get( i );
            long count = dbcl.getCount( obj );
            Assert.assertEquals( count, 1,
                    "find update record is " + obj.toString() );
        }

        // 再次插入1000条记录，范围从70000~71000
        insertDatas( dbcl, 70000, 71000 );
        long expCount = 1000;
        String cond = "{$and:[{no:{$gte:" + 70000 + "}},{no:{$lt:" + 71000
                + "}}]}";
        long actCount = dbcl.getCount( cond );
        Assert.assertEquals( actCount, expCount );

    }

    private void insertDatas( DBCollection dbcl, int beginNo, int endNo ) {
        ArrayList< BSONObject > test = new ArrayList< BSONObject >();
        for ( int i = beginNo; i < endNo; i++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "no", i );
            record.put( "num", i );
            record.put( "test", "test_" + i );
            test.add( record );
        }
        dbcl.insert( test );
        expInsertRecords.addAll( test );
    }
}
