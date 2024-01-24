package com.sequoiadb.fulltext.parallel;

import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.testcommon.FullTestBase;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-15826:删除全文索引与创建/删除普通索引并发
 * @Author yinzhen
 * @Date 2019-4-30
 */
public class Fulltext15826 extends FullTestBase {
    private String clName = "cl15826";
    private String fullIdxName = "idx15826";
    private String cappedCLName;
    private String esIndexName;
    private int insertNum = 20000;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( CLNAME, clName );
    }

    @Override
    protected void caseInit() throws Exception {
        FullTextDBUtils.insertData( cl, insertNum );

        // 创建索引
        cl.createIndex( fullIdxName, "{'a':'text','b':'text','c':'text'}",
                false, false );
        cl.createIndex( "idx1", "{'a':1, 'b':1}", false, false );
        cl.createIndex( "idx2", "{'e':1, 'f':1}", false, false );
        FullTextUtils.isIndexCreated( cl, fullIdxName, insertNum );

        esIndexName = FullTextDBUtils.getESIndexName( cl, fullIdxName );
        cappedCLName = FullTextDBUtils.getCappedName( cl, fullIdxName );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor thExecutor = new ThreadExecutor(
                FullTextUtils.THREAD_TIMEOUT );
        thExecutor.addWorker( new DropFullIdx() );
        thExecutor.addWorker( new CreateIdx( "idx3", "{'d':1, 'f':1}" ) );
        thExecutor.addWorker( new CreateIdx( "idx4", "{'b':1, 'c':1}" ) );
        thExecutor.addWorker( new DropIdx( "idx1" ) );
        thExecutor.addWorker( new DropIdx( "idx2" ) );

        thExecutor.run();

        // 主备节点上索引信息及固定集合信息一致，ES同步的索引数据正确
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
        @ExecuteOrder(step = 1, desc = "删除全文索引")
        private void dropFullIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.dropIndex( fullIdxName );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private class CreateIdx {
        private String idxName;
        private String option;

        private CreateIdx( String idxName, String option ) {
            this.idxName = idxName;
            this.option = option;
        }

        @ExecuteOrder(step = 1, desc = "创建普通索引，删除普通索引")
        private void createIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                Assert.assertFalse( cl.isIndexExist( idxName ) );
                cl.createIndex( idxName, option, false, false );
                Assert.assertTrue( cl.isIndexExist( idxName ) );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }

        }
    }

    private class DropIdx {
        private String idxName;

        private DropIdx( String idxName ) {
            this.idxName = idxName;
        }

        @ExecuteOrder(step = 1, desc = "创建普通索引，删除普通索引")
        private void dropIdx() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                Assert.assertTrue( cl.isIndexExist( idxName ) );
                cl.dropIndex( idxName );
                Assert.assertFalse( cl.isIndexExist( idxName ) );
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }
}
