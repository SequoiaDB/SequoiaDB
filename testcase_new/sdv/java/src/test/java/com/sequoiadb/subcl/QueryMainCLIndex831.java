package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: QueryMainCLIndex831.java test content:
 * 查询主表下数据,并使用query.explain访问计划查看查询状态 testlink case: seqDB-831
 * 
 * @author zengxianquan
 * @date 2016.12.20
 * @version 1.00
 */
public class QueryMainCLIndex831 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace maincs = null;
    private String maincsName = "maincs831";
    private String mainclName = "maincl831";
    private int groupSize = 0;
    private String index;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( SubCLUtils.getDataGroups( sdb ).size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups" );
        }
        createMainCS();
        createMaincl();
        createSubcls();
        attachSubcls();
        createIndexAndInsertData();
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( maincsName );
        } catch ( BaseException e ) {
            Assert.fail( "drop cs failed: " + e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    public void createMainCS() {
        BSONObject options = new BasicBSONObject();
        options.put( "PageSize", 4096 );
        try {
            maincs = sdb.createCollectionSpace( maincsName );
        } catch ( BaseException e ) {
            if ( -33 != e.getErrorCode() ) {
                Assert.assertTrue( false,
                        "create Maincs failed " + e.getMessage() );
            }
        }
    }

    public void createMaincl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingType", "range" );
        BSONObject opt = new BasicBSONObject();
        opt.put( "a", 1 );
        options.put( "ShardingKey", opt );
        try {
            maincs.createCollection( mainclName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "create Maincl failed " + e.getMessage() );
        }
    }

    public void createSubcls() {
        ArrayList< String > groupNames = SubCLUtils.getDataGroups( sdb );
        groupSize = groupNames.size();
        try {
            for ( int j = 0; j < groupSize; j++ ) {
                BSONObject options = new BasicBSONObject();
                options.put( "Group", groupNames.get( j ) );
                maincs.createCollection( "subcl831_" + j );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "createSubcl failed, groupSize: "
                    + groupSize + e.getMessage() );
        }
    }

    public void attachSubcls() {
        DBCollection maincl = null;
        try {
            maincl = maincs.getCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to get collection " + e.getMessage() );
        }
        try {
            for ( int j = 0; j < groupSize; j++ ) {
                String jsonStr = "{'LowBound':{'a':" + j * 300
                        + "},UpBound:{'a':" + ( j + 1 ) * 300 + "}}";// every
                                                                     // subcl
                                                                     // has 300
                                                                     // records
                BSONObject options = ( BSONObject ) JSON.parse( jsonStr );
                maincl.attachCollection( maincsName + ".subcl831_" + j,
                        options );
            }
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "attachSubcl  failed " + e.getMessage() );
        }
    }

    public void createIndexAndInsertData() {
        List< BSONObject > insertor = new ArrayList<>();
        StringBuffer strBuffer = new StringBuffer();
        for ( int j = 0; j < 512; j++ ) {
            strBuffer.append( "a" );
        }
        for ( int i = 0; i < groupSize * 300; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "a", i );
            bson.put( "test", strBuffer.toString() );
            insertor.add( bson );
        }
        DBCollection maincl;
        try {
            maincl = sdb.getCollectionSpace( maincsName )
                    .getCollection( mainclName );
            BSONObject indexBson = new BasicBSONObject();
            indexBson.put( "a", 1 );
            index = "Idx";
            maincl.createIndex( index, indexBson, false, false );
            maincl.insert( insertor );
        } catch ( BaseException e ) {
            System.out.println( "create index error: " + e.getErrorCode() + " "
                    + e.getMessage() );
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @Test
    public void testQueryExplain() {
        DBCollection maincl = null;
        DBCursor expRes = null;
        BSONObject expDet = null;
        BasicBSONList expDets = null;
        BSONObject queryDetail = null;
        try {
            maincl = maincs.getCollection( mainclName );
            BSONObject matcher = ( BSONObject ) JSON
                    .parse( "{a:{$gt:200,$lt:800}}" );
            BSONObject options = ( BSONObject ) JSON.parse( "{Run:true}" );

            BSONObject query = ( BSONObject ) JSON
                    .parse( "{$and:[{a:{$lt:800}},{a:{$gt:200}}]}" );
            expRes = maincl.explain( matcher, null, null, null, 0, -1, 0,
                    options );
            while ( expRes.hasNext() ) {
                expDets = ( BasicBSONList ) expRes.getNext()
                        .get( "SubCollections" );
                expDet = ( BSONObject ) expDets.get( 0 );
                String scanType = ( String ) expDet.get( "ScanType" );
                String indexName = ( String ) expDet.get( "IndexName" );
                queryDetail = ( BSONObject ) JSON
                        .parse( expDet.get( "Query" ).toString() );
                if ( !scanType.equals( "ixscan" ) || !indexName.equals( index )
                        || !queryDetail.equals( query ) ) {
                    Assert.fail( "scanType: " + scanType + " " + "name: "
                            + indexName + " " + "Detail: "
                            + JSON.parse( queryDetail.toString() ) );
                }
            }
        } catch ( BaseException e ) {
            System.out.println( "testQueryExplain error: " + e.getMessage() );
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( expRes != null ) {
                expRes.close();
            }
        }
    }
}
