package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23965:并发删除索引和数据切分(从多个组100%切分到一个组)
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23965C extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_Index23965c";
    private String indexName1 = "testindex23965c_1";
    private String indexName2 = "testindex23965c_2";
    private String indexName3 = "testindex23965c_3";
    private String srcGroupName;
    private String destGroupName;
    private int recsNum = 20000;
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
        options.put( "Group", srcGroupName );
        cl = cs.createCollection( clName, options );
        cl.createIndex( indexName1, "{no:1,testa:1}", true, false );
        cl.createIndex( indexName2, "{no:-1,testa:1,testb:-1}", true, false );
        cl.createIndex( indexName3, "{testa:-1,testb:-1}", false, false );
        insertRecords = IndexUtils.insertData( cl, recsNum );
        cl.split( srcGroupName, destGroupName, 20 );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new DropIndex() );
        es.addWorker( new SplitCL() );
        es.run();

        int expResultCode = 0;
        IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                clName, indexName1, expResultCode, false );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName1, false );
        IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                clName, indexName2, expResultCode, false );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName2, false );
        IndexUtils.checkIndexTask( sdb, "Drop index", SdbTestBase.csName,
                clName, indexName3, expResultCode, false );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName3, false );
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

    private class DropIndex {
        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.dropIndex( indexName1 );
                cl.dropIndex( indexName2 );
                cl.dropIndex( indexName3 );
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
                dbcl.split( destGroupName, srcGroupName, 100 );
            }
        }
    }
}
