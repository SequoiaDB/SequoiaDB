package com.sequoiadb.fulltext.parallel;

import java.util.Date;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
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
 * @FileName seqDB-15875:truncate集合记录与删除集合并发
 * @Author huangxiaoni
 * @Date 2019.5.14
 */

public class Fulltext15875 extends FullTestBase {
    private Random random = new Random();
    private final String CL_NAME = "cl_es_15875";
    private final String IDX_NAME = "idx_es_15875";
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
        FullTextDBUtils.insertData( cl, RECS_NUM );
    }

    // SEQUOIADBMAINSTREAM-4558
    @Test(enabled = false) // jira-4558
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        ThreadTruncate threadTruncate = new ThreadTruncate();
        ThreadDropCL threadDropCL = new ThreadDropCL();
        es.addWorker( threadTruncate );
        es.addWorker( threadDropCL );
        es.run();

        // check results
        if ( threadDropCL.getRetCode() == 0 ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedCSName ) );
        } else {
            Assert.assertTrue(
                    FullTextUtils.isIndexCreated( cl, IDX_NAME, 0 ) );
        }
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCSName ) );
    }

    private class ThreadTruncate {
        @ExecuteOrder(step = 1)
        private void truncate() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl2 = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl2.truncate();
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                e.printStackTrace();
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -190
                        && e.getErrorCode() != -147 ) {
                    throw e;
                }
            }
        }
    }

    private class ThreadDropCL extends ResultStore {
        @ExecuteOrder(step = 1)
        private void alterCL() throws InterruptedException {
            Thread.sleep( random.nextInt( 10 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db
                        .getCollectionSpace( SdbTestBase.csName );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cs.dropCollection( CL_NAME );
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                e.printStackTrace();
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190
                        && e.getErrorCode() != -321 ) {
                    throw e;
                }
                saveResult( -1, e );
            }
        }
    }
}