package com.sequoiadb.crud.truncate;

import com.sequoiadb.exception.SDBError;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.testcommon.SdbThreadBase;

/**
 * @FileName:seqDB-171:split与truncate的并发 插入数据，一条线程执行切分，另一条线程执行truncate
 * @Author linsuqiang
 * @Date 2016-12-06
 * @Version 1.00
 */
public class TestTruncate171 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String clName = "cl171";
    private String srcGroupName = null;
    private String dstGroupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( TruncateUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }

        if ( TruncateUtils.OneGroupMode( sdb ) ) {
            throw new SkipException( "less two groups skip testcase" );
        }
        BSONObject options = ( BSONObject ) JSON
                .parse( "{ShardingKey:{a:1},ShardingType:'hash'}" );
        CollectionSpace cs = sdb.getCollectionSpace( csName );
        DBCollection cl = cs.createCollection( clName, options );
        TruncateUtils.insertData( cl );
        srcGroupName = TruncateUtils.getSrcGroupName( sdb, cl );
        dstGroupName = TruncateUtils.getDstGroupName( sdb, srcGroupName );
    }

    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cs.dropCollection( clName );
        } finally {
            sdb.close();
        }
    }

    @Test
    public void test() {
        TruncateThread truncateThread = new TruncateThread();
        SplitThread splitThread = new SplitThread();

        splitThread.start();
        truncateThread.start();

        if ( !( truncateThread.isSuccess() && splitThread.isSuccess() ) ) {
            Assert.fail(
                    truncateThread.getErrorMsg() + splitThread.getErrorMsg() );
        }
    }

    private class TruncateThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // doing truncate
                cl.truncate();
                // check truncate
                TruncateUtils.checkTruncated( db, cl );
            } finally {
                db.close();
            }
        }
    }

    private class SplitThread extends SdbThreadBase {
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                db.setSessionAttr(
                        ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
                cl = db.getCollectionSpace( csName ).getCollection( clName );
                // doing split
                cl.split( srcGroupName, dstGroupName, 50 );
                // check split
                checkSplit( db, srcGroupName, dstGroupName );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_DMS_TRUNCATED
                        .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_LOCK_FAILED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_TASK_HAS_CANCELED
                                .getErrorCode() ) {
                    throw e;
                }
            } finally {
                db.close();
            }
        }
    }

    private void checkSplit( Sequoiadb db, String srcGroupName,
            String dstGroupName ) {
        // get CataInfo
        BSONObject option = new BasicBSONObject();
        option.put( "Name", csName + '.' + clName );
        DBCursor snapshot = db.getSnapshot( 8, option, null, null );
        BasicBSONList cataInfo = ( BasicBSONList ) snapshot.getNext()
                .get( "CataInfo" );
        snapshot.close();

        // justify source group catalog information
        BSONObject srcInfo = ( BSONObject ) cataInfo.get( 0 );
        int expSrcLowBound = 0;
        int expSrcUpBound = 2048;
        if ( !( ( ( String ) srcInfo.get( "GroupName" ) ).equals( srcGroupName )
                && ( ( BasicBSONObject ) srcInfo.get( "LowBound" ) )
                        .getInt( "" ) == expSrcLowBound
                && ( ( BasicBSONObject ) srcInfo.get( "UpBound" ) )
                        .getInt( "" ) == expSrcUpBound ) ) {
            Assert.fail( "split fail: source group cataInfo is wrong" );
        }

        // justify destination group catalog information
        BSONObject dstInfo = ( BSONObject ) cataInfo.get( 1 );
        int expDstLowBound = 2048;
        int expDstUpBound = 4096;
        if ( !( ( ( String ) dstInfo.get( "GroupName" ) ).equals( dstGroupName )
                && ( ( BasicBSONObject ) dstInfo.get( "LowBound" ) )
                        .getInt( "" ) == expDstLowBound
                && ( ( BasicBSONObject ) dstInfo.get( "UpBound" ) )
                        .getInt( "" ) == expDstUpBound ) ) {
            Assert.fail( "split fail: destination group cataInfo is wrong" );
        }
    }
}
