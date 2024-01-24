package com.sequoiadb.lob.basicoperation;

import java.util.ArrayList;
import java.util.concurrent.LinkedBlockingQueue;

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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-7849: read lob ,when cl split
 * @author wuyan
 * @Date 2016.10.9
 * @update [2017.12.20]
 * @version 1.00
 */
public class TestLobSplitAndRead7849 extends SdbTestBase {
    private String csName = "cs_lob7849";
    private String clName = "cl_lob7849";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private String sourceRGName = "";
    private String targetRGName = "";
    private LinkedBlockingQueue< SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue<>();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cs = sdb.createCollectionSpace( csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:4096,"
                + "ReplSize:0,Compressed:true}";
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( clOptions ) );

        // write lob
        int lobtimes = 100;
        writeLobAndGetMd5( cl, lobtimes );
    }

    @Test
    public void testSplitAndWrite() {
        ReadLobsTask readLobsTask = new ReadLobsTask();
        readLobsTask.start( 100 );
        SplitCL splitCL = new SplitCL();
        splitCL.start();
        Assert.assertTrue( readLobsTask.isSuccess(),
                readLobsTask.getErrorMsg() );
        Assert.assertTrue( splitCL.isSuccess(), splitCL.getErrorMsg() );

        // check the split result
        double expErrorValue = 0.5;
        ArrayList< String > splitRGNames = new ArrayList<>( 2 );
        splitRGNames.add( sourceRGName );
        splitRGNames.add( targetRGName );
        LobOprUtils.checkSplitResult( sdb, csName, clName, splitRGNames,
                expErrorValue );

    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    public class SplitCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            sourceRGName = LobOprUtils.getSrcGroupName( sdb, csName, clName );
            targetRGName = LobOprUtils.getSplitGroupName( sourceRGName );
            try ( Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                DBCollection cl1 = db1.getCollectionSpace( csName )
                        .getCollection( clName );
                int percent = 50;
                cl1.split( sourceRGName, targetRGName, percent );
            }
        }
    }

    private class ReadLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException, InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.setSessionAttr( ( BSONObject ) JSON
                        .parse( "{'PreferedInstance':'M'}" ) );
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                SaveOidAndMd5 oidAndMd5 = id2md5.take();
                ObjectId oid = oidAndMd5.getOid();

                for ( int i = 0; i < 10; i++ ) {
                    try ( DBLob rLob = dbcl.openLob( oid,
                            DBLob.SDB_LOB_READ )) {
                        byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                        rLob.read( rbuff );
                        String curMd5 = LobOprUtils.getMd5( rbuff );
                        String prevMd5 = oidAndMd5.getMd5();
                        Assert.assertEquals( curMd5, prevMd5 );
                    } catch ( BaseException e ) {
                        e.printStackTrace();
                        if ( e.getErrorCode() != -4 ) {
                            throw e;
                        }
                    }
                }
            }
        }
    }

    private class SaveOidAndMd5 {
        private ObjectId oid;
        private String md5;

        public SaveOidAndMd5( ObjectId oid, String md5 ) {
            this.oid = oid;
            this.md5 = md5;
        }

        public ObjectId getOid() {
            return oid;
        }

        public String getMd5() {
            return md5;
        }
    }

    private void writeLobAndGetMd5( DBCollection cl, int lobtimes ) {
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = 1024 * 1024;
            byte[] wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
            ObjectId oid = LobOprUtils.createAndWriteLob( cl, wlobBuff );

            // save oid and md5
            String prevMd5 = LobOprUtils.getMd5( wlobBuff );
            id2md5.offer( new SaveOidAndMd5( oid, prevMd5 ) );
        }
    }

}
