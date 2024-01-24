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
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName FullText15834.java 创建全文索引与alter操作并发
 * @Author luweikang
 * @Date 2019年5月6日
 */
public class Fulltext15834 extends FullTestBase {

    private String clName = "es_15834";
    private String indexName = "fulltextIndex15834";
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

        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thread.addWorker( new CreateIndexThread() );
        thread.addWorker( new AlterTableThread() );
        thread.run();

        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, insertNum ) );

        String cappedName = FullTextDBUtils.getCappedName( cl, indexName );
        cappedNames.add( cappedName );
        esIndexNames = FullTextDBUtils.getESIndexNames( cl, indexName );

        checkSnapshotResult();

        FullTextDBUtils.insertData( cl, insertNum );
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, indexName, insertNum * 2 ) );

        int recordNum = 0;
        DBCursor cur = cl.query(
                "{'': {'$Text': {'query': {'match_all': {}}}}}", null,
                "{'recordId': 1}", "{'': '" + indexName + "'}" );
        while ( cur.hasNext() ) {
            cur.getNext();
            recordNum++;
        }
        cur.close();

        Assert.assertEquals( recordNum, insertNum * 2,
                "use fulltext index search record" );
        Assert.assertTrue( FullTextUtils.isRecordEqualsByMulQueryMode( cl ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                cappedNames ) );
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

    private class AlterTableThread {

        @ExecuteOrder(step = 1)
        private void alterTable() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                BSONObject options = new BasicBSONObject();
                options.put( "ShardingType", "hash" );
                options.put( "ShardingKey",
                        new BasicBSONObject( "recordId", 1 ) );
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
        Assert.assertEquals( shardingKey,
                new BasicBSONObject( "recordId", 1 ) );
    }
}
