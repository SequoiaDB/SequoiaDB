package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;

/**
 * @description seqDB-24381:指定相同数据节点并发创建不同本地索引
 * @author wuyan
 * @date 2021/10/8
 * @version 1.10
 */

public class IndexStandalone24381 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_IndexStandalone_24381";
    private int recsNum = 10000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        BasicBSONObject options = new BasicBSONObject();
        options.put( "ReplSize", 0 );
        cl = cs.createCollection( clName, options );
        IndexUtils.insertDataWithOutReturn( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        int indexNums = 10;
        List< String > indexNames = new ArrayList< String >();
        String nodeName = IndexUtils.getCLOneNode( sdb, SdbTestBase.csName,
                clName );
        List< BSONObject > indexKeyList = setIndexKeys();
        for ( int i = 0; i < indexNums; i++ ) {
            String indexName = "testindex24381_" + i;
            BSONObject indexKey = indexKeyList.get( i );
            es.addWorker( new CreateIndex( indexName, nodeName, indexKey ) );
            indexNames.add( indexName );
        }
        es.run();

        // check results
        IndexUtils.checkStandaloneIndexOnNode( sdb, csName, clName, indexNames,
                nodeName, true );
        IndexUtils.checkStandaloneIndexTasks( sdb, "Create index", csName,
                clName, nodeName, indexNames );
        checkUseIndex( csName, clName, nodeName, indexNames );

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
        private String indexName;
        private String nodeName;
        private BSONObject indexKeys;

        private CreateIndex( String indexName, String nodeName,
                BSONObject indexKeys ) {
            this.indexName = indexName;
            this.nodeName = nodeName;
            this.indexKeys = indexKeys;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            BSONObject indexAttr = new BasicBSONObject();
            indexAttr.put( "Standalone", true );
            BSONObject option = new BasicBSONObject();
            option.put( "NodeName", nodeName );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.createIndex( indexName, indexKeys, indexAttr, option );
            }
        }
    }

    private List< BSONObject > setIndexKeys() {
        List< BSONObject > indexKeyList = new ArrayList<>();
        BSONObject indexKeys1 = new BasicBSONObject();
        indexKeys1.put( "testno", 1 );
        indexKeyList.add( indexKeys1 );
        BSONObject indexKeys2 = new BasicBSONObject();
        indexKeys2.put( "testno", -1 );
        indexKeyList.add( indexKeys2 );
        BSONObject indexKeys3 = new BasicBSONObject();
        indexKeys3.put( "testno", 1 );
        indexKeys3.put( "no", 1 );
        indexKeyList.add( indexKeys3 );
        BSONObject indexKeys4 = new BasicBSONObject();
        indexKeys4.put( "no", -1 );
        indexKeyList.add( indexKeys4 );
        BSONObject indexKeys5 = new BasicBSONObject();
        indexKeys5.put( "testb", 1 );
        indexKeyList.add( indexKeys5 );
        BSONObject indexKeys6 = new BasicBSONObject();
        indexKeys6.put( "testa", 1 );
        indexKeyList.add( indexKeys6 );
        BSONObject indexKeys7 = new BasicBSONObject();
        indexKeys7.put( "testb", -1 );
        indexKeyList.add( indexKeys7 );
        BSONObject indexKeys8 = new BasicBSONObject();
        indexKeys8.put( "testa", -1 );
        indexKeys8.put( "no", -1 );
        indexKeyList.add( indexKeys8 );
        BSONObject indexKeys9 = new BasicBSONObject();
        indexKeys9.put( "testa", -1 );
        indexKeys9.put( "testb", -1 );
        indexKeyList.add( indexKeys9 );
        BSONObject indexKeys10 = new BasicBSONObject();
        indexKeys10.put( "testb", -1 );
        indexKeys10.put( "no", -1 );
        indexKeyList.add( indexKeys10 );
        return indexKeyList;
    }

    private void checkUseIndex( String csName, String clName, String nodeName,
            List< String > indexNames ) {
        try ( Sequoiadb db = new Sequoiadb( nodeName, "", "" )) {
            DBCollection dbcl = db.getCollectionSpace( csName )
                    .getCollection( clName );
            IndexUtils.checkExplain( dbcl, "{testno :1,no:1}", "ixscan",
                    indexNames.get( 2 ) );
        }
    }
}