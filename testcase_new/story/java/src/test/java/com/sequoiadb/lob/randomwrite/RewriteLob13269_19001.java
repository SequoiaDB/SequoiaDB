package com.sequoiadb.lob.randomwrite;

import java.util.ArrayList;
import java.util.List;

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
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-13269: 切分表并发加锁写lob
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */

/*
 * 1、共享模式下，多个连接多线程并发如下操作: (1)打开已存在lob对象，seek指定偏移范围，执行lock锁定数据段，向锁定数据段写入lob；
 * 多个并发线程中锁定数据段范围不冲突，锁定数据范围覆盖目标组和源组 2、读取lob，检查操作结果
 */

public class RewriteLob13269_19001 extends SdbTestBase {

    private final String csName = "writelob13269";
    private final String clName = "writelob13269";
    private String mainCLName = "mainCL_19001";
    private String subCLName = "subCL_19001";
    private final int lobPageSize = 4 * 1024; // 32k
    private final int threadNum = 16;
    private final int writeSizePerThread = 32 * lobPageSize;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is subCLName
                // testcase:13269
                new Object[] { clName },
                // testcase:19001
                new Object[] { mainCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "no groups to split" );
        }

        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        cs = sdb.createCollectionSpace( csName, csOpt );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        DBCollection cl = cs.createCollection( clName, clOpt );

        // split cl
        String srcGroupName = RandomWriteLobUtil.getSrcGroupName( sdb, csName,
                clName );
        String dstGroupName = RandomWriteLobUtil.getSplitGroupName( sdb,
                srcGroupName );
        cl.split( srcGroupName, dstGroupName, 50 );

        LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );
    }

    // start <threadNum> threads, and every thread put
    // a part of lob, which size is <writeSizePerThread>
    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        int lobSize = 512 * 1024;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );
        List< LobPart > parts = getContinuousParts( threadNum,
                writeSizePerThread );
        List< WriteLobThread > wLobThrds = new ArrayList< WriteLobThread >();

        // initialize threads and expData
        byte[] expData = data;
        for ( int i = 0; i < threadNum; ++i ) {
            WriteLobThread wLobThrd = new WriteLobThread( clName, oid,
                    parts.get( i ) );
            wLobThrds.add( wLobThrd );
            expData = updateExpData( expData, parts.get( i ) );
        }

        // write concurrently
        for ( WriteLobThread wLobThrd : wLobThrds ) {
            wLobThrd.start();
        }
        for ( WriteLobThread wLobThrd : wLobThrds ) {
            Assert.assertTrue( wLobThrd.isSuccess(), wLobThrd.getErrorMsg() );
        }

        // check lob data
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
            Sequoiadb db = null;
            DBLob lob = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( this.clNamet );
                lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE );

                lob.lockAndSeek( part.getOffset(), part.getLength() );
                lob.write( part.getData() );
            } finally {
                if ( null != lob ) {
                    lob.close();
                }
                if ( null != db ) {
                    db.close();
                }
            }
        }
    }

    private List< LobPart > getContinuousParts( int partNum, int partSize ) {
        List< LobPart > parts = new ArrayList< LobPart >();
        for ( int i = 0; i < partNum; ++i ) {
            LobPart part = new LobPart( i * partSize, partSize );
            parts.add( part );
        }
        return parts;
    }

    private byte[] updateExpData( byte[] expData, LobPart part ) {
        return RandomWriteLobUtil.appendBuff( expData, part.getData(),
                part.getOffset() );
    }

}
