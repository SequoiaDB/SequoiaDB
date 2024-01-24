package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23964:并发创建索引和数据切分(从一个数据组切到多个数据组)
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23964A extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_Index23964a";
    private String srcGroupName;
    private String destGroupName;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();
    private int recsNum = 50000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException(
                    "---Skip testCase.Current environment less than tow groups! " );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }

        ArrayList< String > groupNames = CommLib.getDataGroupNames( sdb );
        srcGroupName = groupNames.get( 0 );
        destGroupName = groupNames.get( 1 );
        BasicBSONObject options = new BasicBSONObject();
        BasicBSONObject keyValue = new BasicBSONObject();
        keyValue.put( "no", 1 );
        options.put( "ShardingKey", keyValue );
        options.put( "Group", srcGroupName );
        cl = cs.createCollection( clName, options );
        insertRecords = IndexUtils.insertData( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName1 = "testindex23964A_aa";
        String indexName2 = "testindex23964A_bb";
        String indexName3 = "testindex23964A_cc";
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateIndex( indexName1, indexName2, indexName3 ) );
        es.addWorker( new SplitCL() );
        es.run();

        int expResultCode = 0;
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                clName, indexName1, expResultCode, false );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName1, true );
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                clName, indexName2, expResultCode, false );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName2, true );
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                clName, indexName3, expResultCode, false );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName3, true );
        IndexUtils.checkRecords( cl, insertRecords, "",
                "{'':'" + indexName1 + "'}" );
        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( clName );
            }
        } finally {
            sdb.close();
        }
    }

    private class CreateIndex {
        private String indexName1;
        private String indexName2;
        private String indexName3;

        private CreateIndex( String indexName1, String indexName2,
                String indexName3 ) {
            this.indexName1 = indexName1;
            this.indexName2 = indexName2;
            this.indexName3 = indexName3;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {

                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.createIndex( indexName1, "{no:1,testa:1}", true, false );
                cl.createIndex( indexName2, "{testno:1,testa:-1}", false,
                        false );
                cl.createIndex( indexName3, "{testno:-1,testb:-1}", false,
                        false );
            }
        }
    }

    private class SplitCL {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                dbcl.split( srcGroupName, destGroupName, 20 );
            }
        }
    }

}
