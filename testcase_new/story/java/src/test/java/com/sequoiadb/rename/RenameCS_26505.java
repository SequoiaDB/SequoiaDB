package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.Random;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-26505:同组不同CS下CL，并发执行renameCS/renameCL/crud/alterCL
 * @Author liuli
 * @Date 2022.05.12
 * @UpdateAuthor liuli
 * @UpdateDate 2022.05.12
 * @version 1.10
 */
public class RenameCS_26505 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String csName1 = "cs_26505_1";
    private String csName2 = "cs_26505_2";
    private String csName3 = "cs_26505_3";
    private String clName1 = "cl_26505_1";
    private String clName2 = "cl_26505_2";
    private String clName3 = "cl_26505_3";
    private String csNameNew3 = "cs_26505_new_3";
    private String clNameNew2 = "cl_26505_new_2";
    private ArrayList< String > groupNames = new ArrayList<>();
    private ArrayList< BSONObject > insertRecordsCL1 = new ArrayList<>();
    private ArrayList< Integer > removeNums = new ArrayList<>();
    private ArrayList< BSONObject > insertRecordsCL2 = new ArrayList<>();
    private ArrayList< BSONObject > insertRecordsCL3 = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "can not support split" );
        }

        if ( sdb.isCollectionSpaceExist( csName1 ) ) {
            sdb.dropCollectionSpace( csName1 );
        }
        if ( sdb.isCollectionSpaceExist( csName2 ) ) {
            sdb.dropCollectionSpace( csName2 );
        }
        if ( sdb.isCollectionSpaceExist( csName3 ) ) {
            sdb.dropCollectionSpace( csName3 );
        }
        if ( sdb.isCollectionSpaceExist( csNameNew3 ) ) {
            sdb.dropCollectionSpace( csNameNew3 );
        }

        groupNames = CommLib.getDataGroupNames( sdb );
        CollectionSpace dbcs1 = sdb.createCollectionSpace( csName1 );
        CollectionSpace dbcs2 = sdb.createCollectionSpace( csName2 );
        CollectionSpace dbcs3 = sdb.createCollectionSpace( csName3 );

        dbcs1.createCollection( clName1,
                new BasicBSONObject( "Group", groupNames.get( 0 ) ) );
        dbcs2.createCollection( clName2,
                new BasicBSONObject( "Group", groupNames.get( 0 ) ) );
        dbcs3.createCollection( clName3,
                new BasicBSONObject( "Group", groupNames.get( 0 ) ) );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();

        // cl1执行crud和alter
        // cl2执行插入和renameCL
        // cl3执行插入和renameCS
        InsertOperation insertOperation = new InsertOperation( csName1,
                clName1 );
        UpsertOperation upsertOperation = new UpsertOperation( csName1,
                clName1 );
        QueryOperation queryOperation = new QueryOperation( csName1, clName1 );
        RemoveOperation removeOperation = new RemoveOperation( csName1,
                clName1 );
        AlterCL alterCL = new AlterCL( csName1, clName1 );
        RenameCL renameCL = new RenameCL( csName2, clName2, clNameNew2 );
        RenameCS renameCS = new RenameCS( csName3, csNameNew3 );
        Insert insertCL2 = new Insert( csName2, clName2 );
        Insert insertCL3 = new Insert( csName3, clName3 );

        es.addWorker( insertOperation );
        es.addWorker( upsertOperation );
        es.addWorker( queryOperation );
        es.addWorker( removeOperation );
        es.addWorker( alterCL );
        es.addWorker( renameCL );
        es.addWorker( renameCS );
        es.addWorker( insertCL2 );
        es.addWorker( insertCL3 );
        es.run();

        // 校验clName2 rename成功，并校验数据
        CollectionSpace dbcs2 = sdb.getCollectionSpace( csName2 );
        Assert.assertTrue( dbcs2.isCollectionExist( clNameNew2 ) );
        Assert.assertFalse( dbcs2.isCollectionExist( clName2 ) );
        DBCollection dbcl2 = dbcs2.getCollection( clNameNew2 );
        if ( renameCL.getRetCode() == 0 ) {
            RenameUtil.checkRecords( dbcl2, insertRecordsCL2, "{a:1}" );
        } else {
            if ( renameCL.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && renameCL.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode() ) {
                Assert.fail( "not expected error, renameCS.getRetCode() : "
                        + renameCL.getRetCode() );
            }
            Assert.assertEquals( dbcl2.getCount(), 0 );
        }

        // 校验csName3 rename成功，并校验数据
        Assert.assertTrue( sdb.isCollectionSpaceExist( csNameNew3 ) );
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName3 ) );
        DBCollection dbcl3 = sdb.getCollectionSpace( csNameNew3 )
                .getCollection( clName3 );
        if ( renameCS.getRetCode() == 0 ) {
            RenameUtil.checkRecords( dbcl3, insertRecordsCL3, "{a:1}" );
        } else {
            if ( renameCS.getRetCode() != SDBError.SDB_DMS_NOTEXIST
                    .getErrorCode()
                    && renameCS.getRetCode() != SDBError.SDB_DMS_CS_NOTEXIST
                            .getErrorCode() ) {
                Assert.fail( "not expected error, renameCS.getRetCode() : "
                        + renameCS.getRetCode() );
            }
            Assert.assertEquals( dbcl3.getCount(), 0 );
        }

        // 校验cl1数据
        DBCollection dbcl1 = sdb.getCollectionSpace( csName1 )
                .getCollection( clName1 );
        queryAndCheck( dbcl1, insertRecordsCL1, removeNums );

        // 校验cl1 alter成功
        Assert.assertTrue( dbcl1.isIndexExist( "$shard" ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName1 ) ) {
                sdb.dropCollectionSpace( csName1 );
            }
            if ( sdb.isCollectionSpaceExist( csName2 ) ) {
                sdb.dropCollectionSpace( csName2 );
            }
            if ( sdb.isCollectionSpaceExist( csName3 ) ) {
                sdb.dropCollectionSpace( csName3 );
            }
            if ( sdb.isCollectionSpaceExist( csNameNew3 ) ) {
                sdb.dropCollectionSpace( csNameNew3 );
            }
        } finally {
            if ( this.sdb != null ) {
                this.sdb.close();
            }
        }
    }

    private class AlterCL extends ResultStore {
        private String csName;
        private String clName;

        public AlterCL( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 2)
        private void alterCL() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                dbcl.alterCollection( new BasicBSONObject( "ShardingKey",
                        new BasicBSONObject( "num", 1 ) ) );
            }
        }
    }

    private class RenameCL extends ResultStore {
        private String csName;
        private String clName;
        private String newCLName;

        public RenameCL( String csName, String clName, String newCLName ) {
            this.csName = csName;
            this.clName = clName;
            this.newCLName = newCLName;
        }

        @ExecuteOrder(step = 2)
        private void renameCL() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待500ms后再renameCL，防止先renameCS再插入数据
                Random random = new Random();
                Thread.sleep( random.nextInt( 500 ) );
                CollectionSpace dbcs = db.getCollectionSpace( csName );
                dbcs.renameCollection( clName, newCLName );
            }
        }
    }

    private class RenameCS extends ResultStore {
        private String csName;
        private String newCSName;

        public RenameCS( String csName, String newCSName ) {
            this.csName = csName;
            this.newCSName = newCSName;
        }

        @ExecuteOrder(step = 2)
        private void renameCS() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                // 随机等待500ms后再renameCS，防止先renameCS再插入数据
                Random random = new Random();
                Thread.sleep( random.nextInt( 500 ) );
                db.renameCollectionSpace( csName, newCSName );
            }
        }
    }

    private class Insert extends ResultStore {
        private String csName;
        private String clName;
        Sequoiadb db = null;
        DBCollection dbcl = null;

        public Insert( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1)
        private void getCL() {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            dbcl = db.getCollectionSpace( csName ).getCollection( clName );
        }

        @ExecuteOrder(step = 2)
        private void insert() {
            try {
                if ( csName.equals( csName2 ) ) {
                    int insertNums = 10000;
                    insertRecordsCL2 = insertDatas( dbcl, insertNums );
                } else if ( csName.equals( csName3 ) ) {
                    int insertNums = 20000;
                    insertRecordsCL3 = insertDatas( dbcl, insertNums );
                }
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class InsertOperation extends ResultStore {
        private String csName;
        private String clName;

        public InsertOperation( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 2)
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
                    insertRecordsCL1.add( obj );
                }
                dbcl.bulkInsert( insertRecordsCL1 );
            }
        }
    }

    private class UpsertOperation extends ResultStore {
        private String csName;
        private String clName;

        public UpsertOperation( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 2)
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
                        insertRecordsCL1.set( i, obj );
                    }
                    cur.close();
                }
            }
        }
    }

    private class QueryOperation extends ResultStore {
        private String csName;
        private String clName;
        private ArrayList< BSONObject > queryRecords = new ArrayList< BSONObject >();

        public QueryOperation( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 2)
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
                dbcl.bulkInsert( queryRecords );
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
        private String csName;
        private String clName;

        public RemoveOperation( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 2)
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
        cursor.close();
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

    private ArrayList< BSONObject > insertDatas( DBCollection dbcl,
            int insertNums ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        for ( int i = 0; i < insertNums; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", "test" + i );
            obj.put( "no", i );
            obj.put( "order", i );
            obj.put( "a", i );
            obj.put( "ftest", i + 0.2345 );
            insertRecord.add( obj );
        }
        dbcl.bulkInsert( insertRecord );
        return insertRecord;
    }
}
