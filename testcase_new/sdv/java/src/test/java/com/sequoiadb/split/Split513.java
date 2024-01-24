package com.sequoiadb.split;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:SeqDB-513 seqDB-513:数据切分过程中插入大量数据，包括普通记录数据和lob数据
 * @Author huangqiaohui
 * @Modify huangxiaoni 2021/5/11
 */

public class Split513 extends SdbTestBase {
    private Sequoiadb commSdb = null;
    private String srcGroupName;
    private String dstGroupName;
    private CollectionSpace commCS;
    private String clName = "split513";
    private DBCollection commCL;
    private long recsNum1 = 1000;
    private long recsNum2 = 1000;
    private long lobNum = 1000;
    private boolean runSuccess = false;

    @BeforeClass
    private void setUp() {
        try {
            commSdb = new Sequoiadb( coordUrl, "", "" );
            // 跳过独立模式和小于2个组的模式
            if ( CommLib.isStandAlone( commSdb ) ) {
                throw new SkipException( "skip standalone" );
            }
            ArrayList< String > groupNames = CommLib
                    .getDataGroupNames( commSdb );

            if ( groupNames.size() < 2 ) {
                throw new SkipException( "skip less than tow groups" );
            }

            // 创建分区表
            srcGroupName = groupNames.get( 0 );
            dstGroupName = groupNames.get( 1 );
            commCS = commSdb.getCollectionSpace( SdbTestBase.csName );
            commCL = commCS.createCollection( clName,
                    new BasicBSONObject( "ShardingType", "hash" )
                            .append( "ShardingKey",
                                    new BasicBSONObject( "a", 1 ) )
                            .append( "Group", srcGroupName ) );

            // 插入数据
            ArrayList< BSONObject > insertor = new ArrayList<>();
            for ( long i = 0; i < recsNum1; i++ ) {
                insertor.add( new BasicBSONObject( "a", i ) );
            }
            commCL.bulkInsert( insertor, SplitUtils.FLG_INSERT_CONTONDUP );
        } catch ( BaseException e ) {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    @Test
    private void test() throws InterruptedException {
        // 切分过程中插入数据并写大对象
        SplitThread splitThread = new SplitThread();
        splitThread.start();

        Thread.sleep( new Random().nextInt( 1000 ) );

        InsertThread insertThread = new InsertThread();
        insertThread.start();

        PutLobThread pubLobThread = new PutLobThread();
        pubLobThread.start();

        if ( !splitThread.isSuccess() ) {
            Assert.fail( splitThread.getErrorMsg() );
        }
        if ( !insertThread.isSuccess() ) {
            Assert.fail( insertThread.getErrorMsg() );
        }
        if ( !pubLobThread.isSuccess() ) {
            Assert.fail( pubLobThread.getErrorMsg() );
        }

        // 检查编目切分范围以及切分后的数据正确性
        checkCatalog();
        checkDataCount();

        runSuccess = true;
    }

    @AfterClass
    private void tearDown() {
        try {
            if ( runSuccess )
                commCS.dropCollection( clName );
        } finally {
            if ( commSdb != null ) {
                commSdb.disconnect();
            }
        }
    }

    private class SplitThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                cl.split( srcGroupName, dstGroupName, 50 );
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }
    }

