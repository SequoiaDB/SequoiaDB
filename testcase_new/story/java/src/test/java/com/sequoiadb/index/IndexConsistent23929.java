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
 * @description seqDB-23929 :: 并发复制不同索引
 * @author wuyan
 * @date 2021.7.20
 * @version 1.10
 */

public class IndexConsistent23929 extends SdbTestBase {
    private boolean runSuccess = false;
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection maincl;
    private String mainclName = "maincl_23929";
    private String subclName1 = "subcl_23929a";
    private String subclName2 = "subcl_23929b";
    private int recsNum = 30000;
    private ArrayList< String > indexNames = new ArrayList<>();
    private int indexNum = 6;

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

        maincl = createMainCL( cs, mainclName );
        for ( int i = 0; i < indexNum; i++ ) {
            String indexName = "testindex23929_" + i;
            indexNames.add( indexName );
        }
        createIndex( maincl );
        createAndAttachCL( cs, maincl, subclName1, subclName2 );
        IndexUtils.insertDataWithOutReturn( maincl, recsNum );

    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        for ( int i = 0; i < indexNames.size(); i++ ) {
            es.addWorker( new CopyIndex( indexNames.get( i ) ) );
        }
        es.run();

        // check results
        List< String > subclNames = new ArrayList<>();
        subclNames.add( SdbTestBase.csName + "." + subclName1 );
        subclNames.add( SdbTestBase.csName + "." + subclName2 );
        DBCollection subcl1 = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( subclName1 );
        DBCollection subcl2 = sdb.getCollectionSpace( SdbTestBase.csName )
                .getCollection( subclName2 );
        for ( int i = 0; i < indexNames.size(); i++ ) {
            String indexName = indexNames.get( i );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName1, indexName, true );
            IndexUtils.checkIndexConsistent( sdb, SdbTestBase.csName,
                    subclName2, indexName, true );

            checkCopyTask( sdb, SdbTestBase.csName, mainclName, subclNames,
                    indexName );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    subclName1, indexNames.get( i ) );
            IndexUtils.checkIndexTask( sdb, "Create index", SdbTestBase.csName,
                    subclName2, indexNames.get( i ) );
            Assert.assertTrue( subcl1.isIndexExist( indexName ),
                    "check index=" + indexName );
            Assert.assertTrue( subcl2.isIndexExist( indexName ),
                    "check index=" + indexName );
        }
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
        private String indexName;

        private CopyIndex( String indexName ) {
            this.indexName = indexName;
        }

        @ExecuteOrder(step = 1)
        private void test() {
            System.out.println(
                    new Date() + " " + this.getClass().getName().toString() );
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( mainclName );
                cl.copyIndex( "", indexName );
            }
        }

    }

    private DBCollection createMainCL( CollectionSpace cs, String mainclName ) {
        BSONObject optionsM = new BasicBSONObject();
        optionsM.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "no", 1 );
        optionsM.put( "ShardingKey", opt );
        optionsM.put( "ShardingType", "range" );
        DBCollection mainCL = cs.createCollection( mainclName, optionsM );
        return mainCL;

    }

    private void createAndAttachCL( CollectionSpace cs, DBCollection mainCL,
            String subclName1, String subclName2 ) {
        cs.createCollection( subclName1,
                ( BSONObject ) JSON.parse( "{ShardingKey:{no:1}}" ) );
        cs.createCollection( subclName2 );

        mainCL.attachCollection( csName + "." + subclName1, ( BSONObject ) JSON
                .parse( "{LowBound:{no:0},UpBound:{no:10000}}" ) );
        mainCL.attachCollection( csName + "." + subclName2, ( BSONObject ) JSON
                .parse( "{LowBound:{no:10000},UpBound:{no:30000}}" ) );
    }

    private void createIndex( DBCollection dbcl ) {
        dbcl.createIndex( indexNames.get( 0 ), "{testa:1}", false, false );
        dbcl.createIndex( indexNames.get( 1 ), "{no:1, testa:-1}", true,
                false );
        dbcl.createIndex( indexNames.get( 2 ), "{no:-1, testa:-1}", false,
                false );
        dbcl.createIndex( indexNames.get( 3 ), "{testb:-1}", false, false );
        dbcl.createIndex( indexNames.get( 4 ), "{testb:1}", false, false );
        dbcl.createIndex( indexNames.get( 5 ), "{testa:1,testb:1}", false,
                false );
    }

    private void checkCopyTask( Sequoiadb db, String csName, String mainclName,
            List< String > subclNames, String indexName ) {
        List< String > indexNames = new ArrayList<>();
        indexNames.add( indexName );
        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", csName + '.' + mainclName );
        matcher.put( "TaskTypeDesc", "Copy index" );
        matcher.put( "IndexNames", indexNames );
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

        // 校验copy子表信息
        @SuppressWarnings("unchecked")
        List< String > actSubCLNames = ( List< String > ) taskInfo
                .get( "CopyTo" );
        Collections.sort( actSubCLNames );
        Collections.sort( subclNames );
        Assert.assertEquals( actSubCLNames, subclNames );

        Assert.assertEquals( taskNum, 1 );
    }
}