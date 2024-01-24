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
 * FileName: CreateSameIndex6216.java test content:
 * 子表创建索引后在主表创建相同索引_SD.subCL.01.024 testlink case: seqDB-6216
 * 
 * @author zengxianquan
 * @date 2016.12.28
 * @version 1.00
 */
public class CreateSameIndex6216 extends SdbTestBase {
    private Sequoiadb db = null;
    private CollectionSpace maincs = null;
    private String maincsName = "maincs6216";
    private String mainclName = "maincl6216";
    private DBCollection subcl1 = null;
    private DBCollection subcl2 = null;
    private DBCollection maincl = null;
    private String indexName = "index_1";

    @BeforeClass
    public void setUp() {
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( SubCLUtils.getDataGroups( db ).size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups" );
        }

        createMainCS();
        createMaincl();

        createAndAttachSubcls();
    }

    @AfterClass
    public void tearDown() {
        try {
            db.dropCollectionSpace( maincsName );
        } catch ( BaseException e ) {
            Assert.fail( "drop cs failed: " + e.getMessage() );
        } finally {
            db.close();
        }
    }

    @Test
    public void testCreateIndex() {

        boolean isUnique = false;

        createIndexInSub( isUnique );

        checkIndexBeforeMainclCreateIndex();

        createIndexInMaincl( isUnique );

        checkIndexAfterMainclCreateIndex();
        insertData();
        checkQueryExplain();

        dropIndex();
        isUnique = true;

        createIndexInSub( isUnique );

        checkIndexBeforeMainclCreateIndex();

        createIndexInMaincl( isUnique );

        checkIndexAfterMainclCreateIndex();
        insertData();
        checkQueryExplain();
    }

    public void dropIndex() {
        DBCursor cursor = null;
        try {
            maincl.dropIndex( indexName );
            cursor = maincl.getIndexes();
            if ( cursor.hasNext() ) {
                Assert.fail( "dropIndex is error" );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
    }

    public void createMainCS() {
        BSONObject options = new BasicBSONObject();
        options.put( "PageSize", 4096 );
        try {
            maincs = db.createCollectionSpace( maincsName );
        } catch ( BaseException e ) {
            if ( -33 != e.getErrorCode() ) {
                Assert.assertTrue( false,
                        "createMaincl failed " + e.getMessage() );
            }
        }
    }

    public void createMaincl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingType", "range" );
        BSONObject opt = new BasicBSONObject();
        opt.put( "time", 1 );
        options.put( "ShardingKey", opt );
        try {
            maincl = maincs.createCollection( mainclName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "createMaincl failed " + e.getMessage() );
        }
    }

    public void createAndAttachSubcls() {
        BSONObject options = new BasicBSONObject();
        options.put( "AutoIndexId", false );
        String jsonStr1 = "{'LowBound':{'time':0},UpBound:{'time':300}}";
        String jsonStr2 = "{'LowBound':{'time':300},UpBound:{'time':600}}";
        BSONObject options1 = ( BSONObject ) JSON.parse( jsonStr1 );
        BSONObject options2 = ( BSONObject ) JSON.parse( jsonStr2 );
        try {
            maincl = maincs.getCollection( mainclName );
            subcl1 = maincs.createCollection( "subcl626_1", options );
            subcl2 = maincs.createCollection( "subcl626_2", options );
            maincl.attachCollection( maincsName + "." + "subcl626_1",
                    options1 );
            maincl.attachCollection( maincsName + "." + "subcl626_2",
                    options2 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( "createAndAttachCl failed " + e.getMessage() );
        }
    }

    private void checkIndexBeforeMainclCreateIndex() {
        String expectIndexName = "index_1";
        BSONObject expectIndexKey = ( BSONObject ) JSON.parse( "{'time':1}" );

        checkIndex( subcl1, expectIndexName, expectIndexKey );
        checkIndex( maincl, null, null );
        checkIndex( subcl2, null, null );
    }

    private void checkIndexAfterMainclCreateIndex() {
        String expectIndexName = "index_1";
        BSONObject expectIndexKey = ( BSONObject ) JSON.parse( "{'time':1}" );

        checkIndex( subcl1, expectIndexName, expectIndexKey );
        checkIndex( maincl, expectIndexName, expectIndexKey );
        checkIndex( subcl2, expectIndexName, expectIndexKey );
    }

    public void createIndexInSub( boolean isUnique ) {
        BSONObject keyBson = new BasicBSONObject();
        keyBson.put( "time", 1 );
        try {
            subcl1.createIndex( indexName, keyBson, isUnique, false );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create index" );
        }
    }

    public void createIndexInMaincl( boolean isUnique ) {
        BSONObject keyBson = new BasicBSONObject();
        keyBson.put( "time", 1 );
        try {
            maincl.createIndex( indexName, keyBson, isUnique, false );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create index" + e.getMessage() );
        }
    }

    public void checkIndex( DBCollection cl, String expectIndexName,
            BSONObject expectIndexKey ) {
        DBCursor cursor = null;
        BSONObject indexDetail = null;
        BSONObject indexDef = null;
        BSONObject key = null;
        String name = null;
        try {

            cursor = cl.getIndexes();
            if ( cursor.hasNext() == false
                    && ( expectIndexName != null || expectIndexKey != null ) ) {
                Assert.fail( "index message is error" );
            }

            while ( cursor.hasNext() ) {
                indexDetail = cursor.getNext();
                indexDef = ( BSONObject ) indexDetail.get( "IndexDef" );
                name = ( String ) indexDef.get( "name" );
                key = ( BSONObject ) indexDef.get( "key" );
                if ( ( !name.equals( expectIndexName ) )
                        || ( !key.equals( expectIndexKey ) ) ) {
                    Assert.fail( "index is error" );
                }
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
    }

    public void insertData() {
        List< BSONObject > insertor = new ArrayList<>();
        for ( int i = 0; i < 600; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "time", i );
            bson.put( "test", "test" );
            insertor.add( bson );
        }
        try {
            maincl.insert( insertor, 1 );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public void checkQueryExplain() {
        DBCursor expRes = null;
        BSONObject expDet = null;
        BasicBSONList expDets = null;
        BSONObject queryDetail = null;
        try {
            BSONObject matcher = ( BSONObject ) JSON
                    .parse( "{time:{$gt:200,$lt:400}}" );
            BSONObject options = ( BSONObject ) JSON.parse( "{Run:true}" );
            BSONObject query = ( BSONObject ) JSON
                    .parse( "{$and:[{time:{$lt:400}},{time:{$gt:200}}]}" );
            expRes = maincl.explain( matcher, null, null, null, 0, -1, 0,
                    options );
            while ( expRes.hasNext() ) {
                expDets = ( BasicBSONList ) expRes.getNext()
                        .get( "SubCollections" );
                expDet = ( BSONObject ) expDets.get( 0 );
                String scanType = ( String ) expDet.get( "ScanType" );
                String name = ( String ) expDet.get( "IndexName" );
                queryDetail = ( BSONObject ) JSON
                        .parse( expDet.get( "Query" ).toString() );
                if ( !scanType.equals( "ixscan" ) || !indexName.equals( name )
                        || !queryDetail.equals( query ) ) {
                    Assert.fail( "scanType: " + scanType + " " + "name: " + name
                            + " " + "Detail: "
                            + JSON.parse( queryDetail.toString() ) );
                }
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( expRes != null ) {
                expRes.close();
            }
        }
    }
}
