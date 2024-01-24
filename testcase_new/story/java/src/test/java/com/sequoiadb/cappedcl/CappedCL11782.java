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
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/*
 * @FileName:
 * seqDB-11782:默认配置，同时在普通集合和固定集合上做并发插入操作
 * seqDB-11783:并发同步配置较大值，同时在普通集合和固定集合上做大并发插入操作
 * seqDB-11781:关闭并发同步，同时在普通集合和固定集合上做并发插入操作
 * @Author liuxiaoxuan
 * @Date 2017.7.18
 */

public class CappedCL11782 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private CollectionSpace cappedCS = null;
    private String cappedCLName = "cappedCL_11782";
    private String clName = "cl_11782";
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 4000 );
    private StringBuffer strBuffer = null;
    private int threadNum = 5;
    private String groupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        cs = sdb.getCollectionSpace( csName );
        cappedCS = sdb.getCollectionSpace( cappedCSName );
        cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        cappedCS.createCollection( cappedCLName, ( BSONObject ) JSON.parse(
                "{Capped:true, Size:10240,Group:'" + groupName + "'}" ) );

        // 构造插入的字符串
        strBuffer = new StringBuffer();
        for ( int len = 0; len < stringLength; len++ ) {
            strBuffer.append( "a" );
        }
    }

    @DataProvider(name = "updateConf")
    public Object[][] createIndex() {
        return new Object[][] { /*
                                 * { (BSONObject) JSON.parse("{maxreplsync:0}")
                                 * },会导致ci虚拟机重放太慢
                                 */
                { ( BSONObject ) JSON.parse( "{maxreplsync:200}" ) },
                { ( BSONObject ) JSON.parse( "{maxreplsync:10}" ) } };
    }

    @Test(dataProvider = "updateConf")
    public void test( BSONObject config ) throws Exception {
        sdb.updateConfig( config, new BasicBSONObject() );

        ThreadExecutor te = new ThreadExecutor( 1800000 );
        for ( int i = 0; i < threadNum; i++ ) {
            te.addWorker( new InsertThread( csName, clName ) );
            te.addWorker( new InsertThread( cappedCSName, cappedCLName ) );
        }
        te.run();

        // 校验主节点id字段
        Assert.assertTrue( CappedCLUtils.checkLogicalID( sdb, cappedCSName,
                cappedCLName, stringLength ) );

        // 校验主备一致性
        Assert.assertTrue( CappedCLUtils.isLSNConsistency( sdb, groupName ) );
        Assert.assertTrue( CappedCLUtils.isRecordConsistency( sdb, cappedCSName,
                cappedCLName ) );
        Assert.assertTrue(
                CappedCLUtils.isRecordConsistency( sdb, csName, clName ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            cs.dropCollection( clName );
            cappedCS.dropCollection( cappedCLName );
        } finally {
            sdb.updateConfig( ( BSONObject ) JSON.parse( "{maxreplsync:10}" ),
                    new BasicBSONObject() );
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
