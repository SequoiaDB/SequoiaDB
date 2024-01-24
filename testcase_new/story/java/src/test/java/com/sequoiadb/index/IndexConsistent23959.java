package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Random;

import com.sequoiadb.testcommon.CommLib;
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
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23959 :: 并发创建索引和增/删/改/查数据
 * @author wuyan
 * @date 2021.4.12
 * @version 1.10
 */

public class IndexConsistent23959 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_23959";
    private String indexName = "testindex23959";
    private int recsNum = 30000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();
    private ArrayList< BSONObject > insertSubList = new ArrayList<>();
    private ArrayList< BSONObject > querySubList = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone." );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.isCollectionExist( clName );
        }

        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        cl = cs.createCollection( clName, options );
        insertRecords = IndexUtils.insertData( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateIndex( indexName ) );

        int oprNum = 10000;
        int insertBeginNo = recsNum;
        es.addWorker( new InsertData( insertBeginNo, oprNum ) );
        int updateBeginNo = 0;
        es.addWorker( new UpdateData( updateBeginNo, oprNum ) );
        int removeBeginNo = 20000;
        es.addWorker( new RemoveData( removeBeginNo, oprNum ) );
        int queryBeginNo = 10000;
        es.addWorker( new QueryData( queryBeginNo, oprNum ) );
        es.run();

        // check results
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                clName, indexName );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName, true );

        checkDatas( insertBeginNo, updateBeginNo, removeBeginNo, queryBeginNo,
                oprNum );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                if ( cs.isCollectionExist( clName ) ) {
                    cs.dropCollection( clName );
                }

            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex {
        private String indexName;

        private CreateIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.createIndex( indexName, "{no:1,testa:1}", false, false );
            }
        }
    }

    private class InsertData extends ResultStore {
        private int insertNum;
        private int beginNo;

        private InsertData( int beginNo, int insertNum ) {
            this.beginNo = beginNo;
            this.insertNum = insertNum;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                insertSubList = insertData( cl, insertNum, beginNo );
            }
        }
    }

    private class UpdateData extends ResultStore {
        private int updateNum;
        private int beginNo;

        private UpdateData( int beginNo, int updateNum ) {
            this.beginNo = beginNo;
            this.updateNum = updateNum;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );

                int endCond = beginNo + updateNum;
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endCond + "}}]}";
                String modifier = "{$set:{testa:'updatetest" + beginNo + "'}}";
                cl.update( matcher, modifier, "" );
            }
        }
    }

    private class RemoveData extends ResultStore {
        private int removeNum = 10000;
        private int beginNo;

        private RemoveData( int beginNo, int removeNum ) {
            this.beginNo = beginNo;
            this.removeNum = removeNum;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );

                int endCond = beginNo + removeNum;
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endCond + "}}]}";
                cl.delete( matcher );
            }
        }
    }

    private class QueryData extends ResultStore {
        private int queryNo;
        private int queryNum;

        private QueryData( int queryNo, int queryNum ) {
            this.queryNo = queryNo;
            this.queryNum = queryNum;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );

                int endCond = queryNo + queryNum;
                String matcher = "{$and:[{no:{$gte:" + queryNo + "}},{no:{$lt:"
                        + endCond + "}}]}";
                DBCursor cursor = cl.query( matcher, "", "{'no':1}", "" );
                while ( cursor.hasNext() ) {
                    BSONObject record = cursor.getNext();
                    querySubList.add( record );
                }
                cursor.close();
            }
        }

    }

    private void checkDatas( int insertBeginNo, int updateBeginNo,
            int removeBeginNo, int queryBeginNo, int oprNum ) {
        // check insert result
        int insertEndNo = insertBeginNo + oprNum;
        String insertMatcher = "{$and:[{no:{$gte:" + insertBeginNo
                + "}},{no:{$lt:" + insertEndNo + "}}]}";
        IndexUtils.checkRecords( cl, insertSubList, insertMatcher,
                "{'':'" + indexName + "'}" );
        int queryEndNo = queryBeginNo + oprNum;
        String queryMatcher = "{$and:[{no:{$gte:" + queryBeginNo
                + "}},{no:{$lt:" + queryEndNo + "}}]}";
        IndexUtils.checkRecords( cl, querySubList, queryMatcher,
                "{'':'" + indexName + "'}" );

        // check update result
        for ( int i = updateBeginNo; i < oprNum; i++ ) {
            BSONObject obj = insertRecords.get( i );
            obj.put( "testa", "updatetest" + updateBeginNo );
        }
        int updateEndNo = updateBeginNo + oprNum;
        String updateMatcher = "{$and:[{no:{$gte:" + updateBeginNo
                + "}},{no:{$lt:" + updateEndNo + "}}]}";
        List< BSONObject > updatelist = insertRecords.subList( updateBeginNo,
                updateEndNo );
        IndexUtils.checkRecords( cl, updatelist, updateMatcher,
                "{'':'" + indexName + "'}" );

        // check remove result
        int removeEndNo = removeBeginNo + oprNum;
        String removeMatcher = "{$and:[{no:{$gte:" + removeBeginNo
                + "}},{no:{$lt:" + removeEndNo + "}}]}";
        long count = cl.getCount( removeMatcher );
        Assert.assertEquals( count, 0 );

        // check all data Num
        List< BSONObject > sublist = insertRecords.subList( removeBeginNo,
                removeBeginNo + oprNum );
        insertRecords.removeAll( sublist );
        insertRecords.addAll( insertSubList );
        IndexUtils.checkRecords( cl, insertRecords, "",
                "{'':'" + indexName + "'}" );

        long allNum = cl.getCount();
        Assert.assertEquals( allNum, insertRecords.size() );
    }

    private ArrayList< BSONObject > insertData( DBCollection dbcl,
            int recordNum, int beginNo ) {
        ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
        Random random = new Random();
        for ( int i = beginNo; i < beginNo + recordNum; i++ ) {
            String keyValue = IndexUtils
                    .getRandomString( random.nextInt( 30 ) );
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", keyValue );
            obj.put( "no", i );
            obj.put( "num", i );
            insertRecord.add( obj );
        }
        dbcl.insert( insertRecord );
        return insertRecord;
    }
}