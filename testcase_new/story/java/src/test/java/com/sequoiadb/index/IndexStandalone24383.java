package com.sequoiadb.index;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
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

import static org.testng.Assert.assertTrue;

/**
 * @description seqDB-24383:并发删除本地索引和一致性索引
 * @author wuyan
 * @date 2021/10/8
 * @version 1.10
 */

public class IndexStandalone24383 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_IndexStandalone_24383";
    private int recsNum = 10000;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( clName ) ) {
            cs.dropCollection( clName );
        }
        BSONObject options = new BasicBSONObject();
        options.put( "ReplSize", 0 );
        cl = cs.createCollection( clName, options );
        IndexUtils.insertDataWithOutReturn( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexStandaloneNodeName = IndexUtils.getCLOneNode( sdb,
                SdbTestBase.csName, clName );
        List< String > indexNames = createIndexs( cl, indexStandaloneNodeName );

        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < indexNames.size(); i++ ) {
            String indexName = indexNames.get( i );
            es.addWorker( new DropIndex( indexName ) );
        }
        es.run();

        // check results
        checkIndexOnNode( sdb, SdbTestBase.csName, clName );

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

    private class DropIndex {
        private String indexName;

        private DropIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.dropIndex( indexName );
            }
        }
    }

    private List< String > createIndexs( DBCollection dbcl,
            String indexNodeName ) {
        List< String > indexNames = new ArrayList<>();
        BSONObject indexAttr = new BasicBSONObject();
        indexAttr.put( "Standalone", true );
        BSONObject option = new BasicBSONObject();
        option.put( "NodeName", indexNodeName );
        BSONObject indexKeys1 = new BasicBSONObject();
        indexKeys1.put( "testno", 1 );
        String indexName1 = "indexStandalone1";
        dbcl.createIndex( indexName1, indexKeys1, indexAttr, option );
        indexNames.add( indexName1 );

        BSONObject indexKeys2 = new BasicBSONObject();
        indexKeys2.put( "no", 1 );
        String indexName2 = "indexStandalone2";
        dbcl.createIndex( indexName2, indexKeys2, indexAttr, option );
        indexNames.add( indexName2 );

        BSONObject indexKeys3 = new BasicBSONObject();
        indexKeys3.put( "testa", 1 );
        String indexName3 = "indexStandalone3";
        dbcl.createIndex( indexName3, indexKeys3, indexAttr, option );
        indexNames.add( indexName3 );

        // 创建一致性索引
        String indexName4 = "indexConsisitent1";
        dbcl.createIndex( indexName4, "{no:1,testa:1}", false, false );
        indexNames.add( indexName4 );
        String indexName5 = "indexConsisitent2";
        dbcl.createIndex( indexName5, "{no:-1,testno:1}", false, false );
        indexNames.add( indexName5 );
        return indexNames;
    }

    private void checkIndexOnNode( Sequoiadb db, String csName, String clName )
            throws Exception {
        List< String > groupNames = IndexUtils.getCLGroupNames( db, csName,
                clName );
        // 校验lsn是否一致
        Assert.assertTrue(
                IndexUtils.isLSNConsistency( db, groupNames.get( 0 ) ) );
        List< BasicBSONObject > nodes = CommLib.getCLNodes( db, csName,
                clName );
        for ( BasicBSONObject node : nodes ) {
            String nodeUrl = node.getString( "hostName" ) + ":"
                    + node.getString( "svcName" );
            try ( Sequoiadb data = new Sequoiadb( nodeUrl, "", "" )) {
                DBCollection dbcl = CommLib.getCL( data, csName, clName );
                List< BSONObject > indexInfos = new ArrayList<>();
                DBCursor listIndexCur = dbcl.getIndexes();
                while ( listIndexCur.hasNext() ) {
                    BSONObject indexInfo = listIndexCur.getNext();
                    indexInfos.add( indexInfo );
                }
                // 验证节点上只存在id索引信息，其它索引都不存在
                Assert.assertEquals( indexInfos.size(), 1,
                        nodeUrl + " act indexs =" + indexInfos );
                BSONObject indexInfo = ( BSONObject ) indexInfos.get( 0 )
                        .get( "IndexDef" );
                String indexName = ( String ) indexInfo.get( "name" );
                Assert.assertEquals( indexName, "$id",
                        nodeUrl + " act index is " + indexInfos );
                listIndexCur.close();
            }
        }
    }
}