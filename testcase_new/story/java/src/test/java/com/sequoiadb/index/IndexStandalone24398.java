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
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;

/**
 * @Description seqDB-24398:并发创建索引和插入/切分数据
 * @Date 2021/10/8
 * @author wuyan
 * @version 1.10
 */

public class IndexStandalone24398 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_IndexStandalone_24398";
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
        String indexName = "testindex24398";
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateIndex( indexName, indexStandaloneNodeName ) );
        es.addWorker( new SplitCL() );
        es.addWorker( new InsertDatas() );
        es.run();

        // check results
        IndexUtils.checkStandaloneIndexOnNode( sdb, csName, clName, indexName,
                indexStandaloneNodeName, true );
        IndexUtils.checkRecords( cl, insertRecords, "",
                "{'':'" + indexName + "'}" );
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

    private class InsertDatas {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int beginNo = recsNum;
                int endNo = beginNo + 10000;
                ArrayList< BSONObject > curInsertRecords = insertData( cl,
                        endNo - beginNo, beginNo, endNo );
                insertRecords.addAll( curInsertRecords );
            }
        }
    }

    private class CreateIndex {
        private String indexName;
        private String nodeName;

        private CreateIndex( String indexName, String nodeName ) {
            this.indexName = indexName;
            this.nodeName = nodeName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            BSONObject indexAttr = new BasicBSONObject();
            indexAttr.put( "Standalone", true );
            BSONObject option = new BasicBSONObject();
            option.put( "NodeName", nodeName );
            BSONObject indexKeys = new BasicBSONObject();
            indexKeys.put( "testa", 1 );
            indexKeys.put( "no", -1 );

            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.createIndex( indexName, indexKeys, indexAttr, option );
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

    public ArrayList< BSONObject > insertData( DBCollection dbcl, int insertNum,
            int beginNo, int endNo ) {
        int batchNum = 5000;
        int recordNum = endNo - beginNo;
        ArrayList< BSONObject > allRecords = new ArrayList< BSONObject >();
        for ( int i = 0; i < recordNum / batchNum; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                int value = beginNo++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "testa", "testcreateindexkey__" + value );
                obj.put( "no", value );
                obj.put( "testno", value );
                obj.put( "teststr", "teststr" + value );
                batchRecords.add( obj );
            }
            dbcl.insert( batchRecords );
            allRecords.addAll( batchRecords );
            batchRecords.clear();
        }
        return allRecords;
    }
}