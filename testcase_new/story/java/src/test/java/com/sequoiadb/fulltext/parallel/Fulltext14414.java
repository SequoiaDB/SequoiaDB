package com.sequoiadb.fulltext.parallel;

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
 * @FileName seqDB-14414:同一集合并发创建删除相同的全文索引
 * @Author yinzhen
 * @Date 2019-4-28
 */
public class Fulltext14414 extends FullTestBase {
    private String clName = "cl14414";
    private String fullIdxName = "idx14414";
    private String cappedCLName;
    private String esIndexName;
    private boolean checkFlag = false;
    private int insertNum = 20000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        FullTextDBUtils.insertData( cl, insertNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thExecutor.addWorker( new CreateIdx() );
        thExecutor.addWorker( new DropIndex() );
        thExecutor.run();

        // 主备节点上索引信息一致，固定集合、索引信息、ES端数据一致
        if ( cl.isIndexExist( fullIdxName ) ) {
            Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fullIdxName,
                    insertNum ) );
        } else {
            if ( checkFlag ) {
                Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb,
                        esIndexName, cappedCLName ) );
            }
        }
    }

    @Override
    protected void caseFini() throws Exception {
        if ( checkFlag ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexName,
                    cappedCLName ) );
        }
    }

    private class CreateIdx {
        @ExecuteOrder(step = 1, desc = "多线程创建删除同一个全文索引")
        private void createFullIdx() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( fullIdxName,
                        "{'a':'text','b':'text','c':'text', 'd':'text', 'e':'text', 'f':'text'}",
                        false, false );

                cappedCLName = FullTextDBUtils.getCappedName( cl, fullIdxName );
                esIndexName = FullTextDBUtils.getESIndexName( cl, fullIdxName );
                checkFlag = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -199 && e.getErrorCode() != -47 ) {
                    throw e;
                }
            }
        }
    }

    private class DropIndex {
        @ExecuteOrder(step = 1, desc = "删除全文索引")
        private void dropIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.dropIndex( fullIdxName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -47 ) {
                    throw e;
                }
            }
        }
    }
}