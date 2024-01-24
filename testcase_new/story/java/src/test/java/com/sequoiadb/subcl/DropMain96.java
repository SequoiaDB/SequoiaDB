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
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * 
 * 一个连接重复地创建主表、子表、删除主表 ，同时另一个连接重复地挂载子表 testlink case: seqDB96
 * 
 * @author huangwenhua
 * @Date 2016.12.20
 * @version 1.00
 */
public class DropMain96 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs;
    private CollectionSpace cs2;
    private String mainclName = "maincl_96";
    private String subclName = "subcl_96";
    private DBCollection maincl;
    private DBCollection subcl;

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
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
    public void checkAttch() {
        CheckDrop CheckDropThread = new CheckDrop();
        try {
            CheckDropThread.start();
            maincl.attachCollection( subcl.getFullName(), ( BSONObject ) JSON
                    .parse( "{LowBound:{a:100},UpBound:{a:200}}" ) );
            if ( !CheckDropThread.isSuccess() ) {
                Assert.fail( CheckDropThread.getErrorMsg() );
            }
        } catch ( BaseException e ) {
            Assert.assertEquals( -23, e.getErrorCode(), e.getMessage() );
        } finally {
            CheckDropThread.join();
        }
    }

    class CheckDrop extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cs = db1.getCollectionSpace( SdbTestBase.csName );
                cs.dropCollection( mainclName );
            } catch ( BaseException e ) {
                throw e;
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
            if ( cs2.isCollectionExist( mainclName ) ) {
                cs2.dropCollection( mainclName );
            }
            if ( cs2.isCollectionExist( subclName ) ) {
                cs2.dropCollection( subclName );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                this.sdb.disconnect();
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
            cs2 = sdb.getCollectionSpace( SdbTestBase.csName );
            BSONObject mainObj = ( BSONObject ) JSON
                    .parse( "{IsMainCL:true,ShardingKey:{a:1}}" );
            BSONObject subObj = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
            maincl = cs2.createCollection( mainclName, mainObj );
            subcl = cs2.createCollection( subclName, subObj );
        } catch ( BaseException e ) {
            Assert.fail( "create is faild" + e.getMessage() );
        }
    }
}
