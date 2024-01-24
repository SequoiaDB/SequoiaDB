package com.sequoiadb.fulltext.parallel;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
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
 * @FileName seqDB-15859:集合中存在全文索引，并发全文检索/普通查询记录
 * @Author huangxiaoni
 * @Date 2019.5.8
 */

public class Fulltext15859 extends FullTestBase {
    private final int THREAD_NUM = 2;
    private final String CL_NAME = "cl_es_15859";
    private final String IDX_NAME = "idx_es_15859";
    private final BSONObject IDX_KEY = ( BSONObject ) JSON
            .parse( "{a:'text',b:'text',c:'text',d:'text'}" );
    private final int RECS_NUM = 50000;
    private String cappedCSName;

    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, CL_NAME );
    }

    @Override
    protected void caseInit() throws Exception {
        cl.createIndex( IDX_NAME, IDX_KEY, false, false );
        cappedCSName = FullTextDBUtils.getCappedName( cl, IDX_NAME );
        esIndexName = FullTextDBUtils.getESIndexName( cl, IDX_NAME );

        FullTextDBUtils.insertData( cl, RECS_NUM );

        // 确保预置的数据同步到es完成，避免test中查询的数据未同步完成导致非预期
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, IDX_NAME, RECS_NUM ) );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        for ( int i = 0; i < THREAD_NUM; i++ ) {
            es.addWorker( new ThreadFullTextSearch() );
            es.addWorker( new ThreadQuery() );
        }
        es.run();

        // check consistency
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, IDX_NAME, RECS_NUM ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadFullTextSearch {
        private int rcRecsNum = 0;

        @ExecuteOrder(step = 1, desc = "全文检索")
        private void fullTextSearch() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                BSONObject matcher = new BasicBSONObject( "",
                        new BasicBSONObject( "$Text",
                                new BasicBSONObject( "query",
                                        new BasicBSONObject( "match",
                                                new BasicBSONObject( "a",
                                                        CL_NAME ) ) ) ) );
                BSONObject orderby = new BasicBSONObject( "a", 1 );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                DBCursor cursor = cl2.query( matcher, null, orderby, null );
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                    rcRecsNum++;
                }
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            }
        }

        @ExecuteOrder(step = 2, desc = "检查查询返回结果")
        private void checkResult() {
            Assert.assertEquals( rcRecsNum, RECS_NUM );
        }
    }

    private class ThreadQuery {
        private int rcRecsNum = 0;

        @ExecuteOrder(step = 1, desc = "普通查询")
        private void fullTextSearch() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                BSONObject matcher = new BasicBSONObject( "a",
                        new BasicBSONObject( "$exists", 1 ) );
                BSONObject orderby = new BasicBSONObject( "a", 1 );
                BSONObject hint = new BasicBSONObject( "", IDX_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                DBCursor cursor = cl2.query( matcher, null, orderby, hint );
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                    rcRecsNum++;
                }
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            }
        }

        @ExecuteOrder(step = 2, desc = "检查查询返回结果")
        private void checkResult() {
            Assert.assertEquals( rcRecsNum, RECS_NUM );
        }
    }
}
