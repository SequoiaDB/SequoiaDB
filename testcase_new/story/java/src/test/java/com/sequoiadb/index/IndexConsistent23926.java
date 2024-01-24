package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.Test;
import org.testng.annotations.BeforeClass;
import com.sequoiadb.testcommon.*;

import java.util.ArrayList;
import java.util.Random;

/**
 * @Description seqDB-23926:主表和子表并发创建不同索引
 * @Author YiPan
 * @Date 2021.4.12
 * @UpdateAuthor liuli
 * @UpdateDate 2021.08.27
 * @version 1.10
 */
public class IndexConsistent23926 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String csName = "cs_23926";
    private String mainCLName = "maincl_23926";
    private String subCLName1 = "subcl_23926_1";
    private String subCLName2 = "subcl_23926_2";
    private String idxName1 = "index23926_1";
    private String idxName2 = "index23926_2";
    private DBCollection maincl;
    private boolean runSuccess = false;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        CollectionSpace cs = sdb.createCollectionSpace( csName );
        BasicBSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        optionsM.put( "ShardingKey", new BasicBSONObject( "no", 1 ) );
        optionsM.put( "ShardingType", "range" );
        maincl = cs.createCollection( mainCLName, optionsM );

        cs.createCollection( subCLName1, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );
        cs.createCollection( subCLName2, new BasicBSONObject( "ShardingKey",
                new BasicBSONObject( "no", 1 ) ) );

        BasicBSONObject option1 = new BasicBSONObject();
        option1.put( "LowBound", new BasicBSONObject( "no", 0 ) );
        option1.put( "UpBound", new BasicBSONObject( "no", 10000 ) );
        maincl.attachCollection( csName + "." + subCLName1, option1 );

        BasicBSONObject option2 = new BasicBSONObject();
        option2.put( "LowBound", new BasicBSONObject( "no", 10000 ) );
        option2.put( "UpBound", new BasicBSONObject( "no", 20000 ) );
        maincl.attachCollection( csName + "." + subCLName2, option2 );
    }

    @Test
    public void test() throws Exception {
        int recordNum = 20000;
        ArrayList< BSONObject > allrecord = IndexUtils.insertData( maincl,
                recordNum );

        // 并发创建索引
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker(
                new CreateIndex( csName, mainCLName, idxName1, "testno" ) );
        es.addWorker(
                new CreateIndex( csName, subCLName1, idxName2, "testb" ) );
        es.run();

        // 检查主备索引是否一致
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, idxName1,
                true );
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName1, idxName2,
                true );
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, idxName1,
                true );
        IndexUtils.checkIndexConsistent( sdb, csName, subCLName2, idxName2,
                false );
        Assert.assertTrue( maincl.isIndexExist( idxName1 ) );
        Assert.assertFalse( maincl.isIndexExist( idxName2 ) );

        // 校验数据并查询访问计划
        IndexUtils.checkRecords( maincl, allrecord, "", "" );
        Random random = new Random();
        int value = random.nextInt( recordNum );
        String matcher = "{testno :" + value + "}";
        IndexUtils.checkExplain( maincl, matcher, "ixscan", idxName1 );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class CreateIndex {
        private String csName;
        private String clName;
        private String indexName;
        private String idxField;

        public CreateIndex( String csName, String clName, String indexName,
                String idxField ) {
            this.csName = csName;
            this.clName = clName;
            this.indexName = indexName;
            this.idxField = idxField;
        }

        @ExecuteOrder(step = 1)
        private void createIndex() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.createIndex( indexName, new BasicBSONObject( idxField, 1 ),
                        false, false );
            }
        }
    }
}
