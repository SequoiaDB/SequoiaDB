package com.sequoiadb.fulltext.parallel;

import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName FullText15840.java 创建全文索引与全文检索并发
 * @Author luweikang
 * @Date 2019年5月10日
 */
public class Fulltext15840 extends FullTestBase {

    private String clName = "es_15840";
    private String indexName = "fulltextIndex15840";
    private int insertNum = 20000;
    private String cappedName;
    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        FullTextDBUtils.insertData( cl, insertNum );
    }

    @Test
    public void test() throws Exception {

        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thread.addWorker( new CreateIndexThread() );
        thread.addWorker( new QueryByTextIndexThread() );
        thread.run();

        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, insertNum ) );

        int recordNum = 0;
        DBCursor cur = cl.query(
                "{'': {'$Text': {'query': {'match_all': {}}}}}", null,
                "{'recordId': 1}", "{'': '" + indexName + "'}" );
        while ( cur.hasNext() ) {
            cur.getNext();
            recordNum++;
        }
        cur.close();

        Assert.assertEquals( recordNum, insertNum,
                "use fulltext index search record" );
        cappedName = FullTextDBUtils.getCappedName( cl, indexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    private class CreateIndexThread {

        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject indexObj = new BasicBSONObject();
                indexObj.put( "a", "text" );
                indexObj.put( "b", "text" );
                indexObj.put( "c", "text" );
                indexObj.put( "d", "text" );
                indexObj.put( "e", "text" );
                cl.createIndex( indexName, indexObj, false, false );
            }
        }
    }

    private class QueryByTextIndexThread {

        @ExecuteOrder(step = 1)
        private void queryData() throws InterruptedException {
            for ( int i = 0; i < 10; i++ ) {
                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    DBCollection cl = db.getCollectionSpace( csName )
                            .getCollection( clName );
                    DBCursor cur = cl.query(
                            "{'': {'$Text': {'query': {'match_all': {}}}}}",
                            null, "{'a': 1}", "{'': '" + indexName + "'}" );
                    while ( cur.hasNext() ) {
                        cur.getNext();
                    }
                    cur.close();
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -6 && e.getErrorCode() != -52 
                           && e.getErrorCode() != -10 ) {
                        Assert.fail( e.getMessage() );
                    }
                }
                Thread.sleep( 1000 + new Random().nextInt( 500 ) );
            }
        }
    }

}
