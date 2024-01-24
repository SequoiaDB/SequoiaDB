package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15856:集合中存在全文索引，并发更新记录
 * @Author huangxiaoni
 * @Date 2019.5.8
 */

public class Fulltext15856 extends FullTestBase {
    private final int THREAD_NUM = 10;
    private final String CL_NAME = "cl_es_15856";
    private final String IDX_NAME = "idx_es_15856";
    private final BSONObject IDX_KEY = ( BSONObject ) JSON
            .parse( "{a:'text',b:'text',c:'text'}" );
    private final int RECS_NUM = 30000;
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
    }

    @Test
    private void test() throws Exception {
        String modVal = "abcdefghijklmnopq";
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        for ( int i = 0; i < THREAD_NUM; i++ ) {
            // modifier
            BSONObject modifier = new BasicBSONObject( "$set",
                    new BasicBSONObject( "b", modVal + i ) );
            // matcher
            int batchRecsNum = RECS_NUM / THREAD_NUM;
            BSONObject obj1 = new BasicBSONObject( "recordId",
                    new BasicBSONObject( "$gte", batchRecsNum * i ) );
            BSONObject obj2 = new BasicBSONObject( "recordId",
                    new BasicBSONObject( "$lt", batchRecsNum * ( i + 1 ) ) );

            ArrayList< BSONObject > and = new ArrayList<>();
            and.add( obj1 );
            and.add( obj2 );
            BSONObject matcher = new BasicBSONObject( "$and", and );
            // update
            es.addWorker( new ThreadUpdate( modifier, matcher ) );
        }
        es.run();

        // check consistency
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, IDX_NAME, RECS_NUM ) );

        // check update records
        for ( int i = 0; i < THREAD_NUM; i++ ) {
            BSONObject matcher = new BasicBSONObject( "", new BasicBSONObject(
                    "$Text",
                    new BasicBSONObject( "query", new BasicBSONObject( "match",
                            new BasicBSONObject( "b", modVal + i ) ) ) ) );
            int updCnt = ( int ) cl.getCount( matcher );
            Assert.assertEquals( updCnt, RECS_NUM / THREAD_NUM );
        }
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadUpdate {
        private BSONObject modifier;
        private BSONObject matcher;

        private ThreadUpdate( BSONObject modifier, BSONObject matcher ) {
            this.modifier = modifier;
            this.matcher = matcher;
        }

        @ExecuteOrder(step = 1)
        private void update() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                BSONObject hint = new BasicBSONObject( "", IDX_NAME );
                cl2.update( matcher, modifier, hint );
            } catch ( BaseException e ) {
                throw e;
            }
        }
    }
}
