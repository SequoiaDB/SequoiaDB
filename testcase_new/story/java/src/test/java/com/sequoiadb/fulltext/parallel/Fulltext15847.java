package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName FullText15847.java 增删改记录过程中反复创建删除全文索引
 * @Author luweikang
 * @Date 2019年5月10日
 */
public class Fulltext15847 extends FullTestBase {
    private String clName = "es_15847";
    private String indexName = "fulltextIndex15847";
    private String cappedName = null;
    private String esIndexName = null;
    private int insertNum = 40000;
    private int testInsertNum = 10000;
    private int updateNum = insertNum / 2;
    private int deleteNum = insertNum / 2;
    private boolean indexExist = true;
    private int expectIdxLid = 1;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        FullTextDBUtils.insertData( cl, insertNum );

        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        cl.createIndex( indexName, indexObj, false, false );

        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, insertNum ) );
        cappedName = FullTextDBUtils.getCappedName( cl, indexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thread.addWorker( new TextIndexThread() );
        thread.addWorker( new InsertThread() );
        thread.addWorker( new UpdateThread() );
        thread.addWorker( new DeleteThread() );
        thread.run();

        if ( indexExist ) {
            // 先校验索引的逻辑ID是否正常，再校验索引数据、主备节点数据的一致性
            Assert.assertTrue( FullTextUtils.isIdxLidSyncInES( esIndexName,
                    expectIdxLid ) );
            Assert.assertTrue( FullTextUtils.isIndexCreated( cl, indexName,
                    insertNum - deleteNum + testInsertNum ) );
            int recordNum = 0;
            DBCursor cur = cl.query(
                    "{'': {'$Text': {'query': {'match_all': {}}}}}", null,
                    "{'recordId': 1}", "{'': '" + indexName + "'}" );
            while ( cur.hasNext() ) {
                cur.getNext();
                recordNum++;
            }
            cur.close();

            Assert.assertEquals( recordNum, updateNum + testInsertNum,
                    "use fulltext index search record" );
        } else {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedName ) );
            DBCursor cur = null;
            try {
                cur = cl.query( "{'': {'$Text': {'query': {'match_all': {}}}}}",
                        null, "{'recordId': 1}", "{'': '" + indexName + "'}" );
                if ( cur.hasNext() ) {
                    cur.getNext();
                }
                Assert.fail(
                        "use not exist fulltext search should be failed!" );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -52, e.getMessage() );
            } finally {
                if ( cur != null ) {
                    cur.close();
                }
            }
        }

    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    private class TextIndexThread {

        @ExecuteOrder(step = 1)
        private void createAndDropIndex() {
            BSONObject indexObj = new BasicBSONObject();
            indexObj.put( "a", "text" );
            indexObj.put( "b", "text" );
            indexObj.put( "c", "text" );
            indexObj.put( "d", "text" );
            indexObj.put( "e", "text" );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 5; i++ ) {
                    cl.dropIndex( indexName );
                    indexExist = false;
                    cl.createIndex( indexName, indexObj, false, false );
                    indexExist = true;
                    // 当成功创建一次全文索引，次数加1
                    expectIdxLid++;

                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    throw e;
                }
            }
        }
    }

    private class InsertThread {

        @ExecuteOrder(step = 1)
        private void insert() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                List< BSONObject > insertObjs = new ArrayList< BSONObject >();
                String strB = StringUtils.getRandomString( 8 );
                String strC = StringUtils.getRandomString( 32 );
                String strD = StringUtils.getRandomString( 64 );
                int insertNum1 = insertNum + testInsertNum / 10;
                for ( int i = 0; i < 10; i++ ) {
                    for ( int j = insertNum; j < insertNum1; j++ ) {
                        int recordNum = i * ( testInsertNum / 10 ) + j;
                        insertObjs.add( ( BSONObject ) JSON.parse( "{recordId: "
                                + recordNum + ", a: '" + clName + recordNum
                                + "', b: '" + strB + "', c: '" + strC
                                + "', d: '" + strD + "'}" ) );
                    }
                    cl.insert( insertObjs, 0 );
                    insertObjs.clear();
                }
            }
        }
    }

    private class UpdateThread {

        @ExecuteOrder(step = 1)
        private void update() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.update( "{recordId: {$gte: 0, $lt: " + updateNum + "}}",
                        "{$set: {b: 'text'}}", null );
            }
        }
    }

    private class DeleteThread {

        @ExecuteOrder(step = 1)
        private void delete() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.delete( "{recordId: {$gte: " + deleteNum + ", $lt: "
                        + insertNum + "}}" );
            }
        }
    }

}
