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
 * @Descreption seqDB-30379:并发执行upsert前，已存在相同主键的记录导致冲突（创建多个索引）
 * @Author Cheng Jingjing
 * @CreateDate 2023/03/07
 * @Version 1.0
 */
public class TestUpsert30379 extends SdbTestBase {
    private String csName = "cs_30379";
    private String clName = "cl_30379";
    private String indexName = "index_30379";
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @DataProvider( name = "upsertData" )
    public Object[][] generateUpsertDatas() {
        // 场景一：集合已存在冲突记录，多个索引，matcher与冲突的索引字段相同
        // upsert( { $set: { b: 1 } }, { a: 3 } ); index:{ a: 1 },{ b: 1 }
        BasicBSONObject existData1 = new BasicBSONObject();
        existData1.put( "b", 1 );
        BasicBSONObject insertData1 = new BasicBSONObject();
        insertData1.put( "b", 1 );
        BasicBSONObject matcher1 = new BasicBSONObject();
        matcher1.put( "a", 3 );
        BasicBSONObject index1_1 = new BasicBSONObject();
        BasicBSONObject index1_2 = new BasicBSONObject();
        index1_1.put( "a", 1 );
        index1_2.put( "b", 1 );

        // 场景二：集合已存在冲突记录，多个索引，matcher包含冲突的索引字段
        // upsert( { $set: { b: 1 }, { a: 3,  b: 3 } ); index:{ a: 1 },{ b: 1 }
        BasicBSONObject existData2 = new BasicBSONObject();
        existData2.put( "b", 1 );
        BasicBSONObject insertData2 = new BasicBSONObject();
        insertData2.put( "b", 1 );
        BasicBSONObject matcher2 = new BasicBSONObject();
        matcher2.put( "a", 3 );
        matcher2.put( "b", 3 );
        BasicBSONObject index2_1 = new BasicBSONObject();
        BasicBSONObject index2_2 = new BasicBSONObject();
        index2_1.put( "a", 1 );
        index2_2.put( "b", 1 );

        // 场景三：嵌套场景 集合已存在冲突记录，多个索引，matcher不包含冲突的索引字段
        // upsert( { $set: { a: { b: 3 } }, { c: 3 } ); index:{ a: { b: 1 } },{ c: 1 }
        BasicBSONObject existData3 = new BasicBSONObject();
        existData3.put( "c", 1 );
        BasicBSONObject insertData3 = new BasicBSONObject();
        BasicBSONObject insertData3_1 = new BasicBSONObject();
        insertData3_1.put( "b", 3 );
        insertData3.put( "a", insertData3_1 );
        BasicBSONObject matcher3 = new BasicBSONObject();
        matcher3.put( "c", 3 );
        BasicBSONObject index3_1 = new BasicBSONObject();
        BasicBSONObject index3_2 = new BasicBSONObject();
        index3_1.put( "a.b", 1 );
        index3_2.put( "c", 1 );

        // 场景三：嵌套场景 集合已存在冲突记录，多个索引，matcher包含冲突的索引字段
        // upsert( { $set: { c: 1 }, { a: { b: 1 } , c: 3 } ); index:{ a: { b: 1 } },{ c: 1 }
        BasicBSONObject existData4 = new BasicBSONObject();
        existData4.put( "c", 1 );
        BasicBSONObject insertData4 = new BasicBSONObject();
        insertData4.put( "c", 1 );
        BasicBSONObject matcher4 = new BasicBSONObject();
        BasicBSONObject matcher4_1 = new BasicBSONObject();
        matcher4_1.put( "b", 1 );
        matcher4.put( "c", 3 );
        matcher4.put( "a", matcher4_1 );
        BasicBSONObject index4_1 = new BasicBSONObject();
        BasicBSONObject index4_2 = new BasicBSONObject();
        index4_1.put( "a.b", 1 );
        index4_2.put( "c", 1 );

        return new Object[][]{ new Object[]{1, existData1, insertData1, matcher1, index1_1, index1_2 },
                new Object[] { 2, existData2, insertData2, matcher2, index2_1, index2_2 },
                new Object[] { 3, existData3, insertData3, matcher3, index3_1, index3_2 },
                new Object[] { 4, existData4, insertData4, matcher4, index4_1, index4_2 } };
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
    private void test( int num, BasicBSONObject existData, BasicBSONObject insertData, BasicBSONObject matcher, BasicBSONObject index1, BasicBSONObject index2 ) throws Exception {
        if( cs.isCollectionExist( clName+ "_" + num ) ){
            cs.dropCollection( clName + "_" + num );
        }
        // 创建集合，插入数据
        cl = cs.createCollection( clName + "_"+ num );
        cl.insertRecord( existData );
        // 创建索引
        cl.createIndex(indexName + "_" + num + "_1", index1, true, true );
        cl.createIndex(indexName + "_" + num + "_2", index2, true, true );

        ThreadExecutor es = new ThreadExecutor();
        int threadNum =  10;
        for( int i = 0; i < threadNum; i++ ){
            es.addWorker( new testUpsert( num, insertData, matcher ) );
        }
        es.run();

        // 检查upsert结果
        ArrayList<BSONObject> expRecord = new ArrayList<>();
        ArrayList<BSONObject> actRecord = new ArrayList<>();
        expRecord.add( insertData );
        if( num == 3 ) {
            expRecord.clear();
            expRecord.add( existData );
            insertData.put( "c", 3 );
            expRecord.add( insertData );
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
