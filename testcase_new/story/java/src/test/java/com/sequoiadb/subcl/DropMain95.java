package com.sequoiadb.subcl;

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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 
 * 1 一个连接重复地创建主表、子表、attach子表、删除主表，同时另一个连接重复地插入数据 testlink case: seqDB95
 * 
 * @author huangwenhua
 * @Date 2016.12.20
 * @version 1.00
 */
public class DropMain95 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private DBCollection maincl;
    private DBCollection subcl;
    private String mainclName = "maincl_95";
    private String subclName = "subcl_95";

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            // 判断是否是独立模式
            CommLib lib = new CommLib();
            if ( lib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip standalone" );
            }
        } catch ( BaseException e ) {
            Assert.fail( " TestCase95 setUp error:" + e.getMessage() );
        }
        createCL();
    }

    @Test
    public void insertData() {
        DropCl DropClThread = new DropCl();
        Sequoiadb db2 = null;
        try {
            db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            DBCollection cl1 = db2.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( mainclName );
            BSONObject bson;
            DropClThread.start();
            if ( cl1 == null ) {
                return;
            }
            for ( int i = 0; i < 1000; i++ ) {
                bson = new BasicBSONObject();
                bson.put( "b", 5 );
                bson.put( "age", i );
                cl1.insert( bson );
            }
            if ( !DropClThread.isSuccess() ) {
                Assert.fail( DropClThread.getErrorMsg() );
            }
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != -23 && e.getErrorCode() != -190 ) {
                throw e;
            }
            return;
        } finally {
            if ( db2 != null ) {
                db2.disconnect();
            }
            DropClThread.join();
        }
    }

    class DropCl extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db1.getCollectionSpace( csName )
                        .dropCollection( maincl.getName() );
            } catch ( BaseException e ) {
                Assert.assertEquals( e.getErrorCode(), -190 );
            } finally {
                if ( db1 != null ) {
                    db1.disconnect();
                }
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( mainclName ) ) {
                cs.dropCollection( mainclName );
            }
            if ( cs.isCollectionExist( subclName ) ) {
                cs.dropCollection( subclName );
            }

        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    public void createCL() {
        try {
            if ( !sdb.isCollectionSpaceExist( SdbTestBase.csName ) ) {
                sdb.createCollectionSpace( SdbTestBase.csName );
            }
        } catch ( BaseException e ) {
            // -33 CS exist,ignore exceptions
            Assert.assertEquals( -33, e.getErrorCode(), e.getMessage() );
        }
        try {
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            BSONObject mainObj = ( BSONObject ) JSON
                    .parse( "{IsMainCL:true,ShardingKey:{b:1}}" );
            BSONObject subObj = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{b:1},ShardingType:\"hash\"}" );
            maincl = cs.createCollection( mainclName, mainObj );
            subcl = cs.createCollection( subclName, subObj );
        } catch ( BaseException e ) {
            Assert.fail( "create is faild" + e.getMessage() );
        }
        try {
            maincl.attachCollection( subcl.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{b:1},UpBound:{b:100}}" ) );
        } catch ( BaseException e ) {
            Assert.fail( "attach error:" + e.getMessage() );
        }
    }
}
