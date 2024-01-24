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
 * @Descreption seqDB-30343:并发upsert，多个线程同时插入相同主键的相同记录，不报错-38
 * @Author Cheng Jingjing
 * @CreateDate 2022.03.07
 * @Version 1.10
 */
public class TestUpsert30343 extends SdbTestBase {
    private String csName = "cs_30343";
    private String clName = "cl_30343";
    private String indexName = "index_30343";
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @DataProvider( name = "upsertData" )
    public Object[][] generateUpsertDatas() {
        // 场景一：索引与matcher相同，matcher与待插入记录相匹配，matcher包含待插入记录部分字段
        // eg: upsert( { $set: { a: 1, b: 1, c: 3 } }, { a: 1, b: 1 } ); index:{ a: 1, b: 1 }
        BasicBSONObject insertData1 = new BasicBSONObject();
        insertData1.put("a", 1);
        insertData1.put("b", 1);
        insertData1.put("c", 3);
        BasicBSONObject matcher1 = new BasicBSONObject();
        matcher1.put("a", 1);
        matcher1.put("b", 1);
        BasicBSONObject index1 = new BasicBSONObject();
        index1.put("a", 1);
        index1.put("b", 1);

        // 场景二：索引与matcher相同，且matcher与待插入记录相匹配，matcher包含待插入记录所有字段
        // upsert( { $set: { a: 2, a: 2 }, { a: 2, b: 2 } ); index:{ a: 1, b: 1 }
        BasicBSONObject insertData2 = new BasicBSONObject();
        insertData2.put("a", 2);
        insertData2.put("b", 2);
        BasicBSONObject matcher2 = new BasicBSONObject();
        matcher2.put("a", 2);
        matcher2.put("b", 2);
        BasicBSONObject index2 = new BasicBSONObject();
        index2.put("a", 1);
        index2.put("b", 1);

        // 场景三：macher包含部分索引字段，且matcher与待插入记录相匹配
        // upsert( { $set: { a: 2, b: 2, c: 3 }, { a: 2, b: 2 } ); index:{ a: 1 }
        BasicBSONObject insertData3 = new BasicBSONObject();
        insertData3.put("a", 2);
        insertData3.put("b", 2);
        insertData3.put("c", 3);
        BasicBSONObject matcher3 = new BasicBSONObject();
        matcher3.put("a", 2);
        matcher3.put("b", 2);
        BasicBSONObject index3 = new BasicBSONObject();
        index3.put("a", 1);

        // 场景四：macher包含部分索引字段，matcher包含待插入记录所有字段
        // upsert( { $set: { a: 2, b: 2 }, { a: 2, b: 2 } ); index:{ a: 1 }
        BasicBSONObject insertData4 = new BasicBSONObject();
        insertData4.put("a", 2);
        insertData4.put("b", 2);
        insertData4.put("c", 3);
        BasicBSONObject matcher4 = new BasicBSONObject();
        matcher4.put("a", 2);
        matcher4.put("b", 2);
        BasicBSONObject index4 = new BasicBSONObject();
        index4.put("a", 1);

        // 场景四：嵌套场景 索引与matcher相同，matcher包含待插入记录部分字段
        // upsert( { $set: { a: { b: 1 }, c: 1, d: 1 }, { a: { b: 1 }, c: 1 } ); index:{ a: { b: 1 }, c: 1 }
        BasicBSONObject insertData5 = new BasicBSONObject();
        BasicBSONObject insertData5_1 = new BasicBSONObject();
        insertData5_1.put("b", 1);
        insertData5.put("a", insertData5_1);
        insertData5.put("c", 1);
        insertData5.put("d", 1);
        BasicBSONObject matcher5 = new BasicBSONObject();
        BasicBSONObject matcher5_1 = new BasicBSONObject();
        matcher5_1.put("b", 1);
        matcher5.put("a", matcher5_1);
        matcher5.put("c", 1);
        BasicBSONObject index5 = new BasicBSONObject();
        index5.put("a.b", 1);
        index5.put("c", 1);

        // 场景六： 嵌套场景 索引与matcher相同，matcher包含待插入记录所有字段
        // upsert( { $set: { a: { b: 1 }, c: 1 }, { a: { b: 1 }, c: 1 } ); index:{ a: { b: 1 }, c: 1 }
        BasicBSONObject insertData6 = new BasicBSONObject();
        BasicBSONObject insertData6_1 = new BasicBSONObject();
        insertData6_1.put("b", 1);
        insertData6.put("a", insertData6_1);
        insertData6.put("c", 1);
        BasicBSONObject matcher6 = new BasicBSONObject();
        BasicBSONObject matcher6_1 = new BasicBSONObject();
        matcher6_1.put("b", 1);
        matcher6.put("a", matcher6_1);
        matcher6.put("c", 1);
        BasicBSONObject index6 = new BasicBSONObject();
        index6.put("a.b", 1);
        index6.put("c", 1);

        // 场景七： 嵌套场景 matcher包含索引字段，matcher包含待插入记录所有字段
        // upsert( { $set: { a: { b: 1 }, c: 1 }, { a: { b: 1 }, c: 1 } ); index:{ a: { b: 1 } }
        BasicBSONObject insertData7 = new BasicBSONObject();
        BasicBSONObject insertData7_1 = new BasicBSONObject();
        insertData7_1.put("b", 1);
        insertData7.put("a", insertData7_1);
        insertData7.put("c", 1);
        BasicBSONObject matcher7 = new BasicBSONObject();
        BasicBSONObject matcher7_1 = new BasicBSONObject();
        matcher7_1.put("b", 1);
        matcher7.put("a", matcher7_1);
        matcher7.put("c", 1);
        BasicBSONObject index7 = new BasicBSONObject();
        index7.put("a.b", 1);

        // 场景八： 嵌套场景 matcher包含索引字段，matcher包含待插入记录部分字段
        // upsert( { $set: { a: { b: 1 }, c: 1, d: 1 }, { a: { b: 1 }, c: 1 } ); index:{ a: { b: 1 } }
        BasicBSONObject insertData8 = new BasicBSONObject();
        BasicBSONObject insertData8_1 = new BasicBSONObject();
        insertData8_1.put("b", 1);
        insertData8.put("a", insertData8_1);
        insertData8.put("c", 1);
        insertData8.put("d", 1);
        BasicBSONObject matcher8 = new BasicBSONObject();
        BasicBSONObject matcher8_1 = new BasicBSONObject();
        matcher8_1.put("b", 1);
        matcher8.put("a", matcher8_1);
        matcher8.put("c", 1);
        BasicBSONObject index8 = new BasicBSONObject();
        BasicBSONObject index8_1 = new BasicBSONObject();
        index8.put("a.b", 1);

        return new Object[][] { new Object[]{1, insertData1, matcher1, index1},
                new Object[]{2, insertData2, matcher2, index2},
                new Object[]{3, insertData3, matcher3, index3},
                new Object[]{4, insertData4, matcher4, index4},
                new Object[]{5, insertData5, matcher5, index5},
                new Object[]{6, insertData6, matcher6, index6},
                new Object[]{7, insertData7, matcher7, index7},
                new Object[]{8, insertData8, matcher8, index8} };
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
    private void test( int num, BasicBSONObject insertData, BasicBSONObject matcher, BasicBSONObject index ) throws Exception {
        if( cs.isCollectionExist(  clName + "_" + num ) ){
            cs.dropCollection(clName + "_" + num );
        }
        // 创建集合
        cl = cs.createCollection( clName + "_" + num );
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
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ) {
                DBCollection cl = db.getCollectionSpace(csName).getCollection(clName + "_" + num);
                BSONObject modifer = new BasicBSONObject();
                modifer.put("$set", insertData);
                cl.upsertRecords(matcher, modifer);
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
