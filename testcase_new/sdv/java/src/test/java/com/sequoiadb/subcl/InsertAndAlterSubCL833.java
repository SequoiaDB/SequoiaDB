package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.List;

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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * FileName: InsertAndUpdate833.java test content:
 * 挂载子表,批量插入数据的同时修改子表_SD.subCL.01.020 testlink case: seqDB-833
 * 
 * @author zengxianquan
 * @date 2016年12月21日
 * @version 1.00
 */
public class InsertAndAlterSubCL833 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace maincs = null;
    private String mainclName = "maincl833";
    private String subclName = "subcl833";
    private List< BSONObject > insertor = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            maincs = sdb.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect  failed," + SdbTestBase.coordUrl
                    + e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( SubCLUtils.getDataGroups( sdb ).size() < 2 ) {
            throw new SkipException(
                    "current environment less than tow groups" );
        }
        createMaincl();
        createAndattachSubcl();
    }

    @AfterClass
    public void tearDown() {
        try {
            maincs.dropCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        try {
            SdbThreadBase insertDataThread = new InsertDataThread();
            SdbThreadBase alterSubclThread = new AlterSubclThread();
            insertDataThread.start();

            Thread.sleep( 300 );

            alterSubclThread.start();

            if ( !insertDataThread.isSuccess() ) {
                Assert.fail( insertDataThread.getErrorMsg() );
            }
            if ( !alterSubclThread.isSuccess() ) {
                Assert.fail( alterSubclThread.getErrorMsg() );
            }

            checkData();
            checkSnapshot8();
        } catch ( InterruptedException e ) {
            e.printStackTrace();
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
            Assert.assertTrue( false, "createMaincl failed " + e.getMessage() );
        }
    }

    public void createAndattachSubcl() {
        DBCollection maincl = null;
        try {
            maincs.createCollection( subclName );
            maincl = maincs.getCollection( mainclName );
            String jsonStr = "{'LowBound':{'a':0},UpBound:{'a':500}}";
            BSONObject options = ( BSONObject ) JSON.parse( jsonStr );
            maincl.attachCollection( SdbTestBase.csName + "." + subclName,
                    options );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    class InsertDataThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            // 构造插入的数据
            for ( int i = 0; i < 500; i++ ) {
                try {
                    BSONObject bson = new BasicBSONObject();
                    bson.put( "a", i );
                    bson.put( "test", "abcdefghijklnmopqrst1234567890" );
                    maincs.getCollection( mainclName ).insert( bson );
                    insertor.add( bson );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != -147
                            && e.getErrorCode() != -190 ) {
                        Assert.fail( e.getMessage() );
                    }
                }
            }

        }
    }

    class AlterSubclThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            DBCollection subcl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                BSONObject options = new BasicBSONObject();
                BSONObject opt = new BasicBSONObject();
                opt.put( "time", 1 );
                options.put( "ShardingKey", opt );
                options.put( "ShardingType", "hash" );
                subcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( subclName );
                subcl.alterCollection( options );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -147 && e.getErrorCode() != -190 ) {
                    Assert.fail( e.getMessage() );
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }

    }

    public void checkData() {
        Sequoiadb db = null;
        DBCollection cl = null;
        DBCursor cursor = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            cl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( subclName );
            BSONObject order = new BasicBSONObject();
            order.put( "a", 1 );
            cursor = cl.query( null, null, order, null );
            BSONObject res = null;
            BSONObject expBso = null;
            int j = 0;
            while ( cursor.hasNext() ) {
                res = cursor.getNext();
                res.removeField( "_id" );
                expBso = insertor.get( j );
                expBso.removeField( "_id" );
                if ( !( res.toString().equals( expBso.toString() ) ) ) {
                    Assert.fail( "failed to query data " );
                }
                j++;
            }
            if ( j != 500 ) {
                Assert.fail( "The data count is error" );
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

    public void checkSnapshot8() {
        Sequoiadb db = null;
        DBCursor cursor = null;
        DBCollection cl = null;
        BSONObject detail = new BasicBSONObject();
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            cl = db.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( subclName );
            cursor = db.getSnapshot( 8, "{Name:'" + cl.getFullName() + "'}",
                    null, null );
            while ( cursor.hasNext() ) {
                detail = cursor.getNext();
                if ( detail.get( "ShardingType" ) != null
                        && detail.get( "ShardingKey" ) != null ) {
                    String shardingType = detail.get( "ShardingType" )
                            .toString();
                    String shardingKey = detail.get( "ShardingKey" ).toString();
                    if ( shardingType != null && shardingKey != null ) {
                        if ( !shardingType.equals( "hash" )
                                || !shardingKey.equals( "{ \"time\" : 1 }" ) ) {
                            Assert.fail( "alter is error" );
                        }
                    }
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( "catalog message is error" );
        }
    }
}
