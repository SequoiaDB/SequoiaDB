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

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * FileName: seqDB-18836:多个固定集合空间下，并发创建固定集合
 * 
 * @author zhaoyu
 * @Date 2019.7.22
 */
public class CappedCL18836 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private String cappedCSName = "cappedCS_18836";
    private String cappedCLName = "cappedCL_18836";
    private int csNum = 3;
    private int clNum = 3;
    private StringBuffer strBuffer = null;
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 2000 );
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
            sdb.createCollectionSpace( cappedCSNamei,
                    ( BSONObject ) JSON.parse( "{Capped:true}" ) );
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
                te.addWorker( new CreateCLThread( cappedCSName + "_" + i,
                        cappedCLName + "_" + j ) );
            }
        }
        te.run();

        // 检查集合空间及集合创建成功，记录正确
        for ( int i = 0; i < csNum; i++ ) {
            Assert.assertTrue(
                    sdb.isCollectionSpaceExist( cappedCSName + "_" + i ),
                    cappedCSName + "_" + i + " is not exists" );
            for ( int j = 0; j < clNum; j++ ) {
                Assert.assertTrue(
                        sdb.getCollectionSpace( cappedCSName + "_" + i )
                                .isCollectionExist( cappedCLName + "_" + j ),
                        cappedCLName + "_" + j + " is not exists in "
                                + cappedCSName + "_" + i );

                DBCollection cl = sdb
                        .getCollectionSpace( cappedCSName + "_" + i )
                        .getCollection( cappedCLName + "_" + j );
                BasicBSONObject insertObj = new BasicBSONObject();
                insertObj.put( "a", strBuffer.toString() );
                CappedCLUtils.insertRecords( cl, insertObj, 10 );

                // 校验主节点id字段
                Assert.assertTrue( CappedCLUtils.checkLogicalID( sdb,
                        cappedCSName + "_" + i, cappedCLName + "_" + j,
                        stringLength ) );

                // 校验主备一致性
                Assert.assertTrue(
                        CappedCLUtils.isLSNConsistency( sdb, groupName ) );
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

    private class CreateCLThread {
        String csName = null;
        String clName = null;

        public CreateCLThread( String csName, String clName ) {
            this.csName = csName;
            this.clName = clName;
        }

        @ExecuteOrder(step = 1, desc = "创建集合")
        public void createCappedCL() {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                System.out.println( this.getClass().getName().toString()
                        + " start at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
                db.getCollectionSpace( csName ).createCollection( clName,
                        ( BSONObject ) JSON
                                .parse( "{Capped:true, Size:1024, Group:'"
                                        + groupName + "'}" ) );
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
