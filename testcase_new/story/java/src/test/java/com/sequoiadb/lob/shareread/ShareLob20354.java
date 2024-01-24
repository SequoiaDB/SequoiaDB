package com.sequoiadb.lob.shareread;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-20354 seqDB-20354:SHARED_READ|WRITE写lob过程中执行切分
 * @author luweikang
 * @Date 2019.8.26
 */

public class ShareLob20354 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "cl20354";
    private DBCollection cl = null;
    private String srcGroup;
    private String tarGroup;
    private int lobSize = 1024 * 1024;
    private int writeSize = 1024 * 700;
    private int lobNum = 30;
    byte[] lobBuff;
    private List< ObjectId > ids = new ArrayList< >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip standalone mode." );
        }
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        srcGroup = groupNames.get( 0 );
        tarGroup = groupNames.get( 1 );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON
                        .parse( "{ShardingKey:{\"_id\":1}, ShardingType:\"hash\", Group:\""
                                + srcGroup + "\"}" ) );
        lobBuff = RandomWriteLobUtil.getRandomBytes( lobSize );
        for ( int i = 0; i < lobNum; i++ ) {
            ObjectId id = RandomWriteLobUtil.createAndWriteLob( cl, lobBuff );
            ids.add( id );
        }
    }

    @Test
    public void testLob() {
        WriteThread write = new WriteThread();
        SplitThread split = new SplitThread();

        write.start();
        split.start();

        Assert.assertTrue( write.isSuccess(), write.getErrorMsg() );
        Assert.assertTrue( split.isSuccess(), split.getErrorMsg() );

        for ( ObjectId id : ids ) {
            RandomWriteLobUtil.checkShareLobResult( cl, id, lobSize, lobBuff );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class WriteThread extends SdbThreadBase {

        @Override
        public void exec() throws BaseException {
            byte[] writeBuff = LobOprUtils.getRandomBytes( writeSize );
            try ( Sequoiadb db = CommLib.getRandomSequoiadb()) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < lobNum; i++ ) {
                    DBLob lob = cl.openLob( ids.get( i ),
                            DBLob.SDB_LOB_SHAREREAD | DBLob.SDB_LOB_WRITE );
                    lob.lockAndSeek( 1024 * 100, writeSize );
                    lob.write( writeBuff );
                    lob.close();
                }
                lobBuff = RandomWriteLobUtil.appendBuff( lobBuff, writeBuff,
                        1024 * 100 );
            }
        }
    }

    private class SplitThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = CommLib.getRandomSequoiadb()) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                cl.split( srcGroup, tarGroup, 50 );
            }
        }
    }
}
