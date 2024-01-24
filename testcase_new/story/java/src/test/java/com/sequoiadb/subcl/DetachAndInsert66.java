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
 * 
 * FileName: DetachAndInsert66 test content: detach子表后，验证数据操作 testlink
 * case:seqDB-66
 * 
 * @author zengxianquan
 * @date 2016年12月27日
 * @version 1.00
 * @other 对应问题单SEQUOIADBMAINSTREAM-2198，已经修复，用例已经开启
 */
public class DetachAndInsert66 extends SdbTestBase {
    private Sequoiadb sdb1;
    private Sequoiadb sdb2;
    private String clName1 = "subcl66_1";
    private String clName2 = "subcl66_2";
    private String clName3 = "subcl66_3";
    private String mainclName = "maincl66";
    private List< String > addressList = null;

    @BeforeClass
    public void setUp() {
        try ( Sequoiadb tmpdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            if ( SubCLUtils.isStandAlone( tmpdb ) ) {
                throw new SkipException( "is standalone skip testcase" );
            }
            addressList = SubCLUtils.getNodeAddress( tmpdb, "SYSCoord" );
            if ( addressList.size() < 2 ) {
                throw new SkipException( "coord quantity is less than 2" );
            }
            sdb1 = new Sequoiadb( addressList.get( 0 ), "", "" );
            sdb2 = new Sequoiadb( addressList.get( 1 ), "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() + e.getMessage() );
        }
        createCl( sdb1 );
        attach( sdb1 );
        insertData( sdb1 );
    }

    @AfterClass
    public void tearDown() {
        CollectionSpace cs = null;
        try ( Sequoiadb tmpdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            cs = tmpdb.getCollectionSpace( SdbTestBase.csName );
            if ( cs.isCollectionExist( mainclName ) ) {
                cs.dropCollection( mainclName );
            }
            if ( cs.isCollectionExist( clName1 ) ) {
                cs.dropCollection( clName1 );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            sdb1.close();
            sdb2.close();
        }
    }

    @Test
    public void test() {
        // detach 子表一
        detach( sdb1 );
        // 检验子表被detach，通过主表无法增删改查该子表中的数据
        checkCRUD( sdb2 );
        // detach其中一张子表不影响其他子表的数据，增删改查操作功能正常
        checkUpdate( sdb2, 100, 300 );
        checkDelete( sdb2, 100, 0 );
        checkInsert( sdb2, 100, 300 );
    }

    /**
     * 检验detach的子表后，在主表操作增删改查会不会对原子表数据有影响
     * 
     * @param db
     */
    public void checkCRUD( Sequoiadb db ) {
        CollectionSpace cs = null;
        DBCollection maincl = null;
        DBCollection subcl = null;
        DBCursor queryCur = null;
        DBCursor res = null;
        try {
            db.setSessionAttr( new BasicBSONObject( "PreferedInstance", "M" ) );
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );
            subcl = cs.getCollection( clName1 );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( "Fail to check CRUD" + e.getMessage() );
        }

        try {
            maincl.insert( "{age:1,name:'xiaohaong'}" );
            Assert.fail( "insert success" );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -135, e.getMessage() );
        }

        try {
            // 检验在主表做查该删操作，是否对子表数据有影响
            queryCur = maincl.query( "{age:1}", null, null, null );
            if ( queryCur.hasNext() ) {
                Assert.fail( "query success" );
            }
            maincl.update( null, "{$set:{'test':'update'}}", null );
            maincl.delete( "{age:1}" );
            // 经过在主表做增删改查后，检验子表的数据是否有改变
            List< BSONObject > insertor = new ArrayList<>();
            for ( int i = 0; i < 100; i++ ) {
                BSONObject bson = new BasicBSONObject();
                bson.put( "age", i );
                bson.put( "name", "xiaohong" );
                bson.put( "test", "test" );
                insertor.add( bson );
            }
            BSONObject order = new BasicBSONObject();
            order.put( "age", 1 );
            res = subcl.query( null, null, order, null );
            int i = 0;
            while ( res.hasNext() ) {
                BSONObject dataRes = res.getNext();
                dataRes.removeField( "_id" );
                if ( !insertor.get( i ).equals( dataRes ) ) {
                    Assert.fail( "failed to query data " );
                }
                i++;
            }
            Assert.assertEquals( 100, subcl.getCount(),
                    "the data count is error" );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    private void insertData( Sequoiadb db ) {
        CollectionSpace cs = null;
        DBCollection maincl = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );
            // 检验插入
            List< BSONObject > insertor = new ArrayList<>();
            for ( int i = 0; i < 300; i++ ) {
                BSONObject bson = new BasicBSONObject();
                bson.put( "age", i );
                bson.put( "name", "xiaohong" );
                bson.put( "test", "test" );
                insertor.add( bson );
            }
            maincl.bulkInsert( insertor, DBCollection.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    public void createCl( Sequoiadb db ) {
        CollectionSpace cs = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            BSONObject mainOpt = ( BSONObject ) JSON.parse(
                    "{IsMainCL:true,ShardingKey:{age:1},ShardingType:\"range\"}" );
            BSONObject subOpt = ( BSONObject ) JSON.parse(
                    "{ShardingKey:{age:1},ShardingType:\"hash\",Partition:1024}" );
            cs.createCollection( mainclName, mainOpt );

            cs.createCollection( clName1, subOpt );
            cs.createCollection( clName2, subOpt );
            cs.createCollection( clName3, subOpt );
        } catch ( BaseException e ) {
            Assert.fail( "create is faild:" + e.getMessage() );
        }
    }

    public void attach( Sequoiadb db ) {
        CollectionSpace cs = null;
        DBCollection maincl = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );

            BSONObject opt1 = ( BSONObject ) JSON
                    .parse( "{LowBound:{age:0},UpBound:{age:100}}" );
            BSONObject opt2 = ( BSONObject ) JSON
                    .parse( "{LowBound:{age:100},UpBound:{age:200}}" );
            BSONObject opt3 = ( BSONObject ) JSON
                    .parse( "{LowBound:{age:200},UpBound:{age:300}}" );
            maincl.attachCollection( SdbTestBase.csName + "." + clName1, opt1 );
            maincl.attachCollection( SdbTestBase.csName + "." + clName2, opt2 );
            maincl.attachCollection( SdbTestBase.csName + "." + clName3, opt3 );
        } catch ( BaseException e ) {
            Assert.fail( "attach is error:" + e.getMessage() );
        }
    }

