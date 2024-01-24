package com.sequoiadb.subcl;

import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * FileName: CreateMainCl24.java test content: 创建主表时指定多个分区键 testlink case:
 * seqDB-24
 * 
 * @author zengxianquan
 * @date 2016年12月9日
 * @version 1.00
 */
public class CreateMainCl24 extends SdbTestBase {

    private Sequoiadb db = null;
    private String csName = null;
    private CollectionSpace cs = null;
    private String mainclName = "maincl_24";
    private String subclName = "subcl_24";

    @BeforeClass
    public void setUp() {
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect  failed " + SdbTestBase.coordUrl
                    + e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            db.disconnect();
        }
    }

    @Test
    public void testcreateMainclByManyShardingKey() {
        csName = SdbTestBase.csName;
        try {
            cs = db.getCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to get collectionspace " + "ErrorMsg:\n"
                    + e.getMessage() );
        }
        // 通过不同的shardingkey创建maincl
        createMaincl();
        // 在maincl上挂载子表
        attachSubcl();
        // 检测增删改查的操作
        checkCRUD();
    }

    public void createMaincl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "time", 1 );
        opt.put( "no", -1 );
        options.put( "ShardingKey", opt );
        try {
            cs.createCollection( mainclName, options );
        } catch ( BaseException e ) {
            Assert.fail( "Failed to create maincl  " + "ErrorMsg:\n"
                    + e.getMessage() );
        }
        Assert.assertEquals( cs.isCollectionExist( mainclName ), true,
                "maincl is not exist" );
    }

    public void attachSubcl() {
        try {
            cs.createCollection( subclName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create subcl " + e.getMessage() );
        }
        BSONObject attachOpt = new BasicBSONObject();
        BSONObject lowBound = new BasicBSONObject();
        lowBound.put( "time", 0 );
        lowBound.put( "no", 100 );
        BSONObject upBound = new BasicBSONObject();
        upBound.put( "time", 100 );
        upBound.put( "no", 0 );
        attachOpt.put( "LowBound", lowBound );
        attachOpt.put( "UpBound", upBound );
        try {
            cs.getCollection( mainclName )
                    .attachCollection( csName + "." + subclName, attachOpt );
        } catch ( BaseException e ) {
            Assert.fail( "failed to attach " + "ErrorMsg:\n" + e.getMessage() );
        }
    }

    public void checkCRUD() {
        DBCollection maincl = null;
        try {
            maincl = cs.getCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to get collection" + "ErrorMsg:\n"
                    + e.getMessage() );
        }
        try {
            maincl.insert( "{time:10,no:10,test:'test'}" );

            DBCursor res = maincl.query();
            while ( res.hasNext() ) {
                BSONObject dataRes = res.getNext();
                if ( ( !dataRes.get( "time" ).equals( 10 ) )
                        || ( !dataRes.get( "no" ).equals( 10 ) )
                        || ( !dataRes.get( "test" ).equals( "test" ) ) ) {
                    Assert.fail( "failed to query data " );
                }
            }

            maincl.update( "{time:10}", "{$set:{'test':'update'}}", null );
            DBCursor updateRes = maincl.query();
            while ( updateRes.hasNext() ) {
                BSONObject dataRes = updateRes.getNext();
                if ( ( !dataRes.get( "time" ).equals( 10 ) )
                        || ( !dataRes.get( "no" ).equals( 10 ) )
                        || ( !dataRes.get( "test" ).equals( "update" ) ) ) {
                    Assert.fail( "failed to query data " );
                }
            }

            maincl.delete( "{time:10}" );
            Assert.assertEquals( maincl.getCount(), 0,
                    "failed to delete data" );

        } catch ( BaseException e ) {
            Assert.fail( "failed to CRUD :" + e.getMessage() );
        }
    }
}
