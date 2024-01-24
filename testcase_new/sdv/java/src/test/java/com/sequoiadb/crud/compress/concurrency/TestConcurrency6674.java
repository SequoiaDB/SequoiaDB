package com.sequoiadb.crud.compress.concurrency;

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
 * @FileName:seqDB-6674:对不同压缩类型的CL并发插入数据 1、创建多个不同压缩类型的CL（包括不压缩、snappy压缩、lzw压缩），并发插入大量数据
 *                                       2、检查各个CL数据正确性，及数据压缩正确性
 * @Date 2016-12-26
 * @Version 1.00
 */
public class TestConcurrency6674 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private String dataGroupName = null;
    private String ranStr = CompressUtils.getRandomString( 512 * 1024 );

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
        dataGroupName = CompressUtils.getDataGroups( sdb ).get( 0 );
    }

    @SuppressWarnings("deprecation")
    @AfterClass
    public void tearDown() {
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            for ( int i = 0; i < 3; i++ ) {
                String clName = "cl_6674_" + i;
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
        SnappyThread snappyThread = new SnappyThread();
        NormalThread normalThread = new NormalThread();

        lzwThread.start();
        snappyThread.start();
        normalThread.start();

        if ( !( lzwThread.isSuccess() && snappyThread.isSuccess()
                && normalThread.isSuccess() ) ) {
            Assert.fail( lzwThread.getErrorMsg() + snappyThread.getErrorMsg()
                    + normalThread.getErrorMsg() );
        }
    }

    private class LzwThread extends SdbThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                String clName = "cl_6674_0";
                DBCollection cl = createCL( db, clName, "lzw" );
                // insert data for creating dictionary
                for ( int i = 0; i < 140; i++ ) {
                    BSONObject rec = new BasicBSONObject();
                    rec.put( "a", i );
                    rec.put( "b", ranStr + i );
                    cl.insert( rec );
                }
                CompressUtils.waitCreateDict( cl, dataGroupName );

                // insert data for compression
                for ( int i = 140; i < 150; i++ ) {
                    BSONObject rec = new BasicBSONObject();
                    rec.put( "a", i );
                    rec.put( "b", ranStr + i );
                    cl.insert( rec );
                }
                CompressUtils.checkCompressed( cl, dataGroupName, "lzw" );

                // check data correctness
                checkData( cl );
            } catch ( BaseException e ) {
                e.printStackTrace();
                throw e;
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private class SnappyThread extends SdbThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                String clName = "cl_6674_1";
                DBCollection cl = createCL( db, clName, "snappy" );
                for ( int i = 0; i < 150; i++ ) {
                    BSONObject rec = new BasicBSONObject();
                    rec.put( "a", i );
                    rec.put( "b", ranStr + i );
                    cl.insert( rec );
                }
                checkData( cl );
                CompressUtils.checkCompressed( cl, dataGroupName, "snappy" );
            } catch ( BaseException e ) {
                e.printStackTrace();
                throw e;
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    private class NormalThread extends SdbThreadBase {
        @SuppressWarnings("deprecation")
        @Override
        public void exec() throws BaseException {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                String clName = "cl_6674_2";
                DBCollection cl = createCL( db, clName, null );
                for ( int i = 0; i < 150; i++ ) {
                    BSONObject rec = new BasicBSONObject();
                    rec.put( "a", i );
                    rec.put( "b", ranStr + i );
                    cl.insert( rec );
                }
                checkData( cl );
                CompressUtils.checkCompressed( cl, dataGroupName, null );
            } catch ( BaseException e ) {
                e.printStackTrace();
                throw e;
            } finally {
                if ( db != null ) {
                    db.disconnect();
                }
            }
        }
    }

    /**
     * create cl of different compression type
     * 
     * @param clName
     * @param compressionType.
     *            if null, not compressed.
     * @return cl
     */
    private DBCollection createCL( Sequoiadb db, String clName,
            String compressionType ) {
        DBCollection cl = null;
        try {
            BSONObject option = new BasicBSONObject();
            option.put( "Group", dataGroupName );
            if ( compressionType != null ) {
                option.put( "Compressed", true );
                option.put( "CompressionType", compressionType );
            } else {
                option.put( "Compressed", false );
            }
            CollectionSpace cs = db.getCollectionSpace( csName );
            cl = cs.createCollection( clName, option );
        } catch ( BaseException e ) {
            throw e;
        }
        return cl;
    }

    @SuppressWarnings("deprecation")
    private void checkData( DBCollection cl ) {
        // check count
        int expCnt = 150;
        int actCnt = ( int ) cl.getCount();
        if ( actCnt != expCnt ) {
            throw new BaseException( "data is different at count" );
        }
        // check content
        DBCursor cursor = cl.query( "", "{_id:{$include:0}}", "{a:1}", "" );
        for ( int i = 0; i < 150; i++ ) {
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