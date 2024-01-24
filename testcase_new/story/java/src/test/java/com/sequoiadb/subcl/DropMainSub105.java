package com.sequoiadb.subcl;

import java.util.Date;

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 用例要求： 1.创建主表CS1并创建主表，在主表CS1下载创建一张子表1 2.创建子表CS2， 并创建一张子表2 3.将两张子表挂载到主表上
 * 4.通过主表插入数据（覆盖2张子表） 5.drop子表2，再通过主表对2张子表的数据执行增删改查操作
 * 
 * @author huangwenhua
 * @Date 2016.12.14
 * @version 1.00
 */
public class DropMainSub105 extends SdbTestBase {
    private CollectionSpace commCS1;
    private CollectionSpace commCS2;
    private DBCollection mainCl1;
    private DBCollection subCl1;
    private DBCollection subCl2;
    private String mainClName1 = "mainCL105_1";
    private String subClName1 = "subCL105_1";
    private String subClName2 = "subCL105_2";
    private String cs2Name = "commCS105_1";
    private Sequoiadb sdb = null;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CommLib lib = new CommLib();
            if ( lib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip standalone" );
            }
            commCS1 = sdb.getCollectionSpace( SdbTestBase.csName );
            commCS2 = createCS();
            mainCl1 = createCL( mainClName1, commCS1,
                    "{IsMainCL:true,ShardingKey:{a:1}}" );
            subCl1 = createCL( subClName1, commCS1,
                    "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
            subCl2 = createCL( subClName2, commCS2,
                    "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
        } catch ( BaseException e ) {
            Assert.fail( "TestTable105 setUp error:" + e.getMessage() );
        }
        attach();
    }

    @Test
    public void checkTable() {
        insertDatas();
        // drop不在同一cs的子表2
        commCS2.dropCollection( subClName2 );
        // 检查 drop subClName2
        if ( commCS2.isCollectionExist( subClName2 ) ) {
            Assert.fail( "drop subClName2 fail" );
        }
        // 第二次执行增删查改
        try {
            mainCl1.insert( "{a:80}" );
            long count1 = mainCl1.getCount( "{a:80}" );
            Assert.assertEquals( 1, count1,
                    "insert fail,the count1 is " + count1 );
            mainCl1.update( "{a:99}", "{$set:{name:\"upset85\"}}", null );
            long count2 = mainCl1.getCount( "{a:99}" );
            Assert.assertEquals( 1, count2,
                    "updata fail,the count2 is" + count2 );
            mainCl1.delete( "{a:99}" );
            long count3 = mainCl1.getCount( "{a:99}" );
            Assert.assertEquals( 0, count3,
                    "delete fail,the count3 is" + count3 );
            long allCount = mainCl1.getCount();
            Assert.assertEquals( 1, allCount, "delete fail" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        // 超出范围外的
        try {
            mainCl1.insert( "{a:170},{name:toke}" );
        } catch ( BaseException e ) {
            Assert.assertEquals( -135, e.getErrorCode(), e.getMessage() );
        }
        try {
            mainCl1.update( "{a:170}", "{$set:{name:\"joke\"}}", null );
        } catch ( BaseException e ) {
            Assert.assertEquals( -135, e.getErrorCode(), e.getMessage() );
        }
        try {
            mainCl1.delete( "{a:170}" );
        } catch ( BaseException e ) {
            Assert.assertEquals( -135, e.getErrorCode(), e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( commCS2.getName() ) ) {
                sdb.dropCollectionSpace( commCS2.getName() );
            }
            if ( commCS1.isCollectionExist( subClName1 ) ) {
                commCS1.dropCollection( subClName1 );
            }
            if ( commCS1.isCollectionExist( mainClName1 ) ) {
                commCS1.dropCollection( mainClName1 );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    public void attach() {
        try {
            mainCl1.attachCollection( subCl1.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{a:1},UpBound:{a:100}}" ) );
            mainCl1.attachCollection( subCl2.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{a:100},UpBound:{a:200}}" ) );

        } catch ( BaseException e ) {
            Assert.fail( "subcl attach  fail:" + e.getMessage() );
        }
    }

    public CollectionSpace createCS() {
        CollectionSpace commCS = null;
        try {
            if ( !sdb.isCollectionSpaceExist( cs2Name ) ) {
                commCS = sdb.createCollectionSpace( cs2Name );
            } else {
                commCS = sdb.getCollectionSpace( cs2Name );
            }
        } catch ( BaseException e ) {
            Assert.fail( "createCS error" + e.getMessage() );
        }
        return commCS;
    }

    // CL多次用到
    public DBCollection createCL( String clName, CollectionSpace cs,
            String option ) {
        DBCollection subCl = null;
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            subCl = cs.createCollection( clName,
                    ( BSONObject ) JSON.parse( option ) );
        } catch ( BaseException e ) {
            Assert.fail( "createCL fail:" + e.getMessage() );
        }
        return subCl;
    }

    public void insertDatas() {
        try {
            // 数据插入第一张表中
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", 99 );
            obj.put( "age", 90 );
            obj.put( "name", "Jones" );
            // 数据插入第二张表中
            BSONObject obj1 = new BasicBSONObject();
            obj1.put( "a", 180 );
            obj1.put( "age", 80 );
            obj1.put( "name", "Jone" );

            mainCl1.insert( obj );
            mainCl1.insert( obj1 );
        } catch ( BaseException e ) {
            Assert.fail( "insertDatas fail:" + e.getMessage() );
        }
    }
}
