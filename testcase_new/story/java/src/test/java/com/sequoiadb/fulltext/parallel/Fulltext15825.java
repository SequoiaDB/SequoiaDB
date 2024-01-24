package com.sequoiadb.fulltext.parallel;

import java.util.concurrent.atomic.AtomicInteger;

import com.sequoiadb.exception.SDBError;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15825:同一集合并发删除相同的全文索引
 * @Author yinzhen
 * @Date 2019-4-30
 */
public class Fulltext15825 extends FullTestBase {
    private String clName = "cl15825";
    private String fullIdxName = "idx15825";
    private String groupName;
    private String cappedCLName;
    private String esIndexName;
    private AtomicInteger atoint = new AtomicInteger( 0 );
    private int insertNum = 20000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        caseProp.setProperty( CLNAME, clName );
        caseProp.setProperty( CLOPT, "{Group:'" + groupName + "'}" );
    }

    @Override
    protected void caseInit() throws Exception {
        FullTextDBUtils.insertData( cl, insertNum );
        cl.createIndex( fullIdxName,
                "{'a':'text','b':'text','c':'text', 'd':'text', 'e':'text', 'f':'text'}",
                false, false );
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, fullIdxName, insertNum ) );
        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIdxName );
        cappedCLName = FullTextDBUtils.getCappedName( cl, fullIdxName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        for ( int i = 0; i < 10; i++ ) {
            thExecutor.addWorker( new DropFullIdx() );
        }
        thExecutor.run();
        Assert.assertEquals( atoint.get(), 1 );

        // 索引信息被删除，同时固定集合删除成功，主备节点数据一致，ES端最终无索引数据
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
        Assert.assertTrue( FullTextUtils.isCLDataConsistency( cl ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
    }

    private class DropFullIdx {
        @ExecuteOrder(step = 1, desc = "多线程创建删除同一个全文索引")
        private void dropFullIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( fullIdxName );
                atoint.incrementAndGet();
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_IXM_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_DROPPING
                                .getErrorCode() ) {
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
