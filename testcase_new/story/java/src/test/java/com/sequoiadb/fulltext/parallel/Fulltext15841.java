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
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName FullText15841.java 创建全文索引与truncate并发
 * @Author luweikang
 * @Date 2019年5月10日
 */
public class Fulltext15841 extends FullTestBase {

    private String clName = "es_15841";
    private String indexName = "fulltextIndex15841";
    private BSONObject indexObj = null;
    private int insertNum = 100000;
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

        indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        CreateIndexThread createIndexThread = new CreateIndexThread();
        TruncateThread truncateThread = new TruncateThread();
        thread.addWorker( createIndexThread );
        thread.addWorker( truncateThread );
        thread.run();
        if ( createIndexThread.getRetCode() != 0 ) {
            cl.createIndex( indexName, indexObj, false, false );
        }
        if ( truncateThread.getRetCode() == 0 ) {
            Assert.assertTrue(
                    FullTextUtils.isIndexCreated( cl, indexName, 0 ) );
        } else {
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
        }
        cappedName = FullTextDBUtils.getCappedName( cl, indexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    private class CreateIndexThread extends ResultStore {

        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( indexName, indexObj, false, false );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -321 ) {
                    throw e;
                }
                saveResult( -1, e );
            }
        }
    }

    private class TruncateThread extends ResultStore {

        @ExecuteOrder(step = 1)
        private void truncate() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                Thread.sleep( 1000 + new Random().nextInt( 100 ) );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncate();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    throw e;
                }
                saveResult( -1, e );
            }
        }
    }

}
