package com.sequoiadb.fulltext.parallel;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;
import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
/**
 * @FileName seqDB-22227:全文索引数据操作与唯一索引数据备节点并发重放并发
 * @Author Zhao Xiaoni
 * @Date 2020.5.28
 */
public class Fulltext22227 extends FullTestBase {
    private String groupName;
    private DBCollection cl1 = null;
    private DBCollection cl2 = null;
    private final String clName1 = "cl22227_1";
    private final String clName2 = "cl22227_2";
    private String cappedCSName;
    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
    }

    @Override
    protected void caseInit() throws Exception {
        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        BSONObject clOptions = new BasicBSONObject( "Group", groupName );
        cl1 = sdb.getCollectionSpace( csName ).createCollection( clName1, clOptions );
        cl2 = sdb.getCollectionSpace( csName ).createCollection( clName2, clOptions );
        cl1.createIndex( "index_es_22227", "{ 'a': 'text' }", false, false );
        cl2.createIndex( "index_unique_22227", "{ 'a': 1 }", true, false );
        cappedCSName = FullTextDBUtils.getCappedName( cl1, "index_es_22227" );
        esIndexName = FullTextDBUtils.getESIndexName( cl1, "index_es_22227" );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl1, "index_es_22227", 0 ) );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        ThreadFulltext threadFulltext = new ThreadFulltext();
        ThreadUnique threadUnique = new ThreadUnique();
        es.addWorker( threadFulltext );
        es.addWorker( threadUnique );
        es.run();

        // check results
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl1, "index_es_22227", 110000 ) );
    }

    @Override
    protected void caseFini() throws Exception {
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        FullTextDBUtils.dropCollection( cs, clName1 );
        FullTextDBUtils.dropCollection( cs, clName2 );
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadFulltext extends ResultStore {
        @ExecuteOrder(step = 1)
        private void fulltext() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName1 );
                for( int i = 0; i < 110000; i++ ){
                    cl.insert( "{ 'a': 'a_" + i + "' }" );
                }
            }
        }
    }
    
    private class ThreadUnique extends ResultStore {
        @ExecuteOrder(step = 1)
        private void unique() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName2 );
                for( int i = 0; i < 20000; i++ ){
                    cl.delete( "{ 'a': 1 }" );
                    cl.insert( "{ 'a': 1 }" );
                    cl.delete( "{ 'a': 10 }" );
                    cl.insert( "{ 'a': 10 }" );
                    cl.delete( "{ 'a': 100 }" );
                    cl.insert( "{ 'a': 100 }" );
                }
            }
        }
    }
}