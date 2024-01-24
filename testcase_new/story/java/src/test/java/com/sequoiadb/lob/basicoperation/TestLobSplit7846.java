package com.sequoiadb.lob.basicoperation;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingDeque;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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

/**
 * @Description seqDB-7846: lob split
 * @author wuyan
 * @Date 2016.9.12
 * @update [2017.12.20]
 * @version 1.00
 */

public class TestLobSplit7846 extends SdbTestBase {
    private String clName = "cl_lob7846";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private Random random = new Random();
    private ConcurrentHashMap< ObjectId, String > id2md5 = new ConcurrentHashMap< ObjectId, String >();
    private LinkedBlockingDeque< ObjectId > oidQueue = new LinkedBlockingDeque< ObjectId >();
    private ArrayList< String > splitRGList = new ArrayList< String >( 2 );
    private String sourceRGName = "";
    private String targetRGName = "";
    private int lobNums = 100;

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
        BSONObject options = ( BSONObject ) JSON.parse(
                "{ShardingKey:{a:1},ShardingType:'hash',Partition:4096}" );
        cl = cs.createCollection( clName, options );
        writeLobAndGetMd5( lobNums );
    }

    @Test
    public void testSplitLob() throws InterruptedException {
        // test a:split by cond
        splitCLByCond();
        double expErrorValue = 0.5;
        splitRGList.add( sourceRGName );
        splitRGList.add( targetRGName );
        LobOprUtils.checkSplitResult( sdb, csName, clName, splitRGList,
                expErrorValue );

        // test b:split by percent,eg:split from targetRG to srcRG
        splitCLByPercent();
        // compare the total lobnums ,check the split range by cataInfo
        checkLobNums( lobNums );
        checkSplitResultByCataInfo();

        // testpoint a and b is contain the testpoint c ,check the lobdata after
        // split
        int lobNums = 100;
        readLobAndCheckMd5( lobNums );
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

    private void splitCLByCond() {
        try {
            BSONObject cond = new BasicBSONObject();
            BSONObject endCond = new BasicBSONObject();
            cond.put( "Partition", 2048 );
            endCond.put( "partition", 4096 );
            sourceRGName = LobOprUtils.getSrcGroupName( sdb, SdbTestBase.csName,
                    clName );
            targetRGName = LobOprUtils.getSplitGroupName( sourceRGName );
            cl.split( sourceRGName, targetRGName, cond, endCond );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "split fail:" + e.getMessage() + "srcRGName:" + sourceRGName
                            + "\n tarRGName:" + targetRGName );
        }
    }

    private void splitCLByPercent() {
        try {
            int percent = 80;
            cl.split( targetRGName, sourceRGName, percent );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "split fail:" + e.getMessage() + "srcRGName:" + sourceRGName
                            + "\n tarRGName:" + targetRGName );
        }
    }

    private void writeLobAndGetMd5( int lobtimes ) {
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = random.nextInt( 1024 * 1024 );
            byte[] wlobBuff = LobOprUtils.getRandomBytes( writeLobSize );
            ObjectId oid = LobOprUtils.createAndWriteLob( cl, wlobBuff );

            // save oid and md5
            String prevMd5 = LobOprUtils.getMd5( wlobBuff );
            oidQueue.offer( oid );
            id2md5.put( oid, prevMd5 );
        }
    }

    private void checkLobNums( int expLobNums ) {
        DBCursor listCursor = cl.listLobs();
        int count = 0;
        while ( listCursor.hasNext() ) {
            count++;
            listCursor.getNext();
        }
        listCursor.close();
        Assert.assertEquals( count, expLobNums );
    }

    private void readLobAndCheckMd5( int lobNums ) throws InterruptedException {
        for ( int i = 0; i < lobNums; i++ ) {
            ObjectId oid = oidQueue.take();
            try ( DBLob rLob = cl.openLob( oid, DBLob.SDB_LOB_READ )) {
                byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                rLob.read( rbuff );
                String curMd5 = LobOprUtils.getMd5( rbuff );
                String prevMd5 = id2md5.get( oid );
                Assert.assertEquals( curMd5, prevMd5, " the lob oid :"
                        + oid.toString() + " read data error!" );
                id2md5.remove( oid );
            }
        }
    }

    /**
     * construct expected result values
     * 
     * @return expected result values,rg:["group1","{"":0}","{"":500}"]
     */
    private List< CataInfoItem > buildExpectResult() {
        List< CataInfoItem > cataInfo = new ArrayList< CataInfoItem >();
        CataInfoItem item = new CataInfoItem();
        item.groupName = sourceRGName;
        item.lowBound = 0;
        item.upBound = 2048;

        cataInfo.add( item );
        item = new CataInfoItem();
        item.groupName = targetRGName;
        item.lowBound = 2048;
        item.upBound = 2458;
        cataInfo.add( item );
        item = new CataInfoItem();
        item.groupName = sourceRGName;
        item.lowBound = 2458;
        item.upBound = 4096;
        cataInfo.add( item );
        return cataInfo;
    }

    public void checkSplitResultByCataInfo() {
        String cond = String.format( "{Name:\"%s.%s\"}", SdbTestBase.csName,
                clName );
        DBCursor collections = sdb.getSnapshot( 8, cond, null, null );
        List< CataInfoItem > cataInfo = buildExpectResult();
        while ( collections.hasNext() ) {
            BasicBSONObject doc = ( BasicBSONObject ) collections.getNext();
            doc.getString( "Name" );
            BasicBSONList subdoc = ( BasicBSONList ) doc.get( "CataInfo" );
            for ( int i = 0; i < cataInfo.size(); ++i ) {
                BasicBSONObject elem = ( BasicBSONObject ) subdoc.get( i );
                String groupName = elem.getString( "GroupName" );
                BasicBSONObject obj = ( BasicBSONObject ) elem
                        .get( "LowBound" );
                int lowBound;
                if ( obj.containsField( "" ) ) {
                    lowBound = obj.getInt( "" );
                } else {
                    lowBound = obj.getInt( "partition" );
                }

                int upBound;
                obj = ( BasicBSONObject ) elem.get( "UpBound" );
                if ( obj.containsField( "" ) ) {
                    upBound = obj.getInt( "" );
                } else {
                    upBound = obj.getInt( "partition" );
                }

                boolean compareResult = cataInfo.get( i ).Compare( groupName,
                        lowBound, upBound );
                Assert.assertTrue( compareResult,
                        cataInfo.get( i ).toString() + "actResult:"
                                + "groupName:" + groupName + " LowBound:"
                                + lowBound + " UpBound:" + upBound );
            }
        }
    }

    public class CataInfoItem {
        public String groupName;
        public int lowBound;
        public int upBound;

        public boolean Compare( String name, int low, int up ) {
            return name.equals( groupName ) && low == lowBound && up == upBound;
        }

        @Override
        public String toString() {
            return "groupName : " + groupName + " lowBound: {'':" + lowBound
                    + "}" + " upBound:{'':" + upBound + "}";
        }
    }
}
