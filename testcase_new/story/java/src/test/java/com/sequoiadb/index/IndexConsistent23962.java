package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import com.sequoiadb.testcommon.*;

import java.util.ArrayList;
import java.util.Random;

/**
 * @Description seqDB-23962:并发删除索引和增/删/改/查数据
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.30
 * @version 1.10
 */
public class IndexConsistent23962 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23962";
    private String clName = "cl_23962";
    private String idxName = "idx23962";
    private DBCollection cl;
    private ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
    private ArrayList< Integer > removeNums = new ArrayList< Integer >();
    private boolean runSuccess = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName );
        cl.createIndex( idxName, new BasicBSONObject( "a", 1 ), false, false );
    }

    @Test
    public void test() throws Exception {
        // 并发增删改查和删除索引
        ThreadExecutor es = new ThreadExecutor( 300000 );
        es.addWorker( new InsertOperation() );
        es.addWorker( new QueryOperation() );
        es.addWorker( new UpsertOperation() );
        es.addWorker( new RemoveOperation() );
        es.addWorker( new DropIndex() );
        es.run();

        IndexUtils.checkIndexTask( sdb, "Drop index", csName, clName, idxName,
                0 );
        queryAndCheck( cl, insertRecords, removeNums );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
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

    private class DropIndex extends ResultStore {
        @ExecuteOrder(step = 1)
        private void dropIndex() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                // 随机等待1500ms再删除索引
                Random random = new Random();
                Thread.sleep( random.nextInt( 1500 ) );
                cl.dropIndex( idxName );
            }
        }
    }

    private void queryAndCheck( DBCollection dbcl,
            ArrayList< BSONObject > insertRecords,
            ArrayList< Integer > removeNums ) {
        ArrayList< BSONObject > actList = new ArrayList<>();
        for ( int e : removeNums ) {
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
