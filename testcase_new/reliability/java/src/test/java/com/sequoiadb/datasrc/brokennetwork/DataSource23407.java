package com.sequoiadb.datasrc.brokennetwork;

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
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-23407:使用数据源的集合空间执行数据操作过程中网络异常
 * @author liuli
 * @Date 2021.06.02
 * @version 1.10
 */

public class DataSource23407 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private GroupMgr groupMgr = null;
    private GroupMgr srcGroupMgr = null;
    private String dataSrcName = "datasource23407";

    private ArrayList< BSONObject > insertSuccessRecords = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > expInsertRecords = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > updateSuccessRecords = new ArrayList< BSONObject >();
    private String srcCSName = "cssrc_23407";
    private String csName = "cs_23407";
    private String clName = "cl_23407";

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
        DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, clName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName );
        CollectionSpace cs = sdb.createCollectionSpace( csName, options );
        DBCollection dbcl = cs.getCollection( clName );
        // 插入数据10000条记录，范围从60000~70000
        insertDatas( dbcl, 60000, 70000 );
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( DataSrcUtils.getSrcIp(), 10, 20 );
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

    private void checkResult() {
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );

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

        // 再次插入数据并校验
        ArrayList< BSONObject > expDocs = new ArrayList< BSONObject >();
        for ( int i = 0; i < 1000; i++ ) {
            BSONObject docs = new BasicBSONObject();
            docs.put( "check", i );
            docs.put( "test", "test_" + i );
            expDocs.add( docs );
        }
        dbcl.insert( expDocs );
        DataSrcUtils.checkRecords( dbcl, expDocs, "{check:{$exists:1}}",
                "{check:1}" );
    }

    private void insertDatas( DBCollection dbcl, int beginNo, int endNo ) {
        for ( int i = beginNo; i < endNo; i++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "no", i );
            record.put( "num", i );
            record.put( "test", "test_" + i );
            expInsertRecords.add( record );
        }
        dbcl.insert( expInsertRecords );
    }
}
