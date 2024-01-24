package com.sequoiadb.fulltext.parallel;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15879:truncate集合记录与split并发
 * @Author huangxiaoni
 * @Date 2019.5.14
 */

public class Fulltext15879 extends FullTestBase {
    private Random random = new Random();
    private final String CL_NAME = "cl_es_15879";
    private final String IDX_NAME = "idx_es_15879";
    private final BSONObject IDX_KEY = new BasicBSONObject( "a", "text" );
    private final int RECS_NUM = 20000;

    private CollectionSpace cs;
    private DBCollection cl;
    private List< String > cappedCSNames = new ArrayList< String >();
    private String srcRgName;
    private String dstRgName;

    private List< String > esIndexNames;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
    }

    @Override
    protected void caseInit() throws Exception {
        ArrayList< String > rgNames = CommLib.getDataGroupNames( sdb );
        srcRgName = rgNames.get( 0 );
        dstRgName = rgNames.get( 1 );

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        BSONObject options = new BasicBSONObject();
        options.put( "ShardingType", "hash" );
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "Group", srcRgName );
        cl = cs.createCollection( CL_NAME, options );
        cl.createIndex( IDX_NAME, IDX_KEY, false, false );
        cappedCSNames.add( FullTextDBUtils.getCappedName( cl, IDX_NAME ) );
        esIndexNames = FullTextDBUtils.getESIndexNames( cl, IDX_NAME );

        FullTextDBUtils.insertData( cl, RECS_NUM );

        // 确保预置的数据同步到es完成
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, IDX_NAME, RECS_NUM ) );
    }

    @Test
    private void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor( FullTextUtils.THREAD_TIMEOUT );
        ThreadTruncate threadTruncate = new ThreadTruncate();
        ThreadSplit threadSplit = new ThreadSplit();
        es.addWorker( threadTruncate );
        es.addWorker( threadSplit );
        es.run();

        // 检验结果
        int expRecsNum = 0;
        if ( threadTruncate.getRetCode() == 0 ) {
            Assert.assertEquals( cl.getCount(), expRecsNum );
        } else if ( threadTruncate.getRetCode() != -190
                && threadTruncate.getRetCode() != -147 ) {
            Assert.fail( "truncate fail, e: " + threadTruncate.getRetCode() );
        } else {
            expRecsNum = RECS_NUM;
            Assert.assertEquals( cl.getCount(), expRecsNum );
        }
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, IDX_NAME, expRecsNum ) );
        Assert.assertEquals( ( int ) cl.getCount(), expRecsNum );

        int actRgNum = FullTextDBUtils.getCLGroups( cl ).size();
        if ( threadSplit.getRetCode() == 0 ) {
            Assert.assertEquals( actRgNum, 2 );
            esIndexNames = FullTextDBUtils.getESIndexNames( cl, IDX_NAME );
        } else if ( threadSplit.getRetCode() != -321
                && threadSplit.getRetCode() != -243
                && threadSplit.getRetCode() != -147
                && threadSplit.getRetCode() != -190 ) {
            Assert.fail( "split fail, e: " + threadSplit.getRetCode() );
        } else {
            Assert.assertEquals( actRgNum, 1 );
        }
    }

    @Override
    protected void caseFini() throws Exception {
        FullTextDBUtils.dropCollection( cs, CL_NAME );
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                cappedCSNames ) );
    }

    private class ThreadTruncate extends ResultStore {
        @ExecuteOrder(step = 1)
        private void truncate() throws InterruptedException {
            Thread.sleep( random.nextInt( 1000 ) );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl.truncate();
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }

    private class ThreadSplit extends ResultStore {
        @ExecuteOrder(step = 1)
        private void split() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( CL_NAME );
                System.out.println( new Date() + " begin "
                        + this.getClass().getName().toString() );
                cl.split( srcRgName, dstRgName, 50 );
                System.out.println( new Date() + " end   "
                        + this.getClass().getName().toString() );
            } catch ( BaseException e ) {
                saveResult( e.getErrorCode(), e );
            }
        }
    }
}