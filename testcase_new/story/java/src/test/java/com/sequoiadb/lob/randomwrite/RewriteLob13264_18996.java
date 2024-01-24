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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.lob.utils.LobSubUtils;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-13264:并发加锁写lob，其中写数据范围连续
 *              *seqDB-18996:主子表并发加锁写lob，其中写数据范围连续
 * @Author linsuqiang
 * @Date 2017-11-08
 * @UpdateAuthor luweikang
 * @UpdateDate 2019.09.11
 * @Version 1.00
 */
public class RewriteLob13264_18996 extends SdbTestBase {

    private final String csName = "writelob13264";
    private final String clName = "writelob13264";
    private final String mainCLName = "mainCL18996";
    private final String subCLName = "subCL18996";
    private final int lobPageSize = 32 * 1024; // 32k
    private final int threadNum = 16;
    private final int writeSizePerThread = 2 * lobPageSize;

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;

    @DataProvider(name = "clNameProvider", parallel = false)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clName
                // testcase:13263
                new Object[] { clName },
                // testcase:18996
                new Object[] { mainCLName } };
    }

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

    // start <threadNum> threads, and every thread put
    // a part of lob, which size is <writeSizePerThread>
    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName ) {
        if ( CommLib.isStandAlone( sdb ) && clName.equals( mainCLName ) ) {
            throw new SkipException( "is standalone skip testcase!" );
        }
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

        // check lob data and size
        byte[] actData = RandomWriteLobUtil.readLob( cl, oid );
        RandomWriteLobUtil.assertByteArrayEqual( actData, expData,
                "lob data is wrong" );
        int expSize = threadNum * writeSizePerThread;
        int actSize = getLobSizeByList( cl, oid );
        Assert.assertEquals( actSize, expSize, "lob size is wrong" );

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

    private int getLobSizeByList( DBCollection cl, ObjectId oid ) {
        DBCursor cursor = cl.listLobs();
        boolean oidFound = false;
        int lobSize = 0;
        while ( cursor.hasNext() ) {
            BSONObject currRec = cursor.getNext();
            ObjectId currOid = ( ObjectId ) currRec.get( "Oid" );
            if ( currOid.equals( oid ) ) {
                lobSize = ( int ) ( ( long ) currRec.get( "Size" ) );
                oidFound = true;
                break;
            }
        }
        cursor.close();
        if ( !oidFound ) {
            throw new RuntimeException( "oid not found" );
        }
        return lobSize;
    }

}
