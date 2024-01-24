package com.sequoiadb.fulltext.parallel;

import java.util.concurrent.atomic.AtomicInteger;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-12128:存在全文索引，并发删除同一集合
 * @Author yinzhen
 * @Date 2019-4-30
 */
public class Fulltext12128 extends FullTestBase {
    private String clName = "cl12128";
    private String fullIdxName = "idx12128";
    private String cappedCLName;
    private String esIndexName;
    private AtomicInteger atoint = new AtomicInteger( 0 );
    private int insertNum = 20000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        cl.createIndex( fullIdxName,
                "{'a':'text','b':'text','c':'text', 'd':'text', 'e':'text', 'f':'text'}",
                false, false );

        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIdxName );
        cappedCLName = FullTextDBUtils.getCappedName( cl, fullIdxName );
        FullTextDBUtils.insertData( cl, insertNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        for ( int i = 0; i < 10; i++ ) {
            thExecutor.addWorker( new DropCL() );
        }
        thExecutor.run();
        Assert.assertEquals( atoint.get(), 1 );

        // 集合删除成功
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
        Assert.assertFalse(
                sdb.getCollectionSpace( csName ).isCollectionExist( clName ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
    }

    private class DropCL {
        @ExecuteOrder(step = 1, desc = "并发删除该集合")
        private void dropFullIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                db.getCollectionSpace( csName ).dropCollection( clName );
                atoint.incrementAndGet();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -23 && e.getErrorCode() != -147 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }
}