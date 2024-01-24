package com.sequoiadb.lob.randomwrite;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-13263: 并发锁全部lob写入 seqDB-18995 主子表并发锁全部lob写入
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */

/*
 * 1、共享模式下，多个连接多线程并发如下操作: (1)打开已存在lob对象，锁定全部lob，向锁定的数据段写入数据 2、读取lob，检查操作结果
 */

public class RewriteLob13263_18995 extends SdbTestBase {

    private final String csName = "writelob13263";
    private final String clName = "writelob13263";
    private final String mainCLName = "mainCL18995";
    private final String subCLName = "subCL18995";
    private final int lobPageSize = 16 * 1024; // 16k
    private final int threadNum = 16;
    private AtomicInteger successTimes = new AtomicInteger( 0 );
    private byte[] expData = null;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is csName, clName
                // testcase:13263
                new Object[] { clName },
                // testcase:18995
                new Object[] { mainCLName } };
    }

    // TODO:1、setUp和test中的try-catch建议去掉
    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        cs = sdb.createCollectionSpace( csName, csOpt );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        cs.createCollection( clName, clOpt );
        if ( !CommLib.isStandAlone( sdb ) ) {
            LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                    subCLName );
        }
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
        int lobSize = 8 * 1024 * 1024;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );

        expData = data;
        List< LobPart > parts = new ArrayList<>( threadNum );
        final int partSize = lobSize / threadNum;
        for ( int i = 0; i < threadNum; ++i ) {
            parts.add( new LobPart( partSize * i, partSize ) );
        }

        List< WriteLobThread > thrdList = new ArrayList<>( threadNum );
        for ( int i = 0; i < threadNum; ++i ) {
            WriteLobThread wLobThrd = new WriteLobThread( clName, oid,
                    parts.get( i ) );
            wLobThrd.start();
            thrdList.add( wLobThrd );
        }
        for ( int i = 0; i < threadNum; ++i ) {
            Assert.assertTrue( thrdList.get( i ).isSuccess(),
                    thrdList.get( i ).getErrorMsg() );
        }

        Assert.assertNotEquals( successTimes.get(), 0, "nobody succeed" );
        byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );
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

    private class WriteLobThread extends SdbThreadBase {

        private String clNamet;
        private ObjectId oid = null;
        private LobPart part = null;

        public WriteLobThread( String clName, ObjectId oid, LobPart part ) {
            this.clNamet = clName;
            this.oid = oid;
            this.part = part;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( this.clNamet );
                try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
                    lob.lock( 0, lob.getSize() );
                    lob.seek( part.getOffset(), DBLob.SDB_LOB_SEEK_SET );
                    lob.write( part.getData() );
                    updateExpData( part );
                    successTimes.getAndIncrement();
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_LOB_LOCK_CONFLICTED
                            .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }

    private synchronized void updateExpData( LobPart part ) {
        expData = RandomWriteLobUtil.appendBuff( expData, part.getData(),
                part.getOffset() );
    }

}
