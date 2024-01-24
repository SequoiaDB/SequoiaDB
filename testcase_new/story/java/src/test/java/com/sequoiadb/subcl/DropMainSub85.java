package com.sequoiadb.subcl;

import java.util.ArrayList;
import java.util.Date;
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

/**
 * FileName: DropMainSub85 test content: detach子表后，drop主表 testlink case:
 * seqDB-85
 * 
 * @author zengxianquan
 * @date 2016年12月12日
 * @version 1.00
 */
public class DropMainSub85 extends SdbTestBase {
    private Sequoiadb db = null;
    private DBCollection maincl = null;
    private CollectionSpace cs = null;
    private DBCollection subcl = null;
    private String subclName = "subcl_85";
    private String mainclName = "maincl_85";

    @BeforeClass
    public void setUp() {
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            db.setSessionAttr( ( ( BSONObject ) JSON
                    .parse( "{\"PreferedInstance\":\"M\"}" ) ) );
            cs = db.getCollectionSpace( SdbTestBase.csName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
        if ( SubCLUtils.isStandAlone( db ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( subclName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            db.disconnect();
        }
    }

    @Test
    public void testDropSubNoDetach() {

        // 创建主子表，并挂载主子表
        createAndAttachSubcl();

        // 插入数据
        List< BSONObject > insertor = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            BSONObject bson = new BasicBSONObject();
            bson.put( "time", i );
            bson.put( "test", "testData" );
            insertor.add( bson );
        }
        try {
            maincl.bulkInsert( insertor, DBCollection.FLG_INSERT_CONTONDUP );
            // System.out.println(subcl.getCount());
            // detach子表
            maincl.detachCollection( SdbTestBase.csName + "." + subclName );
            // 删除主表
            cs.dropCollection( mainclName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
        // 检验是否还存在主子表
        checkData();
        // 检验增删改查
        checkCRUD();
    }

    public void createAndAttachSubcl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        BSONObject opt = new BasicBSONObject();
        opt.put( "time", 1 );
        options.put( "ShardingKey", opt );

        try {
            maincl = cs.createCollection( mainclName, options );
            subcl = cs.createCollection( subclName );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
        BSONObject attachOpt = new BasicBSONObject();
        BSONObject lowBound = new BasicBSONObject();
        BSONObject upBound = new BasicBSONObject();
        lowBound.put( "time", 0 );
        upBound.put( "time", 100 );
        attachOpt.put( "LowBound", lowBound );
        attachOpt.put( "UpBound", upBound );
        try {
            maincl.attachCollection( SdbTestBase.csName + "." + subclName,
                    attachOpt );
        } catch ( BaseException e ) {
            Assert.fail( "failed to attach subcl" + e.getMessage() );
        }
    }

    public void checkData() {
        BSONObject order = new BasicBSONObject();
        order.put( "time", 1 );
        DBCursor cursor = null;
        try {
            subcl = cs.getCollection( subclName );
            cursor = subcl.query( null, null, order, null );
            BSONObject res;
            int j = 0;
            while ( cursor.hasNext() ) {
                res = cursor.getNext();
                if ( ( !res.get( "time" ).equals( j ) )
                        || ( !res.get( "test" ).equals( "testData" ) ) ) {
                    Assert.fail( "The data is not same" );
                }
                j++;
            }
            if ( j != 100 ) {
                Assert.fail( "Data count is error" );
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

    public void checkCRUD() {
        try {
            subcl.insert( "{time:10,no:10,test:'test'}" );
            // 在主表查询数据删除了的子表数据
            BSONObject matcher = new BasicBSONObject();
            matcher.put( "no", 10 );
            DBCursor res = subcl.query( matcher, null, null, null );
            while ( res.hasNext() ) {
                BSONObject dataRes = res.getNext();

                if ( ( !dataRes.get( "time" ).equals( 10 ) )
                        || ( !dataRes.get( "no" ).equals( 10 ) )
                        || ( !dataRes.get( "test" ).equals( "test" ) ) ) {
                    Assert.fail( "failed to query data " );
                }
            }

            subcl.update( "{no:10}", "{$set:{'test':'update'}}", null );

            DBCursor updateRes = subcl.query( matcher, null, null, null );
            while ( updateRes.hasNext() ) {
                BSONObject dataRes = updateRes.getNext();
                if ( ( !dataRes.get( "time" ).equals( 10 ) )
                        || ( !dataRes.get( "no" ).equals( 10 ) )
                        || ( !dataRes.get( "test" ).equals( "update" ) ) ) {
                    Assert.fail( "failed to query data " );
                }
            }
            subcl.delete( "{time:10}" );
            DBCursor deleteRes = subcl.query( matcher, null, null, null );
            if ( deleteRes.hasNext() ) {
                Assert.assertEquals( maincl.getCount(), 0,
                        "failed to delete data" );
            }
        } catch ( BaseException e ) {
            Assert.fail( "failed to CRUD :" + e.getMessage() );
        }
    }
}
