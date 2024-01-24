package com.sequoiadb.lob.basicoperation;

import java.util.ArrayList;
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
 * @Description seqDB-7848: cl1 remove lob and split ,when cl1 writelob.
 * @author wuyan
 * @Date 2016.10.9
 * @update [2017.12.20]
 * @version 1.00
 */
public class TestLobSplitAndWrite7848 extends SdbTestBase {
    private String clName1 = "cl_lob7848a";
    private String clName2 = "cl_lob7848b";
    private String csName = "cs7848";
    private static Sequoiadb sdb = null;
    private Random random = new Random();
    private String sourceRGName = "";
    private String targetRGName = "";
    private byte[] wlobBuff = null;
    private String prevMd5 = "";
    private LinkedBlockingDeque< ObjectId > oidQueue1 = new LinkedBlockingDeque< ObjectId >();
    private LinkedBlockingDeque< ObjectId > oidQueue2 = new LinkedBlockingDeque< ObjectId >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }

        createCSAndCL();

        // cl1 write lob
        DBCollection dbcl1 = sdb.getCollectionSpace( csName )
                .getCollection( clName1 );
        int lobtimes = 100;
        int writeLobSize = random.nextInt( 1024 * 1024 );
        wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
        prevMd5 = LobOprUtils.getMd5( wlobBuff );
        writeLob( dbcl1, lobtimes, oidQueue1 );
    }

    @Test
    public void testSplitAndWrite() {
        // cl2(cl_lob7848b) writeLob operation,write lob nums :30 *4 =120
        PutLobsTask putLobTasks = new PutLobsTask();
        putLobTasks.start( 30 );

        // cl1(cl_lob7848a) removeLob and split operation:remove lob nums is 80
        RemoveLobsTask removeLobsTasks = new RemoveLobsTask();
        removeLobsTasks.start( 80 );

        SplitCL splitCL = new SplitCL();
        splitCL.start();

        if ( !splitCL.isSuccess() ) {
            Assert.fail( splitCL.getErrorMsg() );
        }
        Assert.assertTrue( putLobTasks.isSuccess(), putLobTasks.getErrorMsg() );
        Assert.assertTrue( putLobTasks.isSuccess(), putLobTasks.getErrorMsg() );
        Assert.assertTrue( removeLobsTasks.isSuccess(),
                removeLobsTasks.getErrorMsg() );

        // cl1 check the split result
        ArrayList< String > splitRGNames = new ArrayList< String >( 2 );
        splitRGNames.add( sourceRGName );
        splitRGNames.add( targetRGName );
        checkSplitResult( sdb, csName, clName1, splitRGNames );
        // check the lob data
        checkLobofCL( clName1, oidQueue1 );
        checkLobofCL( clName2, oidQueue2 );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( sdb.isCollectionSpaceExist( csName ) ) {
                sdb.dropCollectionSpace( csName );
                ;
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
            sourceRGName = LobOprUtils.getSrcGroupName( sdb, csName, clName1 );
            targetRGName = LobOprUtils.getSplitGroupName( sourceRGName );
            try ( Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                DBCollection cl1 = db1.getCollectionSpace( csName )
                        .getCollection( clName1 );
                int percent = 50;
                cl1.split( sourceRGName, targetRGName, percent );
            }
        }
    }

    private class RemoveLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException, InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName1 );
                ObjectId oid = oidQueue1.take();
                dbcl.removeLob( oid );
            }
        }
    }

    private class PutLobsTask extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName2 );
                int lobtimes = 4;
                writeLob( dbcl, lobtimes, oidQueue2 );
            }
        }
    }

    private void checkLobofCL( String clName,
            LinkedBlockingDeque< ObjectId > oidQueue ) {
        int count = 0;
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        try ( DBCursor listLob = dbcl.listLobs()) {
            while ( listLob.hasNext() ) {
                BasicBSONObject obj = ( BasicBSONObject ) listLob.getNext();
                ObjectId existOid = obj.getObjectId( "Oid" );
                Assert.assertEquals( oidQueue.contains( existOid ), true,
                        existOid.toString() + " of " + clName
                                + " is not found in oidQueue!" );

                try ( DBLob rLob = dbcl.openLob( existOid )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    String curMd5 = LobOprUtils.getMd5( rbuff );
                    Assert.assertEquals( curMd5, prevMd5, "the lob:"
                            + existOid.toString() + " datas error!" );
                }
                count++;
            }
        }

        // the list lobnums must be consistent with the number of remaining
        // digits in the actual
        // oidqueue
        Assert.assertEquals( count, oidQueue.size() );
    }

    private void writeLob( DBCollection cl, int lobtimes,
            LinkedBlockingDeque< ObjectId > oidQueue ) {
        for ( int i = 0; i < lobtimes; i++ ) {
            ObjectId oid = LobOprUtils.createAndWriteLob( cl, wlobBuff );
            // save oid
            oidQueue.offer( oid );
        }
    }

    private void createCSAndCL() {
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        CollectionSpace cSpace = sdb.createCollectionSpace( csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:4096,"
                + "ReplSize:0,Compressed:true}";
        BSONObject options = ( BSONObject ) JSON.parse( clOptions );
        cSpace.createCollection( clName1, options );
        cSpace.createCollection( clName2, options );
    }

    private void checkSplitResult( Sequoiadb sdb, String csName, String clName,
            ArrayList< String > splitGroupNames ) {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        DBCursor listCursor = cl.listLobs();
        int count = 0;
        while ( listCursor.hasNext() ) {
            count++;
            listCursor.getNext();
        }
        listCursor.close();

        int actListNums = 0;
        for ( int i = 0; i < splitGroupNames.size(); i++ ) {
            String nodeName = sdb.getReplicaGroup( splitGroupNames.get( i ) )
                    .getMaster().getNodeName();

            try ( Sequoiadb dataDB = new Sequoiadb( nodeName, "", "" )) {
                DBCollection dataCL = dataDB.getCollectionSpace( csName )
                        .getCollection( clName );
                DBCursor listLobs = dataCL.listLobs();
                int subCount = 0;
                while ( listLobs.hasNext() ) {
                    subCount++;
                    listLobs.getNext();
                }
                listLobs.close();
                actListNums += subCount;
            }
        }
        // sum of query results on each group is equal to the results of coord
        Assert.assertEquals( actListNums, count,
                "list lobs error." + "allCount:" + actListNums );
    }
}
