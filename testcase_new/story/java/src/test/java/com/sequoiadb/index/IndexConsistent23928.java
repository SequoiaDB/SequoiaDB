package com.sequoiadb.index;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;

import com.sequoiadb.testcommon.CommLib;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @description seqDB-23928 :: 并发复制相同索引到不同子表
 * @author wuyan
 * @date 2021.7.20
 * @version 1.10
 */

public class IndexConsistent23928 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection maincl;
    private String mainclName = "maincl_23928";
    private String subclName1 = "subcl_23928a";
    private String subclName2 = "subcl_23928b";
    private int recsNum = 40000;
    private ArrayList< BSONObject > insertRecords = new ArrayList<>();
    private String indexName1 = "testindex23928a";
    private String indexName2 = "testindex23928b";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "---Skip testCase on standalone" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        if ( cs.isCollectionExist( mainclName ) ) {
            cs.dropCollection( mainclName );
        }

        maincl = createMainCLAndIndex( cs, mainclName, indexName1, indexName2 );
        createAndAttachCL( cs, maincl, subclName1, subclName2 );
        insertRecords = IndexUtils.insertData( maincl, recsNum );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        es.addWorker( new CopyIndex( subclName1 ) );
        es.addWorker( new CopyIndex( subclName2 ) );

        es.run();

        // check results
        IndexUtils.checkRecords( maincl, insertRecords, "", "" );

        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName1,
                indexName1, true );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName2,
                indexName1, true );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName1,
                indexName2, true );
        IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName, subclName2,
                indexName2, true );

        List< String > indexNames = new ArrayList<>();
        indexNames.add( indexName1 );
        indexNames.add( indexName2 );
        List< String > subclNames1 = new ArrayList<>();
        subclNames1.add( SdbTestBase.csName + "." + subclName1 );
        List< String > subclNames2 = new ArrayList<>();
        subclNames2.add( SdbTestBase.csName + "." + subclName2 );
        checkCopyTask( sdb, SdbTestBase.csName, mainclName, subclNames1,
                indexNames );
        checkCopyTask( sdb, SdbTestBase.csName, mainclName, subclNames2,
                indexNames );

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                cs.dropCollection( mainclName );
            }
        } finally {
            sdb.close();
        }
    }

    private class CopyIndex {
        private String subclName;

        private CopyIndex( String subclName ) {
            this.subclName = subclName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( mainclName );
                cl.copyIndex( SdbTestBase.csName + "." + subclName, "" );
            }
        }
    }

    private DBCollection createMainCLAndIndex( CollectionSpace cs,
            String mainclName, String indexName1, String indexName2 ) {
        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );
        mainCL.createIndex( indexName1, "{testa:1}", false, false );
        mainCL.createIndex( indexName2, "{no:-1, testa:-1}", false, false );
        return mainCL;
    }

    private void createAndAttachCL( CollectionSpace cs, DBCollection mainCL,
            String subclName1, String subclName2 ) {
        cs.createCollection( subclName1,
                ( BSONObject ) JSON.parse( "{ShardingKey:{no:1}}" ) );
        cs.createCollection( subclName2 );

        mainCL.attachCollection( csName + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:20000}}" ) );
        mainCL.attachCollection( csName + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:20000},UpBound:{no:40000}}" ) );
    }

    private void checkCopyTask( Sequoiadb db, String csName, String mainclName,
            List< String > subclNames, List< String > indexNames ) {
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + mainclName );
        matcher.put( "TaskTypeDesc", "Copy index" );
        matcher.put( "CopyTo", subclNames );
        DBCursor cursor = db.listTasks( matcher, null, null, null );
        BSONObject taskInfo = new BasicBSONObject();
        int taskNum = 0;
        while ( cursor.hasNext() ) {
            taskInfo = cursor.getNext();
            taskNum++;
        }
        cursor.close();

        // 校验索引名
        @SuppressWarnings("unchecked")
        List< String > actIndexNames = ( List< String > ) taskInfo
                .get( "IndexNames" );
        Collections.sort( actIndexNames );
        Collections.sort( indexNames );
        Assert.assertEquals( actIndexNames, indexNames, "task=" + taskInfo );

        // 校验结果状态码和结果码
        int resultCode = 0;
        int actResultCode = ( int ) taskInfo.get( "ResultCode" );
        Assert.assertEquals( actResultCode, resultCode );

        int status = 9;
        int actStatus = ( int ) taskInfo.get( "Status" );
        Assert.assertEquals( actStatus, status );

        Assert.assertEquals( taskNum, 1 );
    }
}