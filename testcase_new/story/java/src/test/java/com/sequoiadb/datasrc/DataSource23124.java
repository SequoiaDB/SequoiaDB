package com.sequoiadb.datasrc;

import java.util.Iterator;
import java.util.Random;
import java.util.concurrent.LinkedBlockingQueue;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @Description seqDB-23124:并发使用数据源执行lob操作
 * @author liuli
 * @Date 2021.02.25
 * @version 1.10
 */
public class DataSource23124 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private String dataSrcName = "datasource23124";
    private String csName = "cs_23124";
    private String srcCSName = "cssrc_23124";
    private String srcCLName = "clsrc_23124";
    private String clName = "cl_23124";

    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private Random random = new Random();
    private LinkedBlockingQueue< SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue< SaveOidAndMd5 >();

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "is standalone skip testcase" );
        }
        DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, srcCLName );
        cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName + "." + srcCLName );
        cl = cs.createCollection( clName, options );

        // write lob
        int lobtimes = 100;
        writeLobAndGetMd5( cl, lobtimes );
    }

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        ReadLob readLob = new ReadLob();
        WriteLob writeLob = new WriteLob();
        RemoveLob removeLob = new RemoveLob();
        es.addWorker( readLob );
        es.addWorker( writeLob );
        es.addWorker( removeLob );
        es.run();

        Assert.assertEquals( readLob.getRetCode(), 0 );
        Assert.assertEquals( writeLob.getRetCode(), 0 );
        Assert.assertEquals( removeLob.getRetCode(), 0 );

        // check the write and remove result
        checkLobData();
    }

    @AfterClass
    public void tearDown() {
        try {
            srcdb.dropCollectionSpace( srcCSName );
            DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( srcdb != null ) {
                srcdb.close();
            }
        }
    }

    private class RemoveLob extends ResultStore {
        @ExecuteOrder(step = 2)
        public void read() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                SaveOidAndMd5 oidAndMd5 = id2md5.take();
                dbcl.removeLob( oidAndMd5.getOid() );
            }
        }
    }

    private class ReadLob extends ResultStore {
        @ExecuteOrder(step = 1)
        public void read() throws InterruptedException {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                db.setSessionAttr( ( BSONObject ) JSON
                        .parse( "{'PreferedInstance':'M'}" ) );
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                SaveOidAndMd5 oidAndMd5 = id2md5.take();
                ObjectId oid = oidAndMd5.getOid();

                try ( DBLob rLob = dbcl.openLob( oid, DBLob.SDB_LOB_READ )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    String curMd5 = DataSrcUtils.getMd5( rbuff );
                    String prevMd5 = oidAndMd5.getMd5();
                    Assert.assertEquals( curMd5, prevMd5 );
                }
                id2md5.offer( oidAndMd5 );
            }
        }
    }

    private class WriteLob extends ResultStore {
        @ExecuteOrder(step = 1)
        public void write() {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                int lobtimes = 2;
                writeLobAndGetMd5( dbcl, lobtimes );
            }
        }
    }

    private void checkLobData() {
        try ( DBCursor listLob = cl.listLobs()) {
            while ( listLob.hasNext() ) {
                BasicBSONObject obj = ( BasicBSONObject ) listLob.getNext();
                ObjectId existOid = obj.getObjectId( "Oid" );
                try ( DBLob rLob = cl.openLob( existOid )) {
                    byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                    rLob.read( rbuff );
                    String curMd5 = DataSrcUtils.getMd5( rbuff );
                    String prevMd5 = getLobMd5ByOid( existOid );
                    ;
                    Assert.assertEquals( curMd5, prevMd5,
                            "the list oid:" + existOid.toString() );
                }
            }
        }
        // the list lobnums must be equal with the nums of the id2mda,if id2md5
        // is not empty
        Assert.assertEquals( id2md5.isEmpty(), true,
                "the remaining " + id2md5.size() + " oids were not found!" );
    }

    // find the md5 from expected queue
    private String getLobMd5ByOid( ObjectId lobOid ) {
        Iterator< SaveOidAndMd5 > iterator = id2md5.iterator();
        boolean found = false;
        String findMd5 = "";
        while ( iterator.hasNext() ) {
            SaveOidAndMd5 current = iterator.next();
            ObjectId oid = current.getOid();
            if ( oid.equals( lobOid ) ) {
                findMd5 = current.getMd5();
                id2md5.remove( current );
                found = true;
                break;
            }
        }

        // if oid does not exist in the queue,than error
        if ( !found ) {
            throw new RuntimeException( "oid[" + lobOid + "] not found" );
        }
        return findMd5;
    }

    private class SaveOidAndMd5 {
        private ObjectId oid;
        private String md5;

        public SaveOidAndMd5( ObjectId oid, String md5 ) {
            this.oid = oid;
            this.md5 = md5;
        }

        public ObjectId getOid() {
            return oid;
        }

        public String getMd5() {
            return md5;
        }
    }

    private void writeLobAndGetMd5( DBCollection cl, int lobtimes ) {
        for ( int i = 0; i < lobtimes; i++ ) {
            int writeLobSize = random.nextInt( 1024 * 1024 );
            byte[] wlobBuff = DataSrcUtils.getRandomBytes( writeLobSize );
            ObjectId oid = DataSrcUtils.createAndWriteLob( cl, wlobBuff );

            // save oid and md5
            String prevMd5 = DataSrcUtils.getMd5( wlobBuff );
            id2md5.offer( new SaveOidAndMd5( oid, prevMd5 ) );
        }
    }

}
