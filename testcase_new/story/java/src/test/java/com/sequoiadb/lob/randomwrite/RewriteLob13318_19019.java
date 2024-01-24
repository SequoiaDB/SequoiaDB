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
 * @FileName seqDB-13318: 并发读lob过程中执行切分 seqDB-19019 主子表并发读lob过程中执行切分
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.12
 * @Version 1.00
 */

/*
 * 1、多线程并发如下操作: (1)打开已存在lob对象，seek指定偏移范围，并发读取lob数据， 其中读取lob覆盖元数据组和目标组数据
 * （2）执行切分split （需要覆盖读取过程中元数据片迁移到目标组） 2、读取lob，检查操作结果
 */

public class RewriteLob13318_19019 extends SdbTestBase {

    private final String csName = "writelob13318";
    private final String clName = "writelob13318";
    private final String mainCLName = "mainCL19019";
    private final String subCLName = "subCL19019";
    private final int lobPageSize = 32 * 1024; // 32k
    private final int threadNum = 8;
    private final int writeSizePerThread = 8 * lobPageSize;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is csName, mainCLName, subCLName
                // testcase:13318
                new Object[] { clName, clName },
                // testcase:19019
                new Object[] { mainCLName, subCLName } };
    }

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) || CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "can not support split" );
        }

        // create cs cl
        BSONObject csOpt = ( BSONObject ) JSON
                .parse( "{LobPageSize: " + lobPageSize + "}" );
        cs = sdb.createCollectionSpace( csName, csOpt );
        BSONObject clOpt = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        cs.createCollection( clName, clOpt );
        LobSubUtils.createMainCLAndAttachCL( sdb, csName, mainCLName,
                subCLName );
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String mainCLName, String subCLName ) {
        int lobSize = 1 * 1024 * 1024;
        byte[] data = RandomWriteLobUtil.getRandomBytes( lobSize );
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( mainCLName );
        ObjectId oid = RandomWriteLobUtil.createAndWriteLob( cl, data );
        List< LobPart > parts = getLobParts( threadNum, writeSizePerThread );

        try ( DBLob lob = cl.openLob( oid, DBLob.SDB_LOB_WRITE )) {
            for ( LobPart part : parts ) {
                lockAndSeekAndWrite( lob, part );
            }
        }

        // init
        SplitThread splitThrd = new SplitThread( csName, subCLName );
        List< ReadAndCheckLobThread > rLobThrds = new ArrayList< ReadAndCheckLobThread >();
        for ( int i = 0; i < threadNum; ++i ) {
            ReadAndCheckLobThread rLobThrd = new ReadAndCheckLobThread( csName,
                    mainCLName, oid, parts.get( i ) );
            rLobThrds.add( rLobThrd );
        }

        // start
        splitThrd.start();
        for ( ReadAndCheckLobThread rLobThrd : rLobThrds ) {
            rLobThrd.start();
        }

        // join
        Assert.assertTrue( splitThrd.isSuccess(), splitThrd.getErrorMsg() );
        for ( ReadAndCheckLobThread rLobThrd : rLobThrds ) {
            Assert.assertTrue( rLobThrd.isSuccess(), rLobThrd.getErrorMsg() );
        }
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

    private void lockAndSeekAndWrite( DBLob lob, LobPart part ) {
        lob.lockAndSeek( part.getOffset(), part.getLength() );
        lob.write( part.getData() );
    }

    private class SplitThread extends SdbThreadBase {

        private Sequoiadb db = null;
        private String csNamet;
        private String clNamet;
        private String srcGroupName = null;
        private String dstGroupName = null;

        public SplitThread( String csName, String clName ) {
            this.csNamet = csName;
            this.clNamet = clName;
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            srcGroupName = RandomWriteLobUtil.getSrcGroupName( db, this.csNamet,
                    this.clNamet );
            dstGroupName = RandomWriteLobUtil.getSplitGroupName( db,
                    srcGroupName );
        }

        @Override
        public void exec() throws Exception {
            try {
                DBCollection cl = db.getCollectionSpace( this.csNamet )
                        .getCollection( this.clNamet );
                cl.split( srcGroupName, dstGroupName, 50 );
            } finally {
                if ( null != db ) {
                    db.close();
                }
            }
        }
    }

    private class ReadAndCheckLobThread extends SdbThreadBase {

        private String csNamet;
        private String clNamet;
        private ObjectId oid = null;
        private byte[] readData = null;
        private LobPart part = null;

        public ReadAndCheckLobThread( String csName, String clName,
                ObjectId oid, LobPart part ) {
            this.csNamet = csName;
            this.clNamet = clName;
            this.oid = oid;
            this.part = part;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( this.csNamet )
                        .getCollection( this.clNamet );
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

    private List< LobPart > getLobParts( int partNum, int partSize ) {
        List< LobPart > parts = new ArrayList< LobPart >();
        for ( int i = 0; i < partNum; ++i ) {
            LobPart part = new LobPart( i * partSize, partSize );
            parts.add( part );
        }
        return parts;
    }

}
