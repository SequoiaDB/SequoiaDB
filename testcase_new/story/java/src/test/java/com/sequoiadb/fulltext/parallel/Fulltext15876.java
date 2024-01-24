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
 * @FileName seqDB-15876:truncate集合记录与删除集合空间并发
 * @Author huangxiaoni
 * @Date 2019.5.14
 */

public class Fulltext15876 extends FullTestBase {
    private Random random = new Random();
    private final String CS_NAME = "cs_es_15876";
    private final String CL_NAME = "cl_es_15876";
    private final String IDX_NAME = "idx_es_15876";
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
        cappedCSName = FullTextDBUtils.getCappedName( cl, IDX_NAME );
        esIndexName = FullTextDBUtils.getESIndexName( cl, IDX_NAME );

        FullTextDBUtils.insertData( cl, RECS_NUM );
    }

    // SEQUOIADBMAINSTREAM-4558
    @Test(enabled = false) // jira-4558
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        ThreadTruncate threadTruncate = new ThreadTruncate();
        ThreadDropCS threadDropCS = new ThreadDropCS();
        es.addWorker( threadTruncate );
        es.addWorker( threadDropCS );
        es.run();

        // check results
        if ( threadDropCS.getRetCode() == 0 ) {
            Assert.assertFalse( sdb.isCollectionSpaceExist( CS_NAME ) );
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedCSName ) );
        } else if ( threadDropCS.getRetCode() != 0 ) {
            int cnt = ( int ) cl.getCount();
            if ( threadTruncate.getRetCode() == 0 ) {
                Assert.assertEquals( cnt, 0 );
            } else if ( threadTruncate.getRetCode() != 0 ) {
                Assert.assertEquals( cnt, RECS_NUM );
            }
            Assert.assertTrue(
                    FullTextUtils.isIndexCreated( cl, IDX_NAME, cnt ) );
        }
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadTruncate extends ResultStore {
        @ExecuteOrder(step = 1)
        private void truncate() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( CS_NAME )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.truncate();
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -34 && e.getErrorCode() != -23
                        && e.getErrorCode() != -248 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
                saveResult( -1, e );
            }
        }
    }

    private class ThreadDropCS extends ResultStore {
        @ExecuteOrder(step = 1)
        private void dropCS() throws InterruptedException {
            Thread.sleep( random.nextInt( 10 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                db.dropCollectionSpace( CS_NAME );
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190
                        && e.getErrorCode() != -321 ) {
                    throw e;
                }
                saveResult( -1, e );
            }
        }
    }
}