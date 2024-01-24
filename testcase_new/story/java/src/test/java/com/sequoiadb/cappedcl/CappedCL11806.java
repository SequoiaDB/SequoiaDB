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
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * FileName: seqDB-11806:并发查询
 * 
 * @author liuxiaoxuan
 * @Date 2017.7.24
 */
public class CappedCL11806 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cappedCS = null;
    private DBCollection cappedCL = null;
    private String cappedCLName = "cappedCL_11806";
    private StringBuffer strBuffer = null;
    private int stringLength = CappedCLUtils.getRandomStringLength( 1, 100 );
    private int insertNum = 10000;
    private ThreadExecutor te = new ThreadExecutor( 1800000 );
    private int threadNum = 5;
    private String groupName = null;

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );

        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        cappedCS = sdb.getCollectionSpace( cappedCSName );
        cappedCL = cappedCS.createCollection( cappedCLName,
                ( BSONObject ) JSON.parse( "{Capped:true, Size:10240,Group:'"
                        + groupName + "'}" ) );

        // 构造插入的字符串
        strBuffer = new StringBuffer();
        for ( int len = 0; len < stringLength; len++ ) {
            strBuffer.append( "a" );
        }
        BasicBSONObject insertObj = new BasicBSONObject();
        insertObj.put( "a", strBuffer.toString() );
        CappedCLUtils.insertRecords( cappedCL, insertObj, insertNum );
    }

    @Test
    public void test() throws Exception {
        for ( int i = 0; i < threadNum; i++ ) {
            te.addWorker( new QueryThread() );
        }
        te.run();

        // 校验主节点id字段
        Assert.assertTrue( CappedCLUtils.checkLogicalID( sdb, cappedCSName,
                cappedCLName, stringLength ) );

        // 校验主备一致性
        Assert.assertTrue( CappedCLUtils.isLSNConsistency( sdb, groupName ) );
        Assert.assertTrue( CappedCLUtils.isRecordConsistency( sdb, cappedCSName,
                cappedCLName ) );

    }

    @AfterClass
    public void tearDown() {
        try {
            cappedCS.dropCollection( cappedCLName );
        } finally {
            sdb.close();
        }
    }

    private class QueryThread {

        @ExecuteOrder(step = 1, desc = "查询记录")
        public void insert() {
            Sequoiadb db = null;
            DBCollection cl = null;
            try {
                db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
                cl = db.getCollectionSpace( cappedCSName )
                        .getCollection( cappedCLName );
                System.out.println( this.getClass().getName().toString()
                        + " start at:"
                        + new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss.S" )
                                .format( new Date() ) );
                DBCursor cursor = cl.query();
                while ( cursor.hasNext() ) {
                    cursor.getNext();
                }
                cursor.close();
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
