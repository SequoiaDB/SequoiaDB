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
 * 用例要求： 1.创建主表，子表，主表与子表属于不同表空间； 2.删除子表空间； 3.从主表上做count操作，确认查询是否Hang住；(操作多做几次)
 * 
 * @author huangwenhua
 * @Date 2016.12.14
 * @version 1.00
 */
public class DropMain100 extends SdbTestBase {
    private CollectionSpace commCS1;
    private CollectionSpace commCS2;
    private DBCollection mainCl1;
    private DBCollection subCl1;
    private String mainClName1 = "mainCL100_1";
    private String subClName1 = "subCL100_1";
    private String cs2Name = "commCS100_2";
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
            subCl1 = createCL( subClName1, commCS2,
                    "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
        } catch ( BaseException e ) {
            Assert.fail( "TestCase100 setUp error:" + e.getMessage() );
        }
        try {
            this.mainCl1.attachCollection( subCl1.getFullName(),
                    ( BSONObject ) JSON
                            .parse( "{LowBound:{a:1},UpBound:{a:100}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( " subCl1 attach error:" + e.getMessage() );
        }
    }

    @Test
    public void checkCount() {
        // drop子表的cs
        sdb.dropCollectionSpace( cs2Name );
        // 检查 drop commCS2
        if ( sdb.isCollectionSpaceExist( cs2Name ) ) {
            Assert.fail( "drop cs:" + cs2Name + "failed." );
        }
        Sequoiadb db2 = null;
        try {
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl2 = db2.getCollectionSpace( csName )
                    .getCollection( this.mainClName1 );
            for ( int i = 0; i < 1000; i++ ) {
                cl2.getCount();
            }
        } catch ( BaseException e ) {
            Assert.fail( "count error" + e.getMessage() );
        } finally {
            if ( db2 != null ) {
                db2.disconnect();
            }
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

    public CollectionSpace createCS( String csName ) {
        CollectionSpace CS = null;
        try {
            if ( !sdb.isCollectionSpaceExist( csName ) ) {
                CS = sdb.createCollectionSpace( csName );
            } else {
                CS = sdb.getCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            Assert.fail( "createCS error" + e.getMessage() );
        }
        return CS;
    }

    public DBCollection createCL( String clName, CollectionSpace cs,
            String option ) {
        DBCollection Cl = null;
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            Cl = cs.createCollection( clName,
                    ( BSONObject ) JSON.parse( option ) );
        } catch ( BaseException e ) {
            Assert.fail( "createCl error" + e.getMessage() );
        }
        return Cl;
    }
}
