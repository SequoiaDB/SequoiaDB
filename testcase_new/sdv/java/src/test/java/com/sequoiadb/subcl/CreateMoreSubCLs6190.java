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

/**
 * FileName: CreateMoreSubCLs6190.java test content:
 * 创建多个子表，指定的ShardingKey字段不同_SD.subCL.01.023 testlink case: seqDB-6190
 * 
 * @author zengxianquan
 * @date 2016年12月22日
 * @version 1.00
 */
public class CreateMoreSubCLs6190 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String mainclName = "maincl6190";
    private List< BSONObject > insertor = new ArrayList<>();

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            sdb.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
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
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23, e.getMessage() );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void testDifferentShardingKeyInSub() {
        createMaincl();
        createSubcls();
        attachSubcls();
        checkCRUD();
    }

    public void createMaincl() {
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingType", "range" );
        BSONObject opt = new BasicBSONObject();
        opt.put( "time", 1 );
        options.put( "ShardingKey", opt );
        try {
            cs.createCollection( mainclName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "createMaincl failed " + e.getMessage() );
        }
    }

    public void createSubcls() {
        try {
            BSONObject option1 = new BasicBSONObject();
            BSONObject shardingKey1 = new BasicBSONObject();
            shardingKey1.put( "a", 1 );
            option1.put( "ShardingKey", shardingKey1 );
            option1.put( "ShardingType", "hash" );
            cs.createCollection( "subcl6190_1", option1 );

            BSONObject option2 = new BasicBSONObject();
            BSONObject shardingKey2 = new BasicBSONObject();
            shardingKey2.put( "b", 1 );
            option2.put( "ShardingKey", shardingKey2 );
            option2.put( "ShardingType", "range" );
            cs.createCollection( "subcl6190_2", option2 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void attachSubcls() {
        DBCollection maincl = null;
        try {
            maincl = cs.getCollection( mainclName );

            String jsonStr1 = "{'LowBound':{'time':0},UpBound:{'time':100}}";
            BSONObject option1 = ( BSONObject ) JSON.parse( jsonStr1 );
            maincl.attachCollection( SdbTestBase.csName + ".subcl6190_1",
                    option1 );

            String jsonStr2 = "{'LowBound':{'time':100},UpBound:{'time':200}}";
            BSONObject option2 = ( BSONObject ) JSON.parse( jsonStr2 );
            maincl.attachCollection( SdbTestBase.csName + ".subcl6190_2",
                    option2 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void checkCRUD() {
        DBCollection maincl = null;
        DBCursor cursor = null;
        try {
            // 检验插入
            maincl = cs.getCollection( mainclName );
            for ( int i = 0; i < 100; i++ ) {
                BSONObject bson = new BasicBSONObject();
                bson.put( "time", i );
                bson.put( "test", "abcdefghijkln34567890" );
                insertor.add( bson );
            }
            maincl.insert( insertor );
            BSONObject order = new BasicBSONObject();
            order.put( "time", 1 );
            cursor = maincl.query( null, null, order, null );
            BSONObject res = null;
            BSONObject expBso = null;
            int j = 0;
            while ( cursor.hasNext() ) {
                res = cursor.getNext();
                res.removeField( "_id" );
                expBso = insertor.get( j );
                expBso.removeField( "_id" );
                if ( !( res.toString().equals( expBso.toString() ) ) ) {
                    System.out.println( "act: " + res.toString() + "\n"
                            + "expect: " + expBso.toString() );
                    Assert.fail( "failed to check data " );
                }
                j++;
            }
            // 检验更新
            maincl.update( null, "{$set:{'test':'update'}}", null );
            DBCursor updateRes = maincl.query( null, null, order, null );
            BSONObject dataRes = null;
            int i = 0;
            while ( updateRes.hasNext() ) {
                dataRes = updateRes.getNext();
                dataRes.removeField( "_id" );
                if ( !( dataRes.toString()
                        .equals( "{ \"test\" : \"update\" , \"time\" : " + i
                                + " }" ) ) ) {
                    Assert.fail( "failed to check data " );
                }
                i++;
            }
            // 检验删除
            maincl.delete( "{time:{$gte:90}}" );
            Assert.assertEquals( maincl.getCount(), 90,
                    "failed to delete data" );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( "failed to CRUD :" + e.getMessage() );
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
        }
    }
}
