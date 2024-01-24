package com.sequoiadb.lob.basicoperation;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Random;
import java.util.concurrent.LinkedBlockingDeque;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.LobOprUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-7847: when spliting ,write/read/remove lob
 * @author wuyan
 * @Date 2016.10.9
 * @version 1.00
 */
public class TestLobSplitAndWrite7847 extends SdbTestBase {
    private String clName = "cl_lob7847";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private Random random = new Random();
    private String sourceRGName = "";
    private String targetRGName = "";
    private LinkedBlockingDeque< LobInfo > lobInfoQue = new LinkedBlockingDeque< LobInfo >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:4096,ReplSize:0}";
        dbcl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( clOptions ) );
        int lobtimes = 100;
        writeLobAndGetMd5( dbcl, lobtimes );
    }

    @Test
    public void testSplitAndWrite() {
        PutLobsTask putLobTask = new PutLobsTask();
        RemoveLobsTask removeLobTask = new RemoveLobsTask();
        putLobTask.start( 30 );
        removeLobTask.start( 110 );

        SplitCL splitCLTask = new SplitCL();
        splitCLTask.start();

        Assert.assertTrue( putLobTask.isSuccess(), putLobTask.getErrorMsg() );
        Assert.assertTrue( removeLobTask.isSuccess(),
                removeLobTask.getErrorMsg() );
        Assert.assertTrue( splitCLTask.isSuccess(), splitCLTask.getErrorMsg() );

        // check the split result
        double expErrorValue = 0.5;
        ArrayList< String > splitRGNames = new ArrayList< String >( 2 );
        splitRGNames.add( sourceRGName );
        splitRGNames.add( targetRGName );
        LobOprUtils.checkSplitResult( sdb, csName, clName, splitRGNames,
                expErrorValue );
        // check the lob data
        checkLobData();
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
        } finally {
            if ( null != sdb ) {
                sdb.close();
            }
        }
    }

    public class SplitCL extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            sourceRGName = LobOprUtils.getSrcGroupName( sdb, SdbTestBase.csName,
                    clName );
            targetRGName = LobOprUtils.getSplitGroupName( sourceRGName );
            try ( Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                BSONObject cond = new BasicBSONObject();
                BSONObject endCond = new BasicBSONObject();
                cond.put( "Partition", 512 );
                endCond.put( "partition", 2560 );
                cl1.split( sourceRGName, targetRGName, cond, endCond );
            }
        }
    }

    private class RemoveLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException, InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                LobInfo lobInfo = lobInfoQue.take();
                ObjectId oid = lobInfo.oid;
                dbcl.removeLob( oid );
            }
        }
    }

    private void checkLobData() {
        try ( DBCursor listLob = dbcl.listLobs()) {
            while ( listLob.hasNext() ) {
                BasicBSONObject obj = ( BasicBSONObject ) listLob.getNext();
                ObjectId existOid = obj.getObjectId( "Oid" );
                try ( DBLob rLob = dbcl.openLob( existOid )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    String curMd5 = LobOprUtils.getMd5( rbuff );
                    String prevMd5 = getLobMd5ByOid( existOid );
                    Assert.assertEquals( curMd5, prevMd5,
                            "the list oid:" + existOid.toString() );
                }
            }
        }
        // the list lobnums must be consistent with the number of remaining
        // digits in the actual
        // map:id2md5
        Assert.assertEquals( lobInfoQue.isEmpty(), true, "the remaining "
                + lobInfoQue.size() + " oids were not found!" );
    }

    // find the md5 from expected queue
    private String getLobMd5ByOid( ObjectId lobOid ) {
        Iterator< LobInfo > iterator = lobInfoQue.iterator();
        boolean found = false;
        String findMd5 = "";
        while ( iterator.hasNext() ) {
            LobInfo current = iterator.next();
            ObjectId oid = current.getOid();
            if ( oid.equals( lobOid ) ) {
                findMd5 = current.getMd5();
                lobInfoQue.remove( current );
                found = true;
                break;
            }
        }

        // if oid does not exist in the queue,than error
        if ( !found ) {
            throw new RuntimeException( "oid[" + lobOid + "] not found" );
        }
        return findMd5;
    }

    private class PutLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int lobtimes = 3;
                writeLobAndGetMd5( dbcl, lobtimes );
            }
        }
    }

    private void writeLobAndGetMd5( DBCollection cl, int lobtimes ) {
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = random.nextInt( 1024 * 1024 );
            ;
            byte[] wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
            ObjectId oid = LobOprUtils.createAndWriteLob( cl, wlobBuff );

            // save oid and md5
            String prevMd5 = LobOprUtils.getMd5( wlobBuff );
            lobInfoQue.offer( new LobInfo( oid, prevMd5 ) );
        }
    }

    private class LobInfo {
        private ObjectId oid;
        private String md5;

        public LobInfo( ObjectId oid, String md5 ) {
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
}
