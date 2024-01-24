package com.sequoiadb.crud.serial;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import com.sequoiadb.crud.CRUDUitls;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
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
 * @Description seqDB-1541:并发+大数据量+记录+索引+不压缩+连到1个coord+读写分离+百条线程
 * @Author laojingtang
 * @Date 2018.01.04
 * @UpdataAuthor zhangyanan
 * @UpdateDate 2021.07.09
 * @Version 1.10
 */
public class CRUD1541 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "curdcl_1541";
    private DBCollection cl = null;
    private String index = "test_index";
    private ArrayList< BSONObject > allRecords = new ArrayList< BSONObject >();
    private CopyOnWriteArrayList< BSONObject > insertRecords = new CopyOnWriteArrayList<>();

    @BeforeClass
    public void setup() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "ReplSize", 0 );
        options.put( "Compressed", false );
        cl = cs.createCollection( clName, options );
        int beginNo = 0;
        int endNo = 40000;
        allRecords = CRUDUitls.insertData( cl, beginNo, endNo );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();

        int insertBeginNo = 0;
        int insertEndNo = 100;
        for ( int i = 0; i < 100; i++ ) {
            threadInsert insertData = new threadInsert( insertBeginNo,
                    insertEndNo );
            es.addWorker( insertData );
            insertBeginNo += 100;
            insertEndNo += 100;
        }

        int deleteBeginNo = 30000;
        int deleteEndNo = 40000;
        threadDelete deleteData = new threadDelete( deleteBeginNo,
                deleteEndNo );
        es.addWorker( deleteData );
        int updateBeginNo = 20000;
        int updateEndNo = 30000;
        threadUpdate updateData = new threadUpdate( updateBeginNo,
                updateEndNo );
        es.addWorker( updateData );
        int queryBeginNo = 0;
        int queryEndNo = 100;
        for ( int i = 0; i < 100; i++ ) {
            threadQuery queryData = new threadQuery( queryBeginNo, queryEndNo );
            es.addWorker( queryData );
            queryBeginNo += 100;
            queryEndNo += 100;
        }

        threadCreateIndex createIndex = new threadCreateIndex();
        es.addWorker( createIndex );

        es.run();
        Assert.assertEquals( deleteData.getRetCode(), 0 );
        Assert.assertEquals( updateData.getRetCode(), 0 );
        Assert.assertEquals( createIndex.getRetCode(), 0 );

        presetData();
        // 指定索引查询校验修改线程数据
        List< BSONObject > sublist = allRecords.subList( updateBeginNo,
                updateEndNo );
        String matcher = "{$and:[{no:{$gte:" + updateBeginNo + "}},{no:{$lt:"
                + updateEndNo + "}}]}";
        DBCursor cursor = cl.query( matcher, "", "{'no':1}",
                "{'':'test_index'}" );
        CRUDUitls.checkRecords( sublist, cursor );
        // 验证索引是否存在
        if ( !cl.isIndexExist( "test_index" ) ) {
            Assert.fail( "test_index not exist" );
        }
        // 验证所有数据
        CRUDUitls.checkRecords( cl, allRecords, "" );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public void presetData() {
        int beginNo1 = 20000;
        int endNo1 = 30000;
        BSONObject obj = new BasicBSONObject();
        for ( int i = beginNo1; i < endNo1; i++ ) {
            obj = allRecords.get( i );
            obj.put( "a", "updatetest" + beginNo1 );
        }
        allRecords.addAll( insertRecords );
        int beginNo2 = 30000;
        int endNo2 = 40000;
        List< BSONObject > sublist = allRecords.subList( beginNo2, endNo2 );
        allRecords.removeAll( sublist );
        Collections.sort( allRecords, new CRUDUitls.OrderBy( "no" ) );
    }

    private class threadInsert extends ResultStore {

        private int beginNo;
        private int endNo;

        private threadInsert( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 1)
        private void insert() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                int insertBeginNo = 40000 + beginNo;
                int insertEndNo = 40000 + endNo;
                ArrayList< BSONObject > insertRecords2 = new ArrayList< BSONObject >();
                insertRecords2 = CRUDUitls.insertData( cl, insertBeginNo,
                        insertEndNo );
                insertRecords.addAll( insertRecords2 );
                String matcher = "{$and:[{no:{$gte:" + insertBeginNo
                        + "}},{no:{$lt:" + insertEndNo + "}}]}";
                CRUDUitls.checkRecords( cl, insertRecords2, matcher );
            }
        }
    }

    private class threadDelete extends ResultStore {

        private int beginNo;
        private int endNo;

        private threadDelete( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;

        }

        @ExecuteOrder(step = 1)
        private void delete() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}}]}";
                cl.delete( matcher );
                ArrayList< BSONObject > deleteRecords = new ArrayList< BSONObject >();
                CRUDUitls.checkRecords( cl, deleteRecords, matcher );
            }
        }
    }

    private class threadUpdate extends ResultStore {
        private int beginNo;
        private int endNo;

        private threadUpdate( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;

        }

        @ExecuteOrder(step = 1)
        private void update() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}}]}";
                String modifier = "{$set:{a:'updatetest" + beginNo + "'}}";
                cl.update( matcher, modifier, "" );
            }
        }

    }

    private class threadQuery extends ResultStore {

        private int beginNo;
        private int endNo;

        private threadQuery( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @ExecuteOrder(step = 1)
        private void query() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                BasicBSONObject options = new BasicBSONObject();
                options.put( "PreferedInstance", "S" );
                db.setSessionAttr( options );
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:"
                        + endNo + "}}]}";
                DBCursor cursor = cl.query( matcher, "", "{'no':1}", "" );
                List< BSONObject > sublist = allRecords.subList( beginNo,
                        endNo );
                CRUDUitls.checkRecords( sublist, cursor );
            }
        }
    }

    private class threadCreateIndex extends ResultStore {
        @ExecuteOrder(step = 1)
        private void createIndex() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                cl.createIndex( index, "{no:1}", false, false );
            }
        }
    }
}
