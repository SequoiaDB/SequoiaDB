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
 * 1. 创建主表、子表A、子表B 2. 一个连接重复地挂载分离子表A，同时另一个连接重复地挂载分离子表B，且子表A和子表B挂载范围不同 testlink
 * case: seqDB97
 * 
 * @author huangwenhua
 * @Date 2016.12.20
 * @version 1.00
 */
public class DetachSub97 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String mainclName = "maincL97";
    private String subclName1 = "subcL97_1111";
    private String subclName2 = "subcL97_2111";
    private int loopNum = 100;

    @BeforeClass
    public void setUp() {
        try {

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            CommLib lib = new CommLib();
            if ( lib.isStandAlone( sdb ) ) {
                throw new SkipException( "skip standalone" );
            }

            cs = sdb.getCollectionSpace( SdbTestBase.csName );

            BSONObject mainObj = ( BSONObject ) JSON
                    .parse( "{IsMainCL:true,ShardingKey:{a:1}}" );
            cs.createCollection( mainclName, mainObj );

            BSONObject subObj1 = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
            cs.createCollection( subclName1, subObj1 );

            BSONObject subObj2 = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{a:1},ShardingType:\"hash\"}" );
            cs.createCollection( subclName2, subObj2 );

        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @Test
    public void test() {
        BSONObject attachOptions1 = ( BSONObject ) JSON
                .parse( "{LowBound:{a:1},UpBound:{a:100}}" );
        CheckAttach CheckAttachThread1 = new CheckAttach(
                SdbTestBase.csName + "." + subclName1, attachOptions1 );

        BSONObject attachOptions2 = ( BSONObject ) JSON
                .parse( "{LowBound:{a:100},UpBound:{a:200}}" );
        CheckAttach CheckAttachThread2 = new CheckAttach(
                SdbTestBase.csName + "." + subclName2, attachOptions2 );

        CheckAttachThread1.start();
        CheckAttachThread2.start();

        if ( !( CheckAttachThread1.isSuccess()
                && CheckAttachThread2.isSuccess() ) ) {
            Assert.fail( CheckAttachThread1.getErrorMsg()
                    + CheckAttachThread1.getErrorMsg() );
        }
    }

    class CheckAttach extends SdbThreadBase {
        String clFullName;
        BSONObject attachOptions;

        public CheckAttach( String clFullName, BSONObject attachOptions ) {
            this.clFullName = clFullName;
            this.attachOptions = attachOptions;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db_parrell = null;
            DBCollection maincl_parrell = null;

            try {
                db_parrell = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                maincl_parrell = db_parrell
                        .getCollectionSpace( SdbTestBase.csName )
                        .getCollection( mainclName );
                for ( int i = 0; i < loopNum; i++ ) {
                    maincl_parrell.attachCollection( clFullName,
                            attachOptions );
                    maincl_parrell.detachCollection( clFullName );
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
                throw e;
            } finally {
                db_parrell.disconnect();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( subclName1 );
            cs.dropCollection( subclName2 );
            cs.dropCollection( mainclName );
        } catch ( BaseException e ) {
            Assert.assertEquals( e.getErrorCode(), -23 );
        } finally {
            sdb.disconnect();
        }
    }
}
