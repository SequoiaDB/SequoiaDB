package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.List;

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
 * @FileName FullText15835.java 删除全文索引与alter操作并发
 * @Author luweikang
 * @Date 2019年5月6日
 */
public class Fulltext15835 extends FullTestBase {
    private String clName = "es_15835";
    private String indexName = "fulltextIndex15835";
    private List< String > cappedNames = new ArrayList< String >();
    private List< String > esIndexNames = null;
    private int insertNum = 50000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( IGNOREONEGROUP, "true" );
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

        String cappedName = FullTextDBUtils.getCappedName( cl, indexName );
        cappedNames.add( cappedName );
        esIndexNames = FullTextDBUtils.getESIndexNames( cl, indexName );

        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thread.addWorker( new DropIndexThread() );
        thread.addWorker( new AlterTableThread() );
        thread.run();

        checkSnapshotResult();
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                cappedNames ) );

        FullTextDBUtils.insertData( cl, 10000 );

        Assert.assertEquals( cl.getCount(), insertNum + 10000,
                "check insert recorf after drop index" );

        DBCursor cur = null;
        try {
            cur = cl.query( "{'': {'$Text': {'query': {'match_all': {}}}}}",
                    null, "{'recordId': 1}", "{'': '" + indexName + "'}" );
            if ( cur.hasNext() ) {
                cur.getNext();
            }
            Assert.fail( "use not exist fulltext search should be failed!" );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -52 && e.getErrorCode() != -10 ) { 
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
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                cappedNames ) );
    }

    private class DropIndexThread {

        @ExecuteOrder(step = 1)
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( indexName );
            }
        }
    }

    private class AlterTableThread {

        @ExecuteOrder(step = 1)
        private void alterTable() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject options = new BasicBSONObject();
                options.put( "ShardingType", "hash" );
                options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
                options.put( "AutoSplit", true );
                cl.alterCollection( options );
            }
        }
    }

    private void checkSnapshotResult() {
        DBCursor snap = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                new BasicBSONObject( "Name", csName + "." + clName ), null,
                null );
        BSONObject clOption = snap.getNext();
        String shardingType = ( String ) clOption.get( "ShardingType" );
        BSONObject shardingKey = ( BSONObject ) clOption.get( "ShardingKey" );
        snap.close();

        Assert.assertEquals( shardingType, "hash" );
        Assert.assertEquals( shardingKey, new BasicBSONObject( "a", 1 ) );
    }
}
