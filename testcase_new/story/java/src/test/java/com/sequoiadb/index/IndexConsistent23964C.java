package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Date;

import com.sequoiadb.base.*;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23964:并发创建索引和数据切分(从多个组切到一个组)
 * @author wuyan
 * @date 2021.4.8
 * @version 1.10
 */

public class IndexConsistent23964C extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection cl;
    private String clName = "cl_Index23964c";
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
        cl.split( srcGroupName, destGroupName, 10 );
        insertRecords = IndexUtils.insertData( cl, recsNum );
    }

    @Test
    public void test() throws Exception {
        String indexName = "testindex23964C";
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CreateIndex( indexName ) );
        es.addWorker( new SplitCL() );
        es.run();

        int expResultCode = 0;
        IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                clName, indexName, expResultCode, false );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, clName,
                indexName, true );
        IndexUtils.checkRecords( cl, insertRecords, "",
                "{'':'" + indexName + "'}" );
        IndexUtils.checkIsExistIndexOnGroup( sdb, destGroupName, indexName,
                SdbTestBase.csName, clName, true );
        IndexUtils.checkIsExistIndexOnGroup( sdb, srcGroupName, indexName,
                SdbTestBase.csName, clName, false );
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

        private CreateIndex( String indexName ) {
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
                cl.createIndex( indexName, "{no:1,testa:1}", true, false );
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
                dbcl.split( srcGroupName, destGroupName, 100 );
            }
        }
    }
}
