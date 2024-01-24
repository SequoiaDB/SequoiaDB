package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
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
 * @FileName seqDB-15857:集合中存在全文索引，并发删除记录
 * @Author huangxiaoni
 * @Date 2019.5.8
 */

public class Fulltext15857 extends FullTestBase {
    private final int THREAD_NUM = 5;
    private final String CL_NAME = "cl_es_15857";
    private final String IDX_NAME = "idx_es_15857";
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

        // 确保预置的数据同步到es完成
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, IDX_NAME, RECS_NUM ) );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        for ( int i = 0; i < THREAD_NUM; i++ ) {
            int batchRecsNum = RECS_NUM / THREAD_NUM;
            BSONObject obj1 = new BasicBSONObject( "recordId",
                    new BasicBSONObject( "$gte", batchRecsNum * i ) );
            BSONObject obj2 = new BasicBSONObject( "recordId",
                    new BasicBSONObject( "$lt", batchRecsNum * ( i + 1 ) ) );

            ArrayList< BSONObject > and = new ArrayList<>();
            and.add( obj1 );
            and.add( obj2 );

            BSONObject matcher = new BasicBSONObject( "$and", and );
            es.addWorker( new ThreadDelete( matcher ) );
        }
        es.run();

        // check consistency
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, IDX_NAME, 0 ) );

        // check total count
        long updCnt = cl.getCount();
        Assert.assertEquals( updCnt, 0 );

        // check fullTextSearch
        BSONObject matcher = new BasicBSONObject( "",
                new BasicBSONObject( "$Text",
                        new BasicBSONObject( "query", new BasicBSONObject(
                                "match_all", new BasicBSONObject() ) ) ) );
        DBCursor cursor = cl.query( matcher, null, null, null );
        Assert.assertFalse( cursor.hasNext() );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadDelete {
        private BSONObject matcher;

        private ThreadDelete( BSONObject matcher ) {
            this.matcher = matcher;
        }

        @ExecuteOrder(step = 1)
        private void delete() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                cl2.delete( matcher );
            }
        }
    }
}
