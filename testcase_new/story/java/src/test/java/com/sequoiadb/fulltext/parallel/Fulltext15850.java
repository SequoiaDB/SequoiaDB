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
 * @FileName FullText15850.java 集合中存在全文索引，全文检索记录时删除集合空间
 * @Author luweikang
 * @Date 2019年5月10日
 */
public class Fulltext15850 extends FullTestBase {
    private String csName = "cs_15850";
    private String clName = "es_15850";
    private String indexName = "fulltextIndex15850";
    private String cappedName = null;
    private String esIndexName = null;
    private int insertNum = 50000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CSNAME, csName );
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
        thread.addWorker( new DropCSThread() );
        thread.addWorker( new QueryByTextIndexThread() );
        thread.run();

        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ),
                "expect cs not exist." );
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );

    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );
    }

    private class DropCSThread {

        @ExecuteOrder(step = 1)
        private void dropCS() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.dropCollectionSpace( csName );
            }
        }
    }

    private class QueryByTextIndexThread {

        @ExecuteOrder(step = 1)
        private void queryData() throws InterruptedException {
            for ( int i = 0; i < 10; i++ ) {
                try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                        "" )) {
                    Thread.sleep( 1000 + new Random().nextInt( 500 ) );
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
                            && e.getErrorCode() != -34
                            && e.getErrorCode() != -10
                            && e.getErrorCode() != -248
                            && e.getErrorCode() != -36 ) {
                        throw e;
                    }
                }
            }
        }
    }

}