    public void detach( Sequoiadb db ) {
        CollectionSpace cs = null;
        DBCollection maincl = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );
            maincl.detachCollection( SdbTestBase.csName + "." + clName1 );
        } catch ( BaseException e ) {
            Assert.fail( "detach is faild:" + e.getMessage() );
        }
    }

    public void checkInsert( Sequoiadb db, int lowBound, int upBound ) {
        CollectionSpace cs = null;
        DBCollection maincl = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );
            // 检验插入
            List< BSONObject > insertor = new ArrayList<>();
            for ( int i = lowBound; i < upBound; i++ ) {
                BSONObject bson = new BasicBSONObject();
                bson.put( "age", i );
                bson.put( "name", "xiaohong" );
                bson.put( "test", "test" );
                insertor.add( bson );
            }
            maincl.bulkInsert( insertor, DBCollection.FLG_INSERT_CONTONDUP );
            // 检验数据的正确性
            BSONObject order = new BasicBSONObject();
            order.put( "age", 1 );
            DBCursor res = maincl.query( null, null, order, null );
            int i = 0;
            while ( res.hasNext() ) {
                BSONObject dataRes = res.getNext();
                if ( !insertor.get( i ).equals( dataRes ) ) {
                    Assert.fail( "failed to query data " );
                }
                i++;
            }
            Assert.assertEquals( maincl.getCount(), upBound - lowBound,
                    "failed to delete data" );
        } catch ( BaseException e ) {
            Assert.fail( "failed to insert :" + e.getMessage() );
        }

    }

    public void checkUpdate( Sequoiadb db, int lowBound, int upBound ) {
        CollectionSpace cs = null;
        DBCollection maincl = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );
            // 检验更新
            List< BSONObject > updateList = new ArrayList<>();
            for ( int i = lowBound; i < upBound; i++ ) {
                BSONObject bson = new BasicBSONObject();
                bson.put( "age", i );
                bson.put( "name", "xiaohong" );
                bson.put( "test", "update" );
                updateList.add( bson );
            }
            maincl.update( null, "{$set:{'test':'update'}}", null );
            BSONObject order = new BasicBSONObject();
            order.put( "age", 1 );
            DBCursor updateRes = maincl.query( null, null, order, null );
            int i = 0;
            while ( updateRes.hasNext() ) {
                BSONObject dataRes = updateRes.getNext();
                dataRes.removeField( "_id" );
                if ( !updateList.get( i ).equals( dataRes ) ) {
                    Assert.fail( "failed to query data " );
                }
                i++;
            }
            Assert.assertEquals( maincl.getCount(), upBound - lowBound,
                    "failed to delete data" );
        } catch ( BaseException e ) {
            Assert.fail( "failed to update :" + e.getMessage() );
        }
    }

    public void checkDelete( Sequoiadb db, int gte, int count ) {
        CollectionSpace cs = null;
        DBCollection maincl = null;
        try {
            cs = db.getCollectionSpace( SdbTestBase.csName );
            maincl = cs.getCollection( mainclName );
            // 检验删除
            maincl.delete( "{age:{$gte:" + gte + "}}" );
            Assert.assertEquals( maincl.getCount(), count,
                    "failed to delete data" );

        } catch ( BaseException e ) {
            Assert.fail( "failed to delete :" + e.getMessage() );
        }
    }
}
