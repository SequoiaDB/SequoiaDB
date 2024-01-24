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
 * @Description seqDB-23938:并发主表复制索引和子表创建相同索引
 * @Author liuli
 * @Date 2021.08.26
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.26
 * @version 1.10
 */
public class IndexConsistent23938 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection maincl;
    private String csName = "csName_23938";
    private String mainclName = "maincl_23938";
    private String subclName1 = "subcl_23938_1";
    private String subclName2 = "subcl_23938_2";
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();
    private String indexName = "testindex23938";
    private boolean createSuc = false;
    private boolean copySuc = false;
    private boolean runSuc = false;

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
        maincl.createIndex( indexName, new BasicBSONObject( "testa", 1 ),
                null );

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

        int recsNum = 40000;
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
        IndexUtils.checkIndexTask( sdb, "Create index", csName, mainclName,
                indexName, 0 );
        IndexUtils.checkIndexTask( sdb, "Create index", csName, subclName1,
                indexName, 0 );
        if ( copySuc ) {
            ArrayList< String > indexNames = new ArrayList<>();
            indexNames.add( indexName );
            ArrayList< String > subclNames = new ArrayList<>();
            subclNames.add( csName + "." + subclName2 );
            if ( !createSuc ) {
                subclNames.add( csName + "." + subclName1 );
            }
            IndexUtils.checkCopyTask( sdb, csName, mainclName, indexNames,
                    subclNames, 0, 9 );
            IndexUtils.checkIndexConsistent( sdb, csName, subclName2, indexName,
                    true );
            IndexUtils.checkIndexTask( sdb, "Create index", csName, subclName2,
                    indexName, 0 );
        } else {
            IndexUtils.checkIndexConsistent( sdb, csName, subclName2, indexName,
                    false );
            Assert.assertFalse( IndexUtils.isExistTask( sdb, "Create index",
                    csName, subclName2 ) );
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
                maincl.copyIndex( "", "" );
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
                DBCollection subcl = db.getCollectionSpace( csName )
                        .getCollection( subclName1 );
                subcl.createIndex( indexName, new BasicBSONObject( "testa", 1 ),
                        null );
                createSuc = true;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_IXM_CREATING
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}