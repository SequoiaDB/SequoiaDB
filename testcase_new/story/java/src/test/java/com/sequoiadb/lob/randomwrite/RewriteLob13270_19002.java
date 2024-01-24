package com.sequoiadb.lob.randomwrite;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.MaxKey;
import org.bson.types.MinKey;
import org.bson.types.ObjectId;
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
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.lob.utils.RandomWriteLobUtil;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @Description seqDB-13270:并发加锁写lob过程中执行切分; seqDB-19002:主子表并发加锁写lob过程中子表执行切分 *
 * @author wuyan
 * @Date 2017.11.8
 * @UpdateDate 2019.08.28
 * @version 1.00
 */
public class RewriteLob13270_19002 extends SdbTestBase {
    @DataProvider(name = "clNameProvider", parallel = true)
    public Object[][] generateCLName() {
        return new Object[][] {
                // the parameter is clName,splitCLName
                // testcase:13270
                new Object[] { clName, clName },
                // testcase:19002
                new Object[] { mainCLName, subCLName } };
    }

    private String clName = "writelob13270";
    private String mainCLName = "lobMainCL_19002";
    private String subCLName = "lobSubCL_19002";
    private static Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private byte[] testLobBuff = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        if ( CommLib.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }
        // create cl
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        String clOptions = "{ShardingKey:{no:1},ShardingType:'hash',Partition:1024,"
                + "ReplSize:0,Compressed:true}";
        RandomWriteLobUtil.createCL( cs, clName, clOptions );

        // create maincl and subcl , than attach subcl to maincl
        createMainCLAndAttachCL( cs, SdbTestBase.csName, mainCLName,
                subCLName );

