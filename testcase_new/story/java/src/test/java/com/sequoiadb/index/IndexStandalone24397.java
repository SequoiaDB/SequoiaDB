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
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * @description seqDB-24397:并发删除本地索引和数据切分
 * @author wuyan
 * @date 2021/10/8
 * @version 1.10
 */

public class IndexStandalone24397 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_IndexStandalone_24397";
    private int recsNum = 10000;
    private String srcGroupName;
    private String destGroupName;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();

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
        options.put( "ReplSize", 0 );
        options.put( "Group", srcGroupName );
        cl = cs.createCollection( clName, options );
        insertRecords = IndexUtils.insertData( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexStandaloneNodeName = IndexUtils.getCLOneNode( sdb,
                SdbTestBase.csName, clName );
        List< String > indexNames = createIndexs( cl, indexStandaloneNodeName );
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new DropIndex( indexNames ) );
        es.addWorker( new SplitCL() );
        es.run();

        // check results
        IndexUtils.checkStandaloneIndexOnNode( sdb, SdbTestBase.csName, clName,
                indexNames, indexStandaloneNodeName, false );
        IndexUtils.checkRecords( cl, insertRecords, "", "" );
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
        private List< String > indexNames;

        private DropIndex( List< String > indexNames ) {
            this.indexNames = indexNames;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                for ( int i = 0; i < indexNames.size(); i++ ) {
                    String indexName = indexNames.get( i );
                    cl.dropIndex( indexName );
                }
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
                dbcl.split( srcGroupName, destGroupName, 50 );
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
        indexKeys2.put( "no", -1 );
        String indexName2 = "indexStandalone2";
        dbcl.createIndex( indexName2, indexKeys2, indexAttr, option );
        indexNames.add( indexName2 );

        BSONObject indexKeys3 = new BasicBSONObject();
        indexKeys3.put( "testa", 1 );
        String indexName3 = "indexStandalone3";
        dbcl.createIndex( indexName3, indexKeys3, indexAttr, option );
        indexNames.add( indexName3 );
        return indexNames;
    }
}