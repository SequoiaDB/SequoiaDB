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
 * 1. 创建主表、子表 2 一个连接重复地挂载分离子表，同时另一个连接重复地挂载分离子表 testlink case: seqDB97
 * 
 * @author huangwenhua
 * @Date 2016.12.20
 * @version 1.00
 */
public class DetachSub98 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private String mainclName = "maincL98";
    private String subclName1 = "subcL98_1";
    private DBCollection subcl1;
    private DBCollection maincl;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CommLib lib = new CommLib();
            if ( lib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip standalone" );
            }

        } catch ( BaseException e ) {
            Assert.fail( " TestCase98 setUp error:" + e.getMessage() );
        }
        createCL();
    }

    @Test
    public void checkAttach1() {
        Sequoiadb db1 = null;
        try {
            db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            maincl = db1.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( mainclName );
            maincl.attachCollection( subcl1.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{a:1},UpBound:{a:100}}" ) );
            maincl.attachCollection( subcl1.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{a:1},UpBound:{a:100}}" ) );
            maincl.detachCollection( subcl1.getFullName() );
            maincl.detachCollection( subcl1.getFullName() );
        } catch ( BaseException e ) {
            Assert.assertEquals( -235, e.getErrorCode(), e.getMessage() );
        } finally {
            if ( db1 != null ) {
                db1.disconnect();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( mainclName ) ) {
                cs.dropCollection( mainclName );
            }
            if ( cs.isCollectionExist( subclName1 ) ) {
                cs.dropCollection( subclName1 );
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
                    .parse( "{IsMainCL:true,ShardingKey:{a:1}}" );
            BSONObject subObj = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
            cs.createCollection( mainclName, mainObj );
            subcl1 = cs.createCollection( subclName1, subObj );
        } catch ( BaseException e ) {
            Assert.fail( "create is faild" + e.getMessage() );
        }
    }
}
