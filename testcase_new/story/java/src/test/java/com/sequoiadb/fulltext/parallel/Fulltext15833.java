package com.sequoiadb.fulltext.parallel;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15833:删除全文索引与删除集合空间并发
 * @Author yinzhen
 * @Date 2019-4-30
 */
public class Fulltext15833 extends FullTestBase {
    private String clName = "cl15833";
    private String fullIdxName = "idx15833";
    private String csName = "cs15833";
    private String cappedCLName;
    private String esIndexName;
    private int insertNum = 20000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CSNAME, csName );
        caseProp.setProperty( CLNAME, clName );
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
        thExecutor.addWorker( new DropFullIdx() );
        thExecutor.addWorker( new DropCS() );

        thExecutor.run();

        // 原始集合空间及固定集合均被删除成功，ES上全文索引删除成功，主备节点数据一致，无数据文件残留
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
        Assert.assertFalse( sdb.isCollectionSpaceExist( csName ) );
    }

    @Override
    protected void caseFini() throws Exception {
        Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                cappedCLName ) );
    }

    private class DropFullIdx {
        @ExecuteOrder(step = 1, desc = "删除全文索引")
        private void dropFullIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( fullIdxName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -248 && e.getErrorCode() != -23
                        && e.getErrorCode() != -34 ) {
                    throw e;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class DropCS {
        @ExecuteOrder(step = 1, desc = "删除集合空间")
        private void dropCS() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                db.dropCollectionSpace( csName );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }
}