package com.sequoiadb.crud.basicoperation;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.util.ArrayList;

/**
 * @Descreption seqDB-30382:并发upsert，多个线程同时插入同一条记录，enforced为true导致主键冲突，报错-38
 * @Author Cheng Jingjing
 * @CreateDate 2023/03/08
 * @Version 1.0
 */
public class TestUpsert30382 extends SdbTestBase {
    private String csName = "cs_30382";
    private String clName = "cl_30382";
    private String indexName = "index_30382";
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @DataProvider( name = "upsertData" )
    public Object[][] generateUpsertDatas() {
        // 场景一：冲突的索引键不包含在matcher中，待插入记录不含索引字段
        // upsert( { $set: { c: 1 } }, { b: 1 } ); index:{ a: 1 }
        BasicBSONObject insertData1 = new BasicBSONObject();
        insertData1.put( "c", 1 );
        BasicBSONObject matcher1 = new BasicBSONObject();
        matcher1.put( "b", 1 );
        BasicBSONObject index1 = new BasicBSONObject();
        index1.put( "a", 1 );

        // 场景二：嵌套场景 冲突的索引键包含在matcher中，待插入记录不含索引字段
        // upsert( { $set: { a: 1, b: 1, c: 1 }, { a: { b: 1 } } ); index:{ a: { b: 1 } }
        BasicBSONObject insertData2 = new BasicBSONObject();
        insertData2.put( "a", 1 );
        insertData2.put( "b", 1 );
        insertData2.put( "c", 1 );
        BasicBSONObject matcher2 = new BasicBSONObject();
        BasicBSONObject matcher2_1 = new BasicBSONObject();
        matcher2_1.put( "b", 1 );
        matcher2.put( "a", matcher2_1 );
        BasicBSONObject index2 = new BasicBSONObject();
        index2.put( "a.b", 1 );

        // 场景三：嵌套场景 冲突的索引键不包含在matcher中，待插入记录不含索引字段
        // upsert( { $set: {a: { b: 1 } }, { a: { b: { c: 1 } } } ); index:{ a: { b: { c: 1 } } }
        BasicBSONObject insertData3 = new BasicBSONObject();
        BasicBSONObject insertData3_1 = new BasicBSONObject();
        insertData3_1.put( "b", 1 );
        insertData3.put( "a", insertData3_1 );
        BasicBSONObject matcher3 = new BasicBSONObject();
        BasicBSONObject matcher3_1 = new BasicBSONObject();
        matcher3_1.put( "c", 1 );
        BasicBSONObject matcher3_2 = new BasicBSONObject();
        matcher3_2.put( "b", matcher3_1 );
        matcher3.put( "a", matcher3_2 );
        BasicBSONObject index3 = new BasicBSONObject();
        index3.put( "a.b.c", 1 );

        return new Object[][] { new Object[] { 1, insertData1, matcher1, index1 },
                new Object[] { 2, insertData2, matcher2, index2 },
                new Object[] { 3, insertData3, matcher3, index3 } };
    }

    @BeforeClass
    public void setUp() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( db.isCollectionSpaceExist( csName ) ) {
            db.dropCollectionSpace( csName );
        }
        cs = db.createCollectionSpace( csName );
    }

    @Test( dataProvider = "upsertData" )
    private void test ( int num, BasicBSONObject insertData, BasicBSONObject matcher, BasicBSONObject index )  throws Exception {
        if( cs.isCollectionExist( clName+ "_" + num ) ){
            cs.dropCollection(clName+ "_" + num );
        }
        // 创建集合
        cl = cs.createCollection( clName+ "_" + num );
        // 创建索引
        cl.createIndex(indexName + "_" + num, index, true, true );
        ThreadExecutor es = new ThreadExecutor();
        int threadNum =  10;
        for( int i = 0; i < threadNum; i++ ){
            es.addWorker( new testUpsert( num, insertData, matcher ) );
        }
        es.run();

        ArrayList<BSONObject> expRecord = new ArrayList<>();
        ArrayList<BSONObject> actRecord = new ArrayList<>();
        expRecord.add( insertData );
        if( num == 1 ) {
            expRecord.clear();
            BasicBSONObject targetInsert = new BasicBSONObject();
            targetInsert.put( "c", 1 );
            targetInsert.put( "b", 1 );
            expRecord.add( targetInsert );
        }
        DBCursor cursor = cl.query();
        while( cursor.hasNext() ){
            BSONObject record = cursor.getNext();
            record.removeField("_id");
            actRecord.add( record );
        }
        cursor.close();
        Assert.assertEquals( actRecord, expRecord );
    }

    private class testUpsert{
        int num;
        private BasicBSONObject insertData;
        private BasicBSONObject matcher;

        private testUpsert( int num, BasicBSONObject insertData, BasicBSONObject matcher ){
            this.num = num;
            this.insertData = insertData;
            this.matcher = matcher;
        }

        @ExecuteOrder(step=1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ){
                DBCollection cl = db.getCollectionSpace( csName ).getCollection( clName+ "_" + num );
                BSONObject modifer = new BasicBSONObject();
                modifer.put("$set", insertData);
                cl.upsertRecords( matcher, modifer );
            }
            catch ( BaseException e ) {
                if ( e.getErrorCode() != -38 ) {
                    throw  e;
                }
            }
        }
    }


    @AfterClass
    public void tearDown() {
        try {
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
            }
        } finally {
            db.close();
        }
    }
}
