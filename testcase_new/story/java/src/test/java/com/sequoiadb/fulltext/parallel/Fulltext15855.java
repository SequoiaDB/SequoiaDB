package com.sequoiadb.fulltext.parallel;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15855:集合中存在全文索引，并发插入记录
 * @Author huangxiaoni
 * @Date 2019.4.25
 */

public class Fulltext15855 extends FullTestBase {
    private final int THREAD_NUM = 2;
    private final String CL_NAME = "cl_es_15855";
    private final String IDX_NAME = "idx_es_15855";
    private final BSONObject IDX_KEY = new BasicBSONObject( "a", "text" );
    private final int RECS_NUM = 20000;
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
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        for ( int i = 0; i < THREAD_NUM; i++ ) {
            es.addWorker( new ThreadInsert() );
        }
        es.run();

        // check results
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, IDX_NAME,
                RECS_NUM * THREAD_NUM ) );

        BSONObject matcher = new BasicBSONObject( "",
                new BasicBSONObject( "$Text",
                        new BasicBSONObject( "query", new BasicBSONObject(
                                "match",
                                new BasicBSONObject( "a", CL_NAME ) ) ) ) );
        int cnt = ( int ) cl.getCount( matcher );
        Assert.assertEquals( cnt, RECS_NUM * THREAD_NUM );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadInsert {
        @ExecuteOrder(step = 1)
        private void insert() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                FullTextDBUtils.insertData( cl2, RECS_NUM );
            }
        }
    }
}
