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
 * @Descreption seqDB-30378:并发执行upsert前，已存在相同主键的记录导致冲突
 * @Author Cheng Jingjing
 * @CreateDate 2023/03/07
 * @Version 1.0
 */
public class TestUpsert30378 extends SdbTestBase {
    private String csName = "cs_30378";
    private String clName = "cl_30378";
    private String indexName = "index_30378";
    private Sequoiadb db = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @DataProvider( name = "upsertData" )
    public Object[][] generateUpsertDatas() {
        // 场景一：集合已存在冲突记录， matcher与索引字段相同
        // upsert( { $set: { a: 1 } }, { a: 3 } ); index:{ a: 1 }
        BasicBSONObject existData1 = new BasicBSONObject();
        existData1.put( "a", 1 );
        BasicBSONObject insertData1 = new BasicBSONObject();
        insertData1.put( "a", 1 );
        BasicBSONObject matcher1 = new BasicBSONObject();
        matcher1.put( "a", 3 );
        BasicBSONObject index1 = new BasicBSONObject();
        index1.put( "a", 1 );

        // 场景二：集合已存在冲突记录， matcher包含部分索引字段
        // upsert( { $set: { a: 1, b: 1 } }, { a: 3, b: 1 } ); index:{ a: 1 }
        BasicBSONObject existData2 = new BasicBSONObject();
        existData2.put( "a", 1 );
        BasicBSONObject insertData2 = new BasicBSONObject();
        insertData2.put( "a", 1 );
        insertData2.put( "b", 1 );
        BasicBSONObject matcher2 = new BasicBSONObject();
        matcher2.put( "a", 3 );
        BasicBSONObject index2 = new BasicBSONObject();
        index2.put( "a", 1 );

        // 场景三：集合已存在冲突记录， matcher不包含索引字段
        // upsert( { $set: { a: 1, b: 1, c: 3 }, { a: 1 } ); index:{ b: 1 }
        BasicBSONObject existData3 = new BasicBSONObject();
        existData3.put( "a", 1 );
        existData3.put( "b", 1 );
        BasicBSONObject insertData3 = new BasicBSONObject();
        insertData3.put( "a", 1 );
        insertData3.put( "b", 1 );
        insertData3.put( "b", 3 );
        BasicBSONObject matcher3 = new BasicBSONObject();
        matcher3.put( "a", 1 );
        matcher3.put( "b", 1 );
        BasicBSONObject index3 = new BasicBSONObject();
        index3.put( "a", 1 );
        index3.put( "b", 1 );

        // 场景四：嵌套场景 集合已存在冲突记录， matcher与索引字段相同
        // upsert( { $set: { a: { b: 1 } }, { a: { b: 3 } } ); index:{ a: { b: 1 } }
        BasicBSONObject existData4 = new BasicBSONObject();
        BasicBSONObject existData4_1 = new BasicBSONObject();
        existData4_1.put( "b", 1 );
        existData4.put( "a", existData4_1 );
        BasicBSONObject insertData4 = new BasicBSONObject();
        BasicBSONObject insertData4_1 = new BasicBSONObject();
        insertData4_1.put( "b", 1 );
        insertData4.put( "a", insertData4_1 );
        BasicBSONObject matcher4 = new BasicBSONObject();
        BasicBSONObject matcher4_1 = new BasicBSONObject();
        matcher4_1.put( "b", 3 );
        matcher4.put( "a", matcher4_1 );
        BasicBSONObject index4 = new BasicBSONObject();
        index4.put( "a.b", 1 );

        // 场景五：嵌套场景 集合已存在冲突记录， matcher包含部分索引字段
        // upsert( { $set: { a: { b: 1 } }, c: 1 }, { a: { b: 3 } } ); index:{ a: { b: 1 } }
        BasicBSONObject existData5 = new BasicBSONObject();
        BasicBSONObject existData5_1 = new BasicBSONObject();
        existData5_1.put( "b", 1 );
        existData5.put( "a", existData5_1 );
        BasicBSONObject insertData5 = new BasicBSONObject();
        BasicBSONObject insertData5_1 = new BasicBSONObject();
        insertData5_1.put( "b", 1 );
        insertData5.put( "a", insertData4_1 );
        insertData5.put( "c", 1 );
        BasicBSONObject matcher5 = new BasicBSONObject();
        BasicBSONObject matcher5_1 = new BasicBSONObject();
        matcher5_1.put( "b", 3 );
        matcher5.put( "a", matcher5_1 );
        BasicBSONObject index5 = new BasicBSONObject();
        index5.put( "a.b", 1 );

        // 场景六：嵌套场景 集合已存在冲突记录， matcher不包含索引字段
        // upsert( { $set: { a: 1 }, { a: { b: 1 } } ); index:{ a: 1 }
        BasicBSONObject existData6 = new BasicBSONObject();
        existData6.put( "a", 1 );
        BasicBSONObject insertData6 = new BasicBSONObject();
        insertData6.put( "a", 1 );
        BasicBSONObject matcher6 = new BasicBSONObject();
        BasicBSONObject matcher6_1 = new BasicBSONObject();
        matcher6_1.put( "b", 1 );
        matcher6.put( "a", matcher6_1 );
        BasicBSONObject index6 = new BasicBSONObject();
        index6.put( "a", 1 );

        return new Object[][] { new Object[] { 1, existData1, insertData1, matcher1, index1 },
                new Object[] { 2, existData2, insertData2, matcher2, index2 },
                new Object[] { 3, existData3, insertData3, matcher3, index3 },
                new Object[] { 4, existData4, insertData4, matcher4, index4 },
                new Object[] { 5, existData5, insertData5, matcher5, index5 },
                new Object[] { 6, existData6, insertData6, matcher6, index6 } };

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
    private void test( int num, BasicBSONObject existData, BasicBSONObject insertData, BasicBSONObject matcher, BasicBSONObject index ) throws Exception {
        if( cs.isCollectionExist( clName+ "_" + num ) ){
            cs.dropCollection( clName + "_" + num );
        }
        // 创建集合，插入数据
        cl = cs.createCollection( clName + "_"+ num );
        cl.insertRecord( existData );
        // 创建索引
        cl.createIndex(indexName + "_" + num, index, true, true );
        ThreadExecutor es = new ThreadExecutor();
        int threadNum =  10;
        for( int i = 0; i < threadNum; i++ ){
            es.addWorker( new testUpsert( num, insertData, matcher ) );
        }
        es.run();

        // 检查upsert结果
        ArrayList<BSONObject> expRecord = new ArrayList<>();
        ArrayList<BSONObject> actRecord = new ArrayList<>();
        expRecord.add( existData );
        if ( num == 3 ) {
            expRecord.clear();
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
