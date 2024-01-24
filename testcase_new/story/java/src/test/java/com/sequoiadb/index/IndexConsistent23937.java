package com.sequoiadb.index;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-23937:主表并发创建和复制相同索引
 * @Author liuli
 * @Date 2021.08.26
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.26
 * @version 1.10
 */
public class IndexConsistent23937 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private boolean runSuc = false;
    private CollectionSpace cs;
    private DBCollection maincl;
    private String csName = "csName_23937";
    private String mainclName = "maincl_23937";
    private String subclName1 = "subcl_23937_1";
    private String subclName2 = "subcl_23937_2";
    private int recsNum = 40000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();
    private String indexName = "testindex23937";
    private boolean copySuc = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        cs = sdb.createCollectionSpace( csName );

        BasicBSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        optionsM.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        optionsM.put( "ShardingType", "range" );
        maincl = cs.createCollection( mainclName, optionsM );

        cs.createCollection( subclName1, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );
        cs.createCollection( subclName2, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 20000 ) );
        maincl.attachCollection( csName + "." + subclName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 20000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 40000 ) );
        maincl.attachCollection( csName + "." + subclName2, option2 );

        insertRecords = IndexUtils.insertData( maincl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CopyIndex() );
        es.addWorker( new CreateIndex() );
        es.run();

        // check results
        IndexUtils.checkRecords( maincl, insertRecords, "", "" );
        Assert.assertTrue( maincl.isIndexExist( indexName ) );
        IndexUtils.checkIndexConsistent( sdb, csName, subclName1, indexName,
                true );
        IndexUtils.checkIndexConsistent( sdb, csName, subclName2, indexName,
                true );
        IndexUtils.checkIndexTask( sdb, "Create index", csName, mainclName,
                indexName, 0 );
        IndexUtils.checkIndexTask( sdb, "Create index", csName, subclName1,
                indexName, 0 );
        IndexUtils.checkIndexTask( sdb, "Create index", csName, subclName2,
                indexName, 0 );
        // 当线程串行执行时，先创建索引在copy索引，copy索引不报错，并且copy索引任务子表名和索引名都显示为空
        if ( copySuc ) {
            IndexUtils.checkCopyTask( sdb, csName, mainclName,
                    new ArrayList< String >(), new ArrayList< String >(), 0,
                    9 );
        } else {
            Assert.assertFalse( IndexUtils.isExistTask( sdb, "Copy index",
                    csName, mainclName ) );
        }
        runSuc = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuc ) {
                cs.dropCollection( mainclName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CopyIndex {

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainclName );
                maincl.copyIndex( "", indexName );
                copySuc = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_IXM_NOTEXIST
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_IXM_CREATING
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    private class CreateIndex {

        @ExecuteOrder(step = 1)
        private void test() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection maincl = db.getCollectionSpace( csName )
                        .getCollection( mainclName );
                maincl.createIndex( indexName,
                        new BasicBSONObject( "testa", 1 ), null );
            }
        }
    }
}