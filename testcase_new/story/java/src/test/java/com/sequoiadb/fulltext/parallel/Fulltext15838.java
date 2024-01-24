package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName FullText15838.java 创建全文索引与split并发
 * @Author luweikang
 * @Date 2019年5月6日
 */
public class Fulltext15838 extends FullTestBase {
    private CollectionSpace cs = null;
    private String clName = "es_15838";
    private String indexName = "fulltextIndex15838";
    private String sourceGruop = null;
    private String targetGruop = null;
    private int insertNum = 100000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( IGNOREONEGROUP, "true" );
    }

    @Override
    protected void caseInit() throws Exception {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        sourceGruop = groupNames.get( 0 );
        targetGruop = groupNames.get( 1 );
        cs = sdb.getCollectionSpace( csName );
        BSONObject options = new BasicBSONObject();
        options.put( "ShardingType", "range" );
        options.put( "ShardingKey", new BasicBSONObject( "recordId", 1 ) );
        options.put( "Group", sourceGruop );
        cl = cs.createCollection( clName, options );

        FullTextDBUtils.insertData( cl, insertNum );
    }

    @Test
    public void test() throws Exception {

        ThreadExecutor thread = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thread.addWorker( new CreateIndexThread() );
        thread.addWorker( new SplitThread() );
        thread.run();

        checkSplitResult();

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

    @Override
    protected void caseFini() throws Exception {
        List< String > cappedNames = new ArrayList< String >();
        cappedNames.add( FullTextDBUtils.getCappedName( cl, indexName ) );
        List< String > esIndexNames = FullTextDBUtils.getESIndexNames( cl,
                indexName );
        FullTextDBUtils.dropCollection( cs, clName );
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

    private class SplitThread {

        @ExecuteOrder(step = 1)
        private void splitTable() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( sourceGruop, targetGruop, 50 );
            }
        }
    }

    private void checkSplitResult() {
        ReplicaGroup sourceRG = sdb.getReplicaGroup( sourceGruop );
        ReplicaGroup targetRG = sdb.getReplicaGroup( targetGruop );
        Sequoiadb sdb1 = sourceRG.getMaster().connect();
        Sequoiadb sdb2 = targetRG.getMaster().connect();
        DBCollection cl1 = sdb1.getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection cl2 = sdb2.getCollectionSpace( csName )
                .getCollection( clName );

        Assert.assertEquals( cl1.getCount(), insertNum / 2 );
        Assert.assertEquals( cl2.getCount(), insertNum / 2 );
    }

}
