package com.sequoiadb.fulltext.parallel;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
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
 * @FileName FullText15845.java 删除全文索引与truncate并发
 * @Author luweikang
 * @Date 2019年5月10日
 */
public class Fulltext15845 extends FullTestBase {
    private String clName = "es_15845";
    private String indexName = "fulltextIndex15845";
    private String cappedName = null;
    private String esIndexName = null;
    private int insertNum = 100000;

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
        DropIndexThread dropIndexThread = new DropIndexThread();
        thread.addWorker( dropIndexThread );
        thread.addWorker( new TruncateThread() );
        thread.run();

        Assert.assertEquals( cl.getCount(), 0,
                "cl be truncate, should no record." );
        if ( dropIndexThread.getRetCode() != 0 ) {
            Assert.assertTrue(
                    FullTextUtils.isIndexCreated( cl, indexName, 0 ) );
            cl.dropIndex( indexName );
        }
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    private class DropIndexThread extends ResultStore {

        @ExecuteOrder(step = 1)
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( indexName );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -321, e.getMessage() );
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class TruncateThread {

        @ExecuteOrder(step = 1)
        private void truncate() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncate();
            }
        }
    }

}
