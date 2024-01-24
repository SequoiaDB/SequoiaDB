package com.sequoiadb.datasrc;

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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-23405:使用数据源创建集合空间并发执行数据操作
 * @author liuli
 * @Date 2021.02.23
 * @version 1.10
 */

public class DataSource23405 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasource23405";
    private String csName = "cs_23405";
    private String srcCSName = "cssrc_23405";
    private String clName = "cl_23405";
    private CollectionSpace dbcs = null;
    private DBCollection dbcl = null;
    private ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
    private ArrayList< Integer > removeNums = new ArrayList< Integer >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, clName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName );
        dbcs = sdb.createCollectionSpace( csName, options );
        dbcl = dbcs.getCollection( clName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        InsertOperation insertOperation = new InsertOperation();
        QueryOperation queryOperation = new QueryOperation();
        UpsertOperation upsertOperation = new UpsertOperation();
        RemoveOperation removeOperation = new RemoveOperation();
        es.addWorker( insertOperation );
        es.addWorker( queryOperation );
        es.addWorker( upsertOperation );
        es.addWorker( removeOperation );
        es.run();

        Assert.assertEquals( insertOperation.getRetCode(), 0 );
        Assert.assertEquals( upsertOperation.getRetCode(), 0 );
        Assert.assertEquals( removeOperation.getRetCode(), 0 );
        queryAndCheck( dbcl, insertRecords, removeNums );
    }

    @AfterClass
    public void tearDown() {
        try {
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

    private class InsertOperation extends ResultStore {

        @ExecuteOrder(step = 1)
        private void insert() {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 2000; i++ ) {
                    BSONObject obj = new BasicBSONObject();
                    obj.put( "_id", i );
                    obj.put( "a", i );
                    obj.put( "num", i );
                    insertRecords.add( obj );
                }
                dbcl.insert( insertRecords );
            }
        }
    }

    private class UpsertOperation extends ResultStore {

        @ExecuteOrder(step = 1)
        private void upsert() {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 2000; i = i + 5 ) {
                    DBCursor cur = dbcl.queryAndUpdate(
                            new BasicBSONObject( "num", i ), null, null, null,
                            new BasicBSONObject( "$set",
                                    new BasicBSONObject( "a", "b" + i ) ),
                            0, -1, 0, true );
                    if ( cur.getNext() != null ) {
                        BSONObject obj = new BasicBSONObject();
                        obj.put( "_id", i );
                        obj.put( "a", "b" + i );
                        obj.put( "num", i );
                        insertRecords.set( i, obj );
                    }
                    cur.close();
                }
            }
        }
    }

    private class QueryOperation extends ResultStore {
        private ArrayList< BSONObject > queryRecords = new ArrayList< BSONObject >();

        @ExecuteOrder(step = 1)
        private void query() {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 500; i++ ) {
                    BSONObject obj = new BasicBSONObject();
                    obj.put( "b", i );
                    queryRecords.add( obj );
                }
                dbcl.insert( queryRecords );
                for ( int i = 0; i < 500; i++ ) {
                    DBCursor cur = dbcl.query( new BasicBSONObject( "b", i ),
                            null, null, null );
                    BSONObject actual = cur.getNext();
                    if ( !actual.equals( queryRecords.get( i ) ) ) {
                        Assert.fail( "The expected master node is "
                                + queryRecords.get( i )
                                + ", but the actual master node is " + actual );
                    }
                    cur.close();
                    DBCursor removeCur = dbcl.queryAndRemove(
                            new BasicBSONObject( "b", i ), null, null, null, -1,
                            -1, 0 );
                    removeCur.getNext();
                    removeCur.close();
                }
            }
        }
    }

    private class RemoveOperation extends ResultStore {

        @ExecuteOrder(step = 1)
        private void remove() {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = sdb.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 2000; i > 0; i -= 2 ) {
                    DBCursor cur = dbcl.queryAndRemove(
                            new BasicBSONObject( "num", i ), null, null, null,
                            -1, -1, 0 );
                    if ( cur.getNext() != null ) {
                        removeNums.add( i );
                    }
                    cur.close();
                }
            }
        }
    }

    private void queryAndCheck( DBCollection dbcl,
            ArrayList< BSONObject > insertRecords,
            ArrayList< Integer > removeNums ) {
        ArrayList< BSONObject > actList = new ArrayList<>();
        for ( int i = 0; i < removeNums.size(); i++ ) {
            int e = removeNums.get( i );
            insertRecords.remove( e );
        }
        DBCursor cursor = dbcl.query( null, null, "{'num':1}", null );
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            actList.add( record );
        }
        if ( actList.size() != insertRecords.size() ) {
            Assert.fail( "lists don't have the same size expected ["
                    + insertRecords.size() + "] but found [" + actList.size()
                    + "], actList: " + actList.toString() + ", expList: "
                    + insertRecords.toString() );
        }
        for ( int i = 0; i < actList.size(); i++ ) {
            if ( !actList.get( i ).equals( insertRecords.get( i ) ) ) {
                Assert.fail( "expected [" + insertRecords.get( i )
                        + "], but found [" + actList.get( i ) + "], actList: "
                        + actList.toString() + ", expList: "
                        + insertRecords.toString() );
            }
        }
    }

}
