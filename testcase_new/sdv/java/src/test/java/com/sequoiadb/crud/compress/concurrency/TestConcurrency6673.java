package com.sequoiadb.crud.compress.concurrency;

import java.util.concurrent.atomic.AtomicInteger;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
 * @FileName:seqDB-6673:创建多个lzw压缩表，并发插入数据 1、创建多个lzw压缩表，并发插入大量数据
 *                                        2、检查各个CL数据正确性，及数据压缩正确性
 * @Date 2016-12-26
 * @Version 1.00
 */
public class TestConcurrency6673 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String dataGroupName = null;
    private static final int CL_COUNT = 5;
    private String ranStr = CompressUtils.getRandomString( 8 * 1024 );

    @BeforeClass
    public void setUp() {
        try {
            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        if ( CompressUtils.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        for ( int i = 0; i < CL_COUNT; i++ ) {
            String clName = "cl_6673_" + i;
            createCL( clName );
        }
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            for ( int i = 0; i < CL_COUNT; i++ ) {
                String clName = "cl_6673_" + i;
                if ( cs.isCollectionExist( clName ) ) {
                    cs.dropCollection( clName );
                }
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( sdb != null ) {
                sdb.disconnect();
            }
        }
    }

    @Test
    public void test() {
        LzwThread lzwThread = new LzwThread();
        lzwThread.start( CL_COUNT );
        if ( !lzwThread.isSuccess() ) {
            Assert.fail( lzwThread.getErrorMsg() );
        }
    }

    private class LzwThread extends SdbThreadBase {
        private AtomicInteger clNo = new AtomicInteger( 0 );

        @SuppressWarnings({ "resource", "deprecation" })
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                String clName = "cl_6673_" + clNo.getAndIncrement();
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                // insert data for creating dictionary
                for ( int i = 0; i < 9000; i++ ) {
                    BSONObject rec = new BasicBSONObject();
                    rec.put( "a", i );
                    rec.put( "b", ranStr + i );
                    cl.insert( rec );
                }
                waitCreateDict( cl, dataGroupName );

                // insert data for compression
                for ( int i = 9000; i < 10000; i++ ) {
                    BSONObject rec = new BasicBSONObject();
                    rec.put( "a", i );
                    rec.put( "b", ranStr + i );
                    cl.insert( rec );
                }
                CompressUtils.checkCompressed( cl, dataGroupName, "lzw" );

                // check data correctness
                checkData( cl );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private DBCollection createCL( String clName ) {
        DBCollection cl = null;
        try {
            BSONObject option = new BasicBSONObject();
            dataGroupName = CompressUtils.getDataGroups( sdb ).get( 0 );
            option.put( "Group", dataGroupName );
            option.put( "Compressed", true );
            option.put( "CompressionType", "lzw" );
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    @SuppressWarnings("deprecation")
    private void waitCreateDict( DBCollection cl, String dataGroupName ) {
        int passSecond = 0;
        int waitSecond = 300;
        for ( passSecond = 0; passSecond < waitSecond; passSecond++ ) {
            try {
                Thread.sleep( 1000 );
                if ( CompressUtils.isDictExist( cl, dataGroupName ) ) {
                    break;
                }
            } catch ( BaseException e ) {
                throw e;
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        }
        if ( passSecond == waitSecond ) {
            throw new BaseException( "fail to create dictionary" );
        }
    }

    @SuppressWarnings("deprecation")
    private void checkData( DBCollection cl ) {
        // check count
        int expCnt = 10000;
        int actCnt = ( int ) cl.getCount();
        if ( actCnt != expCnt ) {
            throw new BaseException( "data is different at count" );
        }
        // check content
        DBCursor cursor = cl.query( "", "{_id:{$include:0}}", "{a:1}", "" );
        for ( int i = 0; i < 10000; i++ ) {
            BSONObject expRec = new BasicBSONObject();
            expRec.put( "a", i );
            expRec.put( "b", ranStr + i );
            BSONObject actRec = cursor.getNext();
            if ( !actRec.equals( expRec ) ) {
                throw new BaseException( "data is different at content" );
            }
        }
        cursor.close();
    }
}