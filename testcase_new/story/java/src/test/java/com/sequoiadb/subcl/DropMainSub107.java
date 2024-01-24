package com.sequoiadb.subcl;

import java.util.Date;

import org.bson.BSONObject;
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
 * 用例要求： 1.创建主表CS并创建主表，在主表CS下创建一张子表 2.创建子表CS并创建一张子表 3.将两张子表挂载到同一主表
 * 4.drop掉子表CS及主表cs下的子表，新建子表CS 及子表并挂载到主表上，通过主表对3张子表的范围执行增删改查操作，检查操作结果
 * 
 * @author huangwenhua
 * @Date 2016.12.14
 * @version 1.00
 */
public class DropMainSub107 extends SdbTestBase {
    private CollectionSpace commCS1;
    private CollectionSpace commCS2;
    private CollectionSpace commCS3;
    private DBCollection mainCl1;
    private DBCollection subCl1;
    private DBCollection subCl2;
    private DBCollection subCl3;
    private String mainClName1 = "mainCL107_1";
    private String subClName1 = "subCL107_1";
    private String subClName2 = "subCL107_2";
    private String subClName3 = "subCL107_3";
    private String cs2Name = "commCS107_2";
    private String cs3Name = "commCS107_3";
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
            commCS2 = createCS( cs2Name );
            mainCl1 = createCL( mainClName1, commCS1,
                    "{IsMainCL:true,ShardingKey:{a:1}}" );
            subCl1 = createCL( subClName1, commCS1,
                    "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
            subCl2 = createCL( subClName2, commCS2,
                    "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
        } catch ( BaseException e ) {
            Assert.fail( "TestTable107 setUp error:" + e.getMessage() );
        }
        attach();
    }

    @Test
    public void testMasterTable() {
        // drop不是主表的cs
        sdb.dropCollectionSpace( cs2Name );
        // 检查 drop commCS2
        if ( sdb.isCollectionSpaceExist( cs2Name ) ) {
            Assert.fail( "drop cs:" + cs2Name + "failed." );
        }
        // drop主表cs下的子表
        commCS1.dropCollection( subClName1 );
        // 检查 drop subClName1
        if ( commCS1.isCollectionExist( subClName1 ) ) {
            Assert.fail( "drop subClName1 fail" );
        }
        // 创建commCS3
        commCS3 = createCS( cs3Name );
        // 创建子表3
        subCl3 = createCL( subClName3, commCS3,
                "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
        // 子表3挂在主表上
        try {
            mainCl1.attachCollection( subCl3.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{a:300},UpBound:{a:400}}" ) );

        } catch ( BaseException e ) {
            Assert.fail( " subcl3 attach error:" + e.getMessage() );
        }
        // 执行范围内增删查改
        try {
            mainCl1.insert( "{a:388}" );
            long count1 = mainCl1.getCount( "{a:388}" );
            Assert.assertEquals( 1, count1,
                    "insert fail,the count1 is" + count1 );
            mainCl1.update( "{a:388}", "{$set:{name:\"upset85\"}}", null );
            long count2 = mainCl1.getCount( "{a:388}" );
            Assert.assertEquals( 1, count2,
                    "updata fail,the count2 is" + count2 );
            mainCl1.delete( "{a:388}" );
            long count3 = mainCl1.getCount( "{a:388}" );
            Assert.assertEquals( 0, count3,
                    "delete fail,the count3 is" + count3 );
            long allCount = mainCl1.getCount();
            Assert.assertEquals( 0, allCount, "delete fail" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        // 执行范围外的增删查改
        try {
            mainCl1.insert( "{a:88}" );
        } catch ( BaseException e ) {
            Assert.assertEquals( -135, e.getErrorCode(), e.getMessage() );
        }
        try {
            mainCl1.update( "{a:88}", "{$set:{name:\"upset85\"}}", null );
        } catch ( BaseException e ) {
            Assert.assertEquals( -135, e.getErrorCode(), e.getMessage() );
        }
        try {
            mainCl1.delete( "{a:88}" );
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
            if ( sdb.isCollectionSpaceExist( commCS3.getName() ) ) {
                sdb.dropCollectionSpace( commCS3.getName() );
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
            this.mainCl1.attachCollection( subCl1.getFullName(),
                    ( BSONObject ) JSON
                            .parse( "{LowBound:{a:1},UpBound:{a:100}}" ) );
            this.mainCl1.attachCollection( subCl2.getFullName(),
                    ( BSONObject ) JSON
                            .parse( "{LowBound:{a:100},UpBound:{a:200}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( " subCl2 attach error:" + e.getMessage() );
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
            Assert.fail( "createCS error" + e.getMessage() );
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
            Assert.fail( "createCl error" + e.getMessage() );
        }
        return subCl;
    }

}
