package com.sequoiadb.fulltext.parallel;

import java.util.Date;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15880:truncate集合记录与阻塞sync并发
 * @Author huangxiaoni
 * @Date 2019.5.14
 */

public class Fulltext15880 extends FullTestBase {
    private Random random = new Random();
    private final String CS_NAME = "cs_es_15880";
    private final String CL_NAME = "cl_es_15880";
    private final String IDX_NAME = "idx_es_15880";
    private final BSONObject IDX_KEY = new BasicBSONObject( "a", "text" );
    private final int RECS_NUM = 20000;

    private String cappedCSName;

    private String esIndexName;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CSNAME, CS_NAME );
        caseProp.setProperty( CLNAME, CL_NAME );
    }

    @Override
    protected void caseInit() throws Exception {
        cl.createIndex( IDX_NAME, IDX_KEY, false, false );
        FullTextDBUtils.insertData( cl, RECS_NUM );
        cappedCSName = FullTextDBUtils.getCappedName( cl, IDX_NAME );
        esIndexName = FullTextDBUtils.getESIndexName( cl, IDX_NAME );
    }

    @Test
    private void test() throws Exception {

        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        ThreadTruncate threadTruncate = new ThreadTruncate();
        ThreadDBSync threadDBSync = new ThreadDBSync();
        es.addWorker( threadTruncate );
        es.addWorker( threadDBSync );
        es.run();

        // check results
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, IDX_NAME, 0 ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadTruncate extends ResultStore {
        @ExecuteOrder(step = 1)
        private void truncate() throws InterruptedException {
            Thread.sleep( random.nextInt( 100 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( CS_NAME )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.truncate();
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            }
        }
    }

    private class ThreadDBSync {
        @ExecuteOrder(step = 1)
        private void sync() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                BSONObject options = new BasicBSONObject();
                options.put( "CollectionSpace", CS_NAME );
                options.put( "Block", true );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                for ( int i = 0; i < 3; i++ ) {
                    db.sync( options );
                }
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }
}
