package com.sequoiadb.datasrc.brokennetwork;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;
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
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasrc.DataSrcUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-23409:使用数据源的集合空间执行数据操作过程中网络异常
 * @author liuli
 * @Date 2021.06.02
 * @version 1.10
 */

public class DataSource23409 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb srcdb = null;
    private GroupMgr groupMgr = null;
    private GroupMgr srcGroupMgr = null;
    private String dataSrcName = "datasource23409";

    private String srcCSName = "cssrc_23409";
    private String csName = "cs_23409";
    private String clName = "cl_23409";
    private CollectionSpace cs = null;
    private DBCollection dbcl = null;
    private Random random = new Random();
    private LinkedBlockingQueue< SaveOidAndMd5 > id2md5 = new LinkedBlockingQueue< SaveOidAndMd5 >();

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        srcGroupMgr = GroupMgr.getInstance( DataSrcUtils.getSrcUrl() );
        if ( !srcGroupMgr.checkBusiness( DataSrcUtils.getSrcUrl() ) ) {
            throw new SkipException( "checkBusiness failed" );
        }
        DataSrcUtils.clearDataSource( sdb, csName, dataSrcName );
        DataSrcUtils.createDataSource( sdb, dataSrcName );
        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        DataSrcUtils.createCSAndCL( srcdb, srcCSName, clName );
        cs = sdb.createCollectionSpace( csName );
        BasicBSONObject options = new BasicBSONObject();
        options.put( "DataSource", dataSrcName );
        options.put( "Mapping", srcCSName + "." + clName );
        dbcl = cs.createCollection( clName, options );
        // write lob
        int lobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = DataSrcUtils.getRandomBytes( lobSize );
        int lobtimes = 5;
        writeLobAndGetMd5( dbcl, lobtimes, wlobBuff );
    }

    @Test
    public void test() throws Exception {
        FaultMakeTask faultTask = BrokenNetwork
                .getFaultMakeTask( DataSrcUtils.getSrcIp(), 0, 20 );
        TaskMgr mgr = new TaskMgr( faultTask );
        mgr.addTask( new RemoveLob() );
        mgr.addTask( new ReadLob() );
        mgr.addTask( new WriteLob() );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( srcGroupMgr.checkBusinessWithLSN( 600,
                DataSrcUtils.getSrcUrl() ), true );

        srcdb = new Sequoiadb( DataSrcUtils.getSrcUrl(), DataSrcUtils.getUser(),
                DataSrcUtils.getPasswd() );
        String match = "Name\\\\:" + csName + "\\\\.";
        CommLib.waitContextClose( sdb, match, 300, true );
        String srcMatch = "Name\\\\:" + srcCSName + "\\\\.";
        CommLib.waitContextClose( srcdb, srcMatch, 300, true );

        int lobSize = random.nextInt( 1024 * 1024 );
        byte[] lobBuff = DataSrcUtils.getRandomBytes( lobSize );
        int lobtimes = 2;
        List< ObjectId > lobIds = writeLobAndGetMd5( dbcl, lobtimes, lobBuff );
        checkLobMD5( dbcl, lobIds, lobBuff );
        for ( ObjectId lobId : lobIds ) {
            dbcl.removeLob( lobId );
        }
        checkRemoveLobResult( lobIds );

        CommLib.waitContextClose( sdb, match, 300, true );
        CommLib.waitContextClose( srcdb, srcMatch, 300, true );
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

    private class RemoveLob extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                SaveOidAndMd5 oidAndMd5 = id2md5.take();
                dbcl.removeLob( oidAndMd5.getOid() );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NOT_CONNECTED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK_CLOSE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NET_SEND_ERR
                                .getErrorCode() ) {
                    e.printStackTrace();
                }
            }
        }
    }

    private class ReadLob extends OperateTask {
        @Override
        public void exec() throws Exception {
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
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NOT_CONNECTED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK_CLOSE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NET_SEND_ERR
                                .getErrorCode() ) {
                    e.printStackTrace();
                }
            }
        }
    }

    private class WriteLob extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection dbcl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                int lobtimes = 5;
                int writeLobSize = random.nextInt( 1024 * 1024 );
                byte[] wlobBuff = DataSrcUtils.getRandomBytes( writeLobSize );
                writeLobAndGetMd5( dbcl, lobtimes, wlobBuff );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_TIMEOUT.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NET_CANNOT_CONNECT
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NOT_CONNECTED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NETWORK_CLOSE
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_NET_SEND_ERR
                                .getErrorCode() ) {
                    e.printStackTrace();
                }
            }
        }
    }

    private void checkRemoveLobResult( List< ObjectId > lobIds ) {
        for ( ObjectId lobId : lobIds ) {
            try {
                dbcl.openLob( lobId );
                Assert.fail( "the lob: " + lobId
                        + " has been deleted and the read should fail" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -4 ) {
                    throw e;
                }
            }
        }

    }

    private boolean checkLobMD5( DBCollection cl, List< ObjectId > lobIds,
            byte[] expData ) {
        String expMD5 = getMd5( expData );
        for ( ObjectId lobId : lobIds ) {
            byte[] data = new byte[ expData.length ];
            DBLob lob = cl.openLob( lobId );
            lob.read( data );
            lob.close();
            String actMD5 = getMd5( data );
            if ( !actMD5.equals( expMD5 ) ) {
                throw new BaseException( 0, "check lob: " + lobId
                        + " md5 error, exp: " + expMD5 + ", act: " + actMD5 );
            }
        }
        return true;
    }

    // find the md5 from expected queue
    private String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";

        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else if ( inbuff instanceof byte[] ) {
                md5.update( ( byte[] ) inbuff );
            } else {
                Assert.fail( "invalid parameter!" );
            }
            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            Assert.fail( "fail to get md5!" + e.getMessage() );
        }
        return value;
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

    private List< ObjectId > writeLobAndGetMd5( DBCollection cl, int lobtimes,
            byte[] wlobBuff ) {
        List< ObjectId > idList = new ArrayList< ObjectId >();
        for ( int i = 0; i < lobtimes; i++ ) {
            ObjectId oid = DataSrcUtils.createAndWriteLob( cl, wlobBuff );

            // save oid and md5
            String prevMd5 = DataSrcUtils.getMd5( wlobBuff );
            id2md5.offer( new SaveOidAndMd5( oid, prevMd5 ) );
            idList.add( oid );
        }
        return idList;
    }
}
