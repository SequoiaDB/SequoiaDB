package com.sequoiadb.fulltext.parallel;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName FullText15836.java 创建全文索引与sync操作并发
 * @Author luweikang
 * @Date 2019年5月6日
 */
public class Fulltext15836 extends FullTestBase {

    private String csName = "cs_15836";
    private String clName = "es_15836";
    private String indexName = "fulltextIndex15836";
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

        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thread.addWorker( new CreateIndexThread() );
        thread.addWorker( new SyncThread() );
        thread.run();

        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, insertNum ) );

        cappedName = FullTextDBUtils.getCappedName( cl, indexName );
        esIndexName = FullTextDBUtils.getESIndexName( cl, indexName );

        FullTextDBUtils.insertData( cl, 10000 );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, indexName,
                insertNum + 10000 ) );

        int recordNum = 0;
        DBCursor cur = cl.query(
                "{'': {'$Text': {'query': {'match_all': {}}}}}", null,
                "{'recordId': 1}", "{'': '" + indexName + "'}" );
        while ( cur.hasNext() ) {
            cur.getNext();
            recordNum++;
        }
        cur.close();

        Assert.assertEquals( recordNum, insertNum + 10000,
                "use fulltext index search record" );
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

    private class SyncThread {
        @ExecuteOrder(step = 1)
        private void syncData() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject options = new BasicBSONObject();
                options.put( "Block", true );
                options.put( "CollectionSpace", csName );
                db.sync( options );
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

}
