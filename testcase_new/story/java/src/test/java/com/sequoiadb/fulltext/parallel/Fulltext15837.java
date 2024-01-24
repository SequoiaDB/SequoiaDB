package com.sequoiadb.fulltext.parallel;

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
 * @FileName FullText15837.java 删除全文索引与sync操作并发
 * @Author luweikang
 * @Date 2019年5月6日
 */
public class Fulltext15837 extends FullTestBase {
    private String csName = "cs_15837";
    private String clName = "es_15837";
    private String indexName = "fulltextIndex15837";
    private String cappedName = null;
    private String esIndexName = null;
    private int insertNum = 100000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CSNAME, csName );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        FullTextDBUtils.insertData( cl, insertNum );
    }

    @Test
    public void test() throws Exception {

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

        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thread.addWorker( new DropIndexThread() );
        thread.addWorker( new SyncThread() );
        thread.run();

        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );

        FullTextDBUtils.insertData( cl, 10000 );

        Assert.assertEquals( cl.getCount(), insertNum + 10000 );

        DBCursor cur = null;
        try {
            cur = cl.query( "{'': {'$Text': {'query': {'match_all': {}}}}}",
                    null, "{'recordId': 1}", "{'': '" + indexName + "'}" );
            if ( cur.hasNext() ) {
                cur.getNext();
            }
            Assert.fail( "use not exist fulltext search should be failed!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -6 && e.getErrorCode() != -52 ) {
                throw e;
            }
        } finally {
            if ( cur != null ) {
                cur.close();
            }
        }
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    private class DropIndexThread {

        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( indexName );
            }
        }
    }

    private class SyncThread {

        @ExecuteOrder(step = 1)
        private void syncData() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject options = new BasicBSONObject();
                options.put( "Block", true );
                options.put( "CollectionSpace", csName );
                try{
                    db.sync( options );
                }catch( BaseException e ){
                    e.printStackTrace();
                }
            }
        }
    }

}
