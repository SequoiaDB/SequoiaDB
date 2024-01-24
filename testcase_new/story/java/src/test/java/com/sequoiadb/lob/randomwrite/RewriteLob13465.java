package com.sequoiadb.lob.randomwrite;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-13465 : truncateLob过程中执行truncate cl
 * @Author linsuqiang
 * @Date 2017-11-16
 * @Version 1.00
 */

/*
 * 1、指定oid执行truncatelob操作，删除超过指定长度部分的数据 2、删除lob数据过程中执行truncate cl操作 3、检查操作结果
 */

public class RewriteLob13465 extends SdbTestBase {

    private final String csName = "lobTruncate13465";
    private final String clName = "lobTruncate13465";
    private final int lobPageSize = 4 * 1024; // 4k
    private final int lobSize = 100 * 1024 * 1024;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {

        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

            // create cs cl
            BSONObject csOpt = ( BSONObject ) JSON
                    .parse( "{LobPageSize: " + lobPageSize + "}" );
            cs = sdb.createCollectionSpace( csName, csOpt );
            BSONObject clOpt = ( BSONObject ) JSON
                    .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
            cl = cs.createCollection( clName, clOpt );

        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @Test
    public void testLob() {
        try {
            byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
            ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );

            TruncateCLThread trunCLThrd = new TruncateCLThread();
            TruncateLobThread trunLobThrd = new TruncateLobThread( oid );

            trunCLThrd.start();
            trunLobThrd.start();

            Assert.assertTrue( trunCLThrd.isSuccess(),
                    trunCLThrd.getErrorMsg() );
            Assert.assertTrue( trunLobThrd.isSuccess(),
                    trunLobThrd.getErrorMsg() );

        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
            }
        } catch ( BaseException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private class TruncateCLThread extends SdbThreadBase {

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncate();
            } finally {
                if ( null != db ) {
                    db.close();
                }
            }
        }
    }

    private class TruncateLobThread extends SdbThreadBase {
        private ObjectId oid = null;

        public TruncateLobThread( ObjectId oid ) {
            this.oid = oid;
        }

        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.truncateLob( oid, 0 );
            } catch ( BaseException e ) {
                int errCode = e.getErrorCode();
                if ( errCode != -4 && errCode != -321 ) {
                    // -4: file not exist
                    // -321: cl truncate
                    throw e;
                }
            } finally {
                if ( null != db ) {
                    db.close();
                }
            }
        }
    }

}
