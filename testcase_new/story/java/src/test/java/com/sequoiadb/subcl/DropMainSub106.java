package com.sequoiadb.subcl;

import java.util.Date;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * 用例要求： 1.创建主表CS1并创建主表，在同一CS1创建一张子表1 2.创建子表CS2并创建一张子表2 3.将两张子表挂载到同一主表上
 * 4.通过主表对2张子表插入数据 5.drop子表2的cs2，通过主表插入数据到子表1
 * 6.drop掉主表cs下的子表，再在主表CS下新建子表挂载上来后，通过主表对3张子表数据执行增删改查操作，检查操作结果
 * 
 * @author huangwenhua
 * @Date 2016.12.14
 * @version 1.00
 */
public class DropMainSub106 extends SdbTestBase {
    private CollectionSpace commCS1;
    private CollectionSpace commCS2;
    private DBCollection mainCl1;
    private DBCollection subCl1;
    private DBCollection subCl2;
    private String mainClName1 = "mainCL106_1";
    private String subClName1 = "subCL106_1";
    private String subClName2 = "subCL106_2";
    private String subClName3 = "subCL106_3";
    private String cs2Name = "commCS106_1";
    private Sequoiadb sdb = null;
    private DBCollection subCl3;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CommLib lib = new CommLib();
            if ( lib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip standalone" );
            }
            commCS1 = sdb.getCollectionSpace( SdbTestBase.csName );
            commCS2 = createCS( cs2Name );
            mainCl1 = createCL( mainClName1, commCS1,
                    "{IsMainCL:true,ShardingKey:{a:1}}" );
            subCl1 = createCL( subClName1, commCS1,
                    "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
            subCl2 = createCL( subClName2, commCS2,
                    "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
            subCl3 = createCL( subClName3, commCS1,
                    "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
        } catch ( BaseException e ) {
            Assert.fail( "TestTable106 setUp error:" + e.getMessage() );
        }
        attach();
    }

    @Test
    public void checkTable() {
        insertDatas();
        // drop子表cs2
        sdb.dropCollectionSpace( cs2Name );
        // 检查 drop commCS2
        if ( sdb.isCollectionSpaceExist( cs2Name ) ) {
            Assert.fail( "drop cs:" + cs2Name + "failed." );
        }
        // 第二次执行插入数据
        try {
            mainCl1.insert( "{a:80}" );
            long count1 = mainCl1.getCount( "{a:80}" );
            Assert.assertEquals( 1, count1,
                    "insert fail,the count1 is " + count1 );
        } catch ( BaseException e ) {
            Assert.assertEquals( -135, e.getErrorCode(), e.getMessage() );
        }
        // drop主表cs下的子表
        this.commCS1.dropCollection( subClName1 );
        // 检查 drop subcl1
        if ( this.commCS1.isCollectionExist( subClName1 ) ) {
            Assert.fail( "drop subClName1 fail" );
        }
        // 主表CS下新建子表挂载上来
        try {
            this.mainCl1.attachCollection( subCl3.getFullName(),
                    ( BSONObject ) JSON
                            .parse( "{LowBound:{a:200},UpBound:{a:300}}" ) );

        } catch ( BaseException e ) {
            Assert.fail( "attach error, error description:" + e.getMessage() );
        }
        // 第三次执行增删查改
        try {
            mainCl1.insert( "{a:288}" );
            long count2 = mainCl1.getCount( "{a:288}" );
            Assert.assertEquals( 1, count2,
                    "insert fail,the count2 is " + count2 );
            mainCl1.update( "{a:288}", "{$set:{name:\"upset85\"}}", null );
            long count3 = mainCl1.getCount( "{a:288}" );
            Assert.assertEquals( 1, count3,
                    "updata fail,the count3 is " + count3 );
            mainCl1.delete( "{a:288}" );
            long count4 = mainCl1.getCount( "{a:288}" );
            Assert.assertEquals( 0, count4,
                    "delete fail,the count4 is " + count4 );
            long allCount = mainCl1.getCount();
            Assert.assertEquals( 0, allCount, "delete fail" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        // 超出范围外
        try {
            mainCl1.insert( "{a:368}" );
        } catch ( BaseException e ) {
            Assert.assertEquals( -135, e.getErrorCode(), e.getMessage() );
        }
        try {
            mainCl1.update( "{a:68}", "{$set:{name:\"jose\"}}", null );
        } catch ( BaseException e ) {
            Assert.assertEquals( -135, e.getErrorCode(), e.getMessage() );
        }
        try {
            mainCl1.delete( "{a:68}" );
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
            if ( commCS1.isCollectionExist( subClName3 ) ) {
                commCS1.dropCollection( subClName3 );
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
            Assert.fail( " attach error:" + e.getMessage() );
        }
    }

    public CollectionSpace createCS( String csName ) {
        CollectionSpace commCS = null;
        try {
            if ( !sdb.isCollectionSpaceExist( csName ) ) {
                commCS = sdb.createCollectionSpace( csName );
            } else {
                commCS = sdb.getCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "createCS error:" + e.getMessage() );
        }
        return commCS;
    }

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
        }
        return subCl;
    }

    public void insertDatas() throws BaseException {
        try {
            // 插入数据给第一张表
            BSONObject obj = new BasicBSONObject();
            obj.put( "a", 79 );
            obj.put( "age", 99 );
            obj.put( "name", "Jone" );
            // 插入数据给第二张表
            BSONObject obj1 = new BasicBSONObject();
            obj1.put( "a", 180 );
            obj1.put( "age", 199 );
            obj1.put( "name", "kkk" );
            mainCl1.insert( obj );
            mainCl1.insert( obj1 );
        } catch ( BaseException e ) {
            Assert.assertEquals( -135, e.getErrorCode(), e.getMessage() );
        }
    }

}
