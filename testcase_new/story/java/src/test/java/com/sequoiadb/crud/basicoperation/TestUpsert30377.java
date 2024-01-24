package com.sequoiadb.crud.basicoperation;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
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
 * @Descreption seqDB-30377:并发upsert，多个线程同时插入相同主键的相同记录（创建多个索引），不报错-38
 * @Author Cheng Jingjing
 * @CreateDate 2023/03/07
 * @Version 1.0
 */
public class TestUpsert30377 extends SdbTestBase {
    private String csName = "cs_30377";
    private String clName = "cl_30377";
    private String indexName = "index_30377";
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @DataProvider(name = "upsertData")
    public Object[][] generateUpsertDatas() {
        // 场景一：多个索引， matcher包含冲突索引字段，matcher包含部分待插入记录字段
        // upsert( { $set: { a: 1, b: 1, c: 3 } }, { a: 1, b: 1 } ); index:{ a: 1, b: 1 },{ a: 1 }
        BasicBSONObject insertData1 = new BasicBSONObject();
        insertData1.put("a", 1);
        insertData1.put("b", 1);
        insertData1.put("c", 3);
        BasicBSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", 1);
        matcher1.put("b", 1);
        BasicBSONObject index1_1 = new BasicBSONObject();
        index1_1.put("a", 1);
        index1_1.put("b", 1);
        BasicBSONObject index1_2 = new BasicBSONObject();
        index1_2.put("a", 1);

        // 场景二：嵌套场景 多个索引，matcher包含冲突索引字段，matcher包含部分待插入记录字段
        // upsert( { $set: { a: { b: 1 } }, { c: { d: 1 } } }, { a: { b: 1 } } ); index:{ a: { b: 1 } },{ a: 1 }
        BasicBSONObject insertData2 = new BasicBSONObject();
        BasicBSONObject insertData2_1 = new BasicBSONObject();
        BasicBSONObject insertData2_2 = new BasicBSONObject();
        insertData2_1.put("b", 1);
        insertData2.put("a", insertData2_1);
        insertData2_2.put("d", 1);
        insertData2.put("c", insertData2_2);
        BasicBSONObject matcher2 = new BasicBSONObject();
        BasicBSONObject matcher2_1 = new BasicBSONObject();
        matcher2_1.put("b", 1);
        matcher2.put("a", matcher2_1);
        BasicBSONObject index2_1 = new BasicBSONObject();
        BasicBSONObject index2_2 = new BasicBSONObject();
        index2_1.put("a.b", 1);
        index2_2.put("a", 1);

        // 场景三：多个索引， matcher包含冲突索引字段，matcher包含所有待插入记录字段
        // upsert( { $set: c: 3 }, { c: 3 } ); index:{ a: { b: 1 } },{ c: 1 }
        BasicBSONObject insertData3 = new BasicBSONObject();
        insertData3.put("c", 3);
        BasicBSONObject matcher3 = new BasicBSONObject();
        matcher3.put("c", 3);
        BasicBSONObject index3_1 = new BasicBSONObject();
        BasicBSONObject index3_2 = new BasicBSONObject();
        index3_1.put("a.b", 1);
        index3_2.put("c", 1);

        // 场景四：嵌套场景 多个索引，matcher包含冲突索引字段，matcher包含所有待插入记录字段
        // upsert( { $set: { a: { b: 1 }, c: 3 }, { a: { b: 1 } , c: 3 } ); index:{ a: { b: 1 } },{ c: 1 }
        BasicBSONObject insertData4 = new BasicBSONObject();
        BasicBSONObject insertData4_1 = new BasicBSONObject();
        insertData4_1.put("b", 1);
        insertData4.put("a", insertData4_1);
        insertData4.put("c", 3);
        BasicBSONObject matcher4 = new BasicBSONObject();
        BasicBSONObject matcher4_1 = new BasicBSONObject();
        matcher4_1.put("b", 1);
        matcher4.put("c", 3);
        matcher4.put("a", matcher4_1);
        BasicBSONObject index4_1 = new BasicBSONObject();
        BasicBSONObject index4_2 = new BasicBSONObject();
        index4_1.put("a.b", 1);
        index4_2.put("c", 1);

        return new Object[][]{new Object[]{1, insertData1, matcher1, index1_1, index1_2},
                new Object[]{2, insertData2, matcher2, index2_1, index2_2},
                new Object[]{3, insertData3, matcher3, index3_1, index3_2},
                new Object[]{4, insertData4, matcher4, index4_1, index4_2},};
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

    @Test(dataProvider = "upsertData")
    private void test(int num, BasicBSONObject insertData, BasicBSONObject matcher, BasicBSONObject index1, BasicBSONObject index2) throws Exception {
        if (cs.isCollectionExist(clName + "_" + num)) {
            cs.dropCollection(clName + "_" + num);
        }
        // 创建集合
        cl = cs.createCollection(clName + "_" + num);
        // 创建索引
        cl.createIndex(indexName + "_" + num + "_1", index1, true, false);
        cl.createIndex(indexName + "_" + num + "_2", index2, true, false);

        ThreadExecutor es = new ThreadExecutor();
        int threadNum =  10;
        for( int i = 0; i < threadNum; i++ ){
            es.addWorker( new testUpsert( num, insertData, matcher ) );
        }
        es.run();

        ArrayList<BSONObject> expRecord = new ArrayList<>();
        ArrayList<BSONObject> actRecord = new ArrayList<>();
        expRecord.add( insertData );
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