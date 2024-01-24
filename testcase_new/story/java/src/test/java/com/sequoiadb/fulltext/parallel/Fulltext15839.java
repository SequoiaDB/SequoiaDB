package com.sequoiadb.fulltext.parallel;

import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
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
 * @FileName FullText15839.java 删除全文索引与split操作并发
 * @Author luweikang
 * @Date 2019年5月6日
 */
public class Fulltext15839 extends FullTestBase {
    private CollectionSpace cs = null;
    private String clName = "es_15839";
    private String indexName = "fulltextIndex15839";
    private String cappedName = null;
    private String esIndexName = null;
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
        thread.addWorker( new DropIndexThread() );
        thread.addWorker( new SplitThread() );
        thread.run();

        checkSplitResult();

        Assert.assertTrue(
                FullTextUtils.isIndexDeleted( sdb, esIndexName, cappedName ) );

    }

    @Override
    protected void caseFini() throws Exception {
        FullTextDBUtils.dropCollection( cs, clName );
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
