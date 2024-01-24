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
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName seqDB-18997 主子表并发加锁写lob，其中写数据范围不连续 seqDB-19014 主子表并发读lob，其中读数据范围不连续
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019-09-12
 * @Version 1.00
 */

/*
 * seqDB-18997 1、共享模式下，多个连接多线程并发如下操作: (1)打开已存在lob对象，seek指定偏移范围，执行lock锁定数据段，
 * 向锁定数据段写入lob 多个并发线程中锁定数据段为连续范围， 如线程1锁定数据范围为1-3、线程2锁定数据范围为4-5、线程3锁定数据范围为5-9
 * 2、读取lob，检查操作结果
 */

/*
 * seqDB-19014 1、多个连接多线程并发如下操作: (1)打开已存在lob对象，seek指定偏移范围，执行read读取指定范围内数据 \
 * 多个并发线程中指定数据范围不连续，如线程1读数据范围为1-3、线程2读数据范围 为6-15、线程3锁定数据范围为25-29 2、检查操作结果
 */

public class RewriteLob18997_19014 extends SdbTestBase {
    private boolean runSuccess = false;
    private final String csName = "csName18997";
    private final String mainCLName = "mainCL_18997";
    private final String subCLName = "subCL_18997";
    private final int lobPageSize = 32 * 1024; // 32k
    private final int threadNum = 8;
    private final int writeSizePerThread = 8 * lobPageSize;

    private Sequoiadb sdb = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        sdb.createCollectionSpace( csName, csOpt );
        cl = LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );
    }

    @Test
    public void testLob() {
        int lobSize = 1 * 1024 * 1024;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );
        List< LobPart > parts = getDiscontinuousParts( threadNum,
                writeSizePerThread );

        // write concurrently
        List< WriteLobThread > wLobThrds = new ArrayList< WriteLobThread >();
        for ( int i = 0; i < threadNum; ++i ) {
            WriteLobThread wLobThrd = new WriteLobThread( oid, parts.get( i ) );
            wLobThrds.add( wLobThrd );
        }
        for ( WriteLobThread wLobThrd : wLobThrds ) {
            wLobThrd.start();
        }
        for ( WriteLobThread wLobThrd : wLobThrds ) {
            Assert.assertTrue( wLobThrd.isSuccess(), wLobThrd.getErrorMsg() );
        }

        // read and check concurrently
        List< ReadAndCheckLobThread > rLobThrds = new ArrayList< ReadAndCheckLobThread >();
        for ( int i = 0; i < threadNum; ++i ) {
            ReadAndCheckLobThread rLobThrd = new ReadAndCheckLobThread( oid,
                    parts.get( i ) );
            rLobThrds.add( rLobThrd );
        }
        for ( ReadAndCheckLobThread rLobThrd : rLobThrds ) {
            rLobThrd.start();
        }
        for ( ReadAndCheckLobThread rLobThrd : rLobThrds ) {
            Assert.assertTrue( rLobThrd.isSuccess(), rLobThrd.getErrorMsg() );
        }

        runSuccess = true;
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( runSuccess ) {
                sdb.dropCollectionSpace( csName );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    private class WriteLobThread extends SdbThreadBase {
        private ObjectId oid = null;
        private LobPart part = null;

        public WriteLobThread( ObjectId oid, LobPart part ) {
            this.oid = oid;
            this.part = part;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
                    lob.lockAndSeek( part.getOffset(), part.getLength() );
                    lob.write( part.getData() );
                }
            }
        }
    }

    private class ReadAndCheckLobThread extends SdbThreadBase {
        private ObjectId oid = null;
        private byte[] readData = null;
        private LobPart part = null;

        public ReadAndCheckLobThread( ObjectId oid, LobPart part ) {
            this.oid = oid;
            this.part = part;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( mainCLName );
                try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
                    lob.lockAndSeek( part.getOffset(), part.getLength() );
                    readData = new byte[ part.getLength() ];
                    lob.read( readData );
                    RandomWriteLobUtil.assertByteArrayEqual( readData,
                            part.getData() );
                }
            }
        }
    }

    private List< LobPart > getDiscontinuousParts( int partNum, int partSize ) {
        List< LobPart > parts = new ArrayList< LobPart >();
        for ( int i = 0; i < partNum; ++i ) {
            LobPart part = new LobPart( 2 * i * partSize, partSize );
            parts.add( part );
        }
        return parts;
    }

}
