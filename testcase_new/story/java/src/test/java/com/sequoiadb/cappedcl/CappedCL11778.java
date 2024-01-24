package com.sequoiadb.cappedcl;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * FileName: seqDB-11778:多个固定集合并发插入
 * 
 * @author liuxiaoxuan
 * @Date 2017.7.18
 */
public class CappedCL11778 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String cappedCSName = "cappedCS_11778";
    private String cappedCLName = "cappedCL_11778";
    private int csNum = 2;
    private int clNum = 2;
    private StringBuffer strBuffer = null;
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 4000 );
    private int threadNum = 5;
    private ThreadExecutor te = new ThreadExecutor( 1800000 );
    private String groupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        for ( int i = 0; i < csNum; i++ ) {
            String cappedCSNamei = cappedCSName + "_" + i;
            if ( sdb.isCollectionSpaceExist( cappedCSNamei ) ) {
                sdb.dropCollectionSpace( cappedCSNamei );
            }
            CollectionSpace cappedCS = sdb.createCollectionSpace( cappedCSNamei,
                    ( BSONObject ) JSON.parse( "{Capped:true}" ) );
            for ( int j = 0; j < clNum; j++ ) {
                cappedCS.createCollection( cappedCLName + "_" + j,
                        ( BSONObject ) JSON
                                .parse( "{Capped:true, Size:10240,Group:'"
                                        + groupName + "'}" ) );

            }
        }

        // 构造插入的字符串
        strBuffer = new StringBuffer();
        for ( int len = 0; len < stringLength; len++ ) {
            strBuffer.append( "a" );
        }
    }

    @Test
    public void test() throws Exception {
        for ( int i = 0; i < csNum; i++ ) {
            for ( int j = 0; j < clNum; j++ ) {
                for ( int k = 0; k < threadNum; k++ ) {
                    te.addWorker( new InsertThread( cappedCSName + "_" + i,
                            cappedCLName + "_" + j ) );
                }
            }
        }
        te.run();

        // 校验主备节点lsn
        Assert.assertTrue( CappedCLUtils.isLSNConsistency( sdb, groupName ) );

        // 校验记录
        for ( int i = 0; i < csNum; i++ ) {
            for ( int j = 0; j < clNum; j++ ) {
                // 校验主节点id字段
                Assert.assertTrue( CappedCLUtils.checkLogicalID( sdb,
                        cappedCSName + "_" + i, cappedCLName + "_" + j,
                        stringLength ) );

                // 校验主备一致性
                Assert.assertTrue( CappedCLUtils.isRecordConsistency( sdb,
                        cappedCSName + "_" + i, cappedCLName + "_" + j ) );
            }
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            for ( int i = 0; i < csNum; i++ ) {
                sdb.dropCollectionSpace( cappedCSName + "_" + i );
            }
        } finally {
            sdb.close();
        }
    }

    private class InsertThread {
        String csName = null;
        String clName = null;

        public InsertThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "插入记录")
        public void insert() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                System.out.println( this.getClass().getName().toString()
                        + " start at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
                for ( int i = 0; i < 10000; i++ ) {
                    BasicBSONObject insertObj = new BasicBSONObject();
                    insertObj.put( "a", strBuffer.toString() );
                    cl.insert( insertObj );
                }
                System.out.println( this.getClass().getName().toString()
                        + " stop at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
            } finally {
                db.close();
            }

        }
    }
}
