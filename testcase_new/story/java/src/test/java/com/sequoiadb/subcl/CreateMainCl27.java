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
 * FileName: CreateMainCl27.java test content: 使用非默认参数创建主表功能验证 testlink case:
 * seqDB-27
 * 
 * @author zengxianquan
 * @date 2016年12月9日
 * @version 1.00
 */
public class CreateMainCl27 extends SdbTestBase {
    private Sequoiadb db = null;
    private String csName = null;
    private CollectionSpace cs = null;
    private String mainclName = "maincl_27";
    private String subclName = "subcl_27";

    @BeforeClass
    public void setUp() {
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "connect  failed," + SdbTestBase.coordUrl
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
            Assert.fail(
                    "failed to drop maincl " + "ErrorMsg:\n" + e.getMessage() );
        } finally {
            db.disconnect();
        }
    }

    @Test
    public void testMainclByAppointSameShardingKey() {
        csName = SdbTestBase.csName;
        try {
            cs = db.getCollectionSpace( csName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to get collectionspace" + "ErrorMsg:\n"
                    + e.getMessage() );
        }
        // 使用非默认值创建主表
        createMaincl();
        // 挂载子表
        attachSubcl();
        // 检验主表是否能正常使用CUID
        checkCRUD();
    }

    public void createMaincl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "time", 1 );
        options.put( "ShardingKey", opt );
        options.put( "ShardingType", "range" );
        options.put( "Partition", 2048 );
        options.put( "ReplSize", 0 );
        options.put( "Compressed", true );
        options.put( "CompressionType", "lzw" );
        options.put( "AutoSplit", false );
        options.put( "EnsureShardingIndex", false );
        try {
            cs.createCollection( mainclName, options );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create maincl " + e.getMessage() );
        }
    }

    public void attachSubcl() {
        try {
            cs.createCollection( subclName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to create subclName " + e.getMessage() );
        }
        BSONObject attachOpt = new BasicBSONObject();
        BSONObject lowBound = new BasicBSONObject();
        BSONObject upBound = new BasicBSONObject();
        lowBound.put( "time", 0 );
        upBound.put( "time", 100 );
        attachOpt.put( "LowBound", lowBound );
        attachOpt.put( "UpBound", upBound );
        try {
            cs.getCollection( mainclName )
                    .attachCollection( csName + "." + subclName, attachOpt );
        } catch ( BaseException e ) {
            Assert.fail( "failed to attach " + e.getMessage() );
        }
    }

    public void checkCRUD() {
        DBCollection maincl = null;
        try {
            maincl = cs.getCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.fail( "failed to get collection" );
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