        int writeSize = 1024 * 1024 * 2;
        testLobBuff = RandomWriteLobUtil.getRandomBytes( writeSize );
    }

    @Test(dataProvider = "clNameProvider")
    public void testLob( String clName, String splitCLName ) {
        try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" )) {
            String sourceRGName = RandomWriteLobUtil.getSrcGroupName( sdb,
                    SdbTestBase.csName, splitCLName );
            String targetRGName = RandomWriteLobUtil.getSplitGroupName( sdb,
                    sourceRGName );

            DBCollection dbcl = sdb.getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );
            ObjectId lobOid = RandomWriteLobUtil.createAndWriteLob( dbcl,
                    testLobBuff );

            int offset = 1024 * 1024;
            int rewriteLobSize = 1024 * 1024;
            List< LockAndRewriteLobTask > rewriteLobTasks = new ArrayList<>(
                    10 );
            byte[] rewriteBuff = RandomWriteLobUtil
                    .getRandomBytes( rewriteLobSize );
            for ( int i = 0; i < 20; i++ ) {
                rewriteLobTasks.add( new LockAndRewriteLobTask( clName, lobOid,
                        offset, rewriteBuff ) );
                offset = offset + rewriteLobSize + 10240;
            }

            for ( LockAndRewriteLobTask rewriteLobTask : rewriteLobTasks ) {
                rewriteLobTask.start();
            }
            SplitTask splitTask = new SplitTask( sdb, splitCLName, sourceRGName,
                    targetRGName );
            splitTask.start();

            for ( LockAndRewriteLobTask rewriteLobTask : rewriteLobTasks ) {
                rewriteLobTask.join();
            }
            splitTask.join();

            Assert.assertTrue( splitTask.isSuccess(), splitTask.getErrorMsg() );
            for ( LockAndRewriteLobTask rewriteLobTask : rewriteLobTasks ) {
                Assert.assertTrue( rewriteLobTask.isSuccess(),
                        rewriteLobTask.getErrorMsg() );
            }

            // check write result
            readLobAndcheckWriteResult( dbcl, lobOid, rewriteBuff );
            // check split result
            checkSplitResult( sdb, splitCLName, sourceRGName, targetRGName );
        }

    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            if ( cs.isCollectionExist( mainCLName ) ) {
                cs.dropCollection( mainCLName );
            }
            if ( cs.isCollectionExist( subCLName ) ) {
                cs.dropCollection( subCLName );
            }
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    public class SplitTask extends SdbThreadBase {
        private String clName;
        private String srcRGName;
        private String targetRGName;

        public SplitTask( Sequoiadb sdb, String clName, String srcRGName,
                String targetRGName ) {
            this.clName = clName;
            this.srcRGName = srcRGName;
            this.targetRGName = targetRGName;
        }

        @Override
        public void exec() throws BaseException {
            try ( Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl1 = db1.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                int persent = 80;
                cl1.split( srcRGName, targetRGName, persent );
            } catch ( BaseException e ) {
                Assert.assertTrue( false,
                        "split fail\n" + "srcGroup:" + srcRGName + "\ntarGroup"
                                + targetRGName + e.getMessage() );
            }
        }
    }

    private class LockAndRewriteLobTask extends SdbThreadBase {
        private String clName;
        private ObjectId lobOid;
        private int offset;
        private byte[] rewriteLobBuff;

        public LockAndRewriteLobTask( String clName, ObjectId lobOid,
                int offset, byte[] rewriteLobBuff ) {
            this.clName = clName;
            this.lobOid = lobOid;
            this.offset = offset;
            this.rewriteLobBuff = rewriteLobBuff;
        }

        @Override
        public void exec() throws Exception {
            DBLob lob = null;
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                lob = cl.openLob( lobOid, DBLob.SDB_LOB_WRITE );
                lob.lockAndSeek( offset, rewriteLobBuff.length );
                lob.write( rewriteLobBuff );
                lob.close();
                updateExpBuff( rewriteLobBuff, offset );
            }
        }
    }

    synchronized private void updateExpBuff( byte[] rewriteBuff, int offset ) {
        testLobBuff = RandomWriteLobUtil.appendBuff( testLobBuff, rewriteBuff,
                offset );
    }

    private void readLobAndcheckWriteResult( DBCollection cl, ObjectId lobOid,
            byte[] rewriteBuff ) {

        // check the rewrite lob
        int expOffset = 1024 * 1024;
        for ( int i = 0; i < 20; i++ ) {
            byte[] actBuff = RandomWriteLobUtil.seekAndReadLob( cl, lobOid,
                    rewriteBuff.length, expOffset );
            expOffset = expOffset + rewriteBuff.length + 10240;
            RandomWriteLobUtil.assertByteArrayEqual( actBuff, rewriteBuff );
        }

        // check write lob before split
        int lobsize = 1024 * 1024;
        byte[] expBuff = Arrays.copyOfRange( testLobBuff, 0, lobsize );
        byte[] rbuff = new byte[ lobsize ];
        try ( DBLob rlob = cl.openLob( lobOid )) {
            rlob.seek( 0, DBLob.SDB_LOB_SEEK_SET );
            rlob.read( rbuff );
        }
        RandomWriteLobUtil.assertByteArrayEqual( rbuff, expBuff );
    }

    private void createMainCLAndAttachCL( CollectionSpace cs, String csName,
            String mainCLName, String subCLName ) {
        if ( cs.isCollectionExist( mainCLName ) ) {
            cs.dropCollection( mainCLName );
        }
        BSONObject options = new BasicBSONObject();
        options.put( "IsMainCL", true );
        options.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        options.put( "ShardingType", "range" );
        options.put( "LobShardingKeyFormat", "YYYYMMDD" );
        DBCollection mainCL = cs.createCollection( mainCLName, options );

        BSONObject clOptions = new BasicBSONObject();
        clOptions.put( "ShardingKey", new BasicBSONObject( "date", 1 ) );
        clOptions.put( "ShardingType", "hash" );
        clOptions.put( "Partition", 1024 );
        cs.createCollection( subCLName, clOptions );

        BSONObject bound = new BasicBSONObject();
        bound.put( "LowBound", new BasicBSONObject( "date", new MinKey() ) );
        bound.put( "UpBound", new BasicBSONObject( "date", new MaxKey() ) );
        mainCL.attachCollection( csName + "." + subCLName, bound );
    }

    /**
     * construct expected result values
     * 
     * @return expected result values,rg:["group1","{"":0}","{"":500}"]
     */
    private List< CataInfoItem > buildExpectResult( String sourceRGName,
            String targetRGName ) {
        List< CataInfoItem > cataInfo = new ArrayList< CataInfoItem >();
        CataInfoItem item = new CataInfoItem();
        item.groupName = sourceRGName;
        item.lowBound = 0;
        item.upBound = 205;

        cataInfo.add( item );
        item = new CataInfoItem();
        item.groupName = targetRGName;
        item.lowBound = 205;
        item.upBound = 1024;
        cataInfo.add( item );
        return cataInfo;
    }

    private void checkSplitResult( Sequoiadb sdb, String clName,
            String sourceRGName, String targetRGName ) {
        String cond = String.format( "{Name:\"%s.%s\"}", SdbTestBase.csName,
                clName );
        DBCursor collections = sdb.getSnapshot( 8, cond, null, null );
        List< CataInfoItem > cataInfo = buildExpectResult( sourceRGName,
                targetRGName );
        while ( collections.hasNext() ) {
            BasicBSONObject doc = ( BasicBSONObject ) collections.getNext();
            doc.getString( "Name" );
            BasicBSONList subdoc = ( BasicBSONList ) doc.get( "CataInfo" );
            for ( int i = 0; i < cataInfo.size(); ++i ) {
                BasicBSONObject elem = ( BasicBSONObject ) subdoc.get( i );
                String groupName = elem.getString( "GroupName" );
                BasicBSONObject obj = ( BasicBSONObject ) elem
                        .get( "LowBound" );
                int LowBound;
                if ( obj.containsField( "" ) ) {
                    LowBound = obj.getInt( "" );
                } else {
                    LowBound = obj.getInt( "partition" );
                }

                int UpBound;
                obj = ( BasicBSONObject ) elem.get( "UpBound" );
                if ( obj.containsField( "" ) ) {
                    UpBound = obj.getInt( "" );
                } else {
                    UpBound = obj.getInt( "partition" );
                }

                boolean compareResult = cataInfo.get( i ).Compare( groupName,
                        LowBound, UpBound );
                Assert.assertTrue( compareResult,
                        cataInfo.get( i ).toString() + "actResult:"
                                + "groupName:" + groupName + " LowBound:"
                                + LowBound + " UpBound:" + UpBound );
            }
        }
    }

    private class CataInfoItem {
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