    private class InsertThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                ArrayList< BSONObject > insertor = new ArrayList<>();
                for ( long i = 0; i < recsNum2; i++ ) {
                    insertor.add( new BasicBSONObject( "a", i ) );
                }
                cl.bulkInsert( insertor, SplitUtils.FLG_INSERT_CONTONDUP );
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }
    }

    private class PutLobThread extends SdbThreadBase {
        @Override
        public void exec() throws Exception {
            Sequoiadb sdb = null;
            try {
                sdb = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = sdb.getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                for ( long i = 0; i < lobNum; i++ ) {
                    DBLob dbLob = cl.createLob();
                    dbLob.write( clName.getBytes() );
                    dbLob.close();
                }
            } finally {
                if ( sdb != null ) {
                    sdb.disconnect();
                }
            }
        }
    }

    private void checkCatalog() {
        DBCursor cursor = commSdb
                .getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                        new BasicBSONObject( "Name",
                                SdbTestBase.csName + "." + clName ),
                        null, null );
        BasicBSONList cataInfo = ( BasicBSONList ) cursor.getNext()
                .get( "CataInfo" );
        for ( int i = 0; i < cataInfo.size(); i++ ) {
            String groupName = ( String ) ( ( BSONObject ) cataInfo.get( i ) )
                    .get( "GroupName" );
            if ( groupName.equals( dstGroupName ) ) {
                BSONObject lowBound = ( BSONObject ) ( ( BSONObject ) cataInfo
                        .get( i ) ).get( "LowBound" );
                Assert.assertEquals( lowBound,
                        new BasicBSONObject( "", 2048 ) );
                BSONObject upBound = ( BSONObject ) ( ( BSONObject ) cataInfo
                        .get( i ) ).get( "UpBound" );
                Assert.assertEquals( upBound, new BasicBSONObject( "", 4096 ) );
                break;
            }
        }
    }

    private void checkDataCount() {
        Sequoiadb srcMstDB = null;
        Sequoiadb dstMstDB = null;
        try {
            srcMstDB = commSdb.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();
            dstMstDB = commSdb.getReplicaGroup( dstGroupName ).getMaster()
                    .connect();

            // 校验切分后记录总数正确性
            long srcDataCount = srcMstDB
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName ).getCount();
            long destDataCount = dstMstDB
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName ).getCount();
            Assert.assertEquals( srcDataCount + destDataCount,
                    recsNum1 + recsNum2 );

            // 校验切分后lob总数正确性
            DBCursor srcLobCursor = srcMstDB
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName ).listLobs();
            int srcLobCount = 0;
            while ( srcLobCursor.hasNext() ) {
                srcLobCount++;
                srcLobCursor.getNext();
            }

            DBCursor destLobCursor = dstMstDB
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName ).listLobs();
            int destLobCount = 0;
            while ( destLobCursor.hasNext() ) {
                destLobCount++;
                destLobCursor.getNext();
            }

            Assert.assertEquals( srcLobCount + destLobCount, lobNum );
        } finally {
            if ( srcMstDB != null ) {
                srcMstDB.disconnect();
            }
            if ( dstMstDB != null ) {
                dstMstDB.disconnect();
            }
        }
    }

    private void insertAndCheck() {
        Sequoiadb srcMstDB = null;
        Sequoiadb dstMstDB = null;
        try {
            srcMstDB = commSdb.getReplicaGroup( srcGroupName ).getMaster()
                    .connect();
            DBCollection srcGroupCL = srcMstDB
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );

            dstMstDB = commSdb.getReplicaGroup( dstGroupName ).getMaster()
                    .connect();
            DBCollection dstGroupCL = dstMstDB
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName );

            // 获取一个源组的记录，添加一个c字段，去除_id后重新插入,期望此数据落入源组
            DBCursor srcQueryCursor = srcMstDB
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName )
                    .query( "", null, null, null, 0, -1 );
            BSONObject srcRecord = srcQueryCursor.getNext();
            srcRecord.removeField( "_id" );
            srcRecord.put( "c", -10 );
            commCL.insert( srcRecord );
            Assert.assertTrue( !SplitUtils.isCollectionContainThisJSON(
                    srcGroupCL, srcRecord.toString() ) );

            // 获取一个目标组的记录，添加一个b字段，去除_id后重新插入，期望此数据落入目标组
            DBCursor dstQueryCursor = dstMstDB
                    .getCollectionSpace( SdbTestBase.csName )
                    .getCollection( clName )
                    .query( "", null, null, null, 0, -1 );
            BSONObject dstRecord = dstQueryCursor.getNext();
            dstRecord.removeField( "_id" );
            dstRecord.put( "b", -10 );
            commCL.insert( dstRecord );
            Assert.assertTrue( !SplitUtils.isCollectionContainThisJSON(
                    dstGroupCL, dstRecord.toString() ) );
        } finally {
            if ( srcMstDB != null ) {
                srcMstDB.disconnect();
            }
            if ( dstMstDB != null ) {
                dstMstDB.disconnect();
            }
        }
    }
}
