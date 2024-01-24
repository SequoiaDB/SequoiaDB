package com.sequoiadb.datasync.killnode;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.OprLobTask;
import com.sequoiadb.datasync.OprLobTask;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.*;

/**
 * @FileName WriteLobAndKillPrimaryNode3217.java test content:when write and
 *           remove Lob, kill -9 the data group master node, and the fault node
 *           is synchronous source node. testlink case:seqDB-3217
 * @author wuyan
 * @Date 2017.5.3
 * @version 1.00
 */

public class WriteLobAndKillPrimaryNode3217 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;
    private boolean clearFlag = false;
    private String clName = "cl3217";
    private String clGroupName = null;
    private Random random = new Random();
    private Stack< ObjectId > oids = new Stack< ObjectId >();

    @BeforeClass
    public void setUp() {
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cs = sdb.getCollectionSpace( SdbTestBase.csName );
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );

            cl = createCL();
            int lobNums = 100;
            putLobs( cl, lobNums );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        }
    }

    @Test
    public void test() {
        try {
            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            NodeWrapper priNode = dataGroup.getMaster();

            FaultMakeTask faultTask = KillNode.getFaultMakeTask(
                    priNode.hostName(), priNode.svcName(), 1 );
            TaskMgr mgr = new TaskMgr( faultTask );
            OprLobTask oTask = new OprLobTask(clName);
            mgr.addTask( oTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            // check whether the cluster is normal and lsn consistency ,the
            // longest waiting time is 600S
            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true,
                    "checkBusinessWithLSN() occurs timeout" );
           
            // check whether the lob is written before the fault
            checkLobBeforeTheFault();

            // put lob again after fault recovery
            int lobNum = 1;
            putLobs( cl, lobNum );

            Assert.assertEquals( groupMgr.checkBusinessWithLSN( 120 ), true,
                    "checkBusinessWithLSN() occurs timeout" );

            // check the consistency between nodes
            checkConsistency( dataGroup );

            // Normal operating environment
            clearFlag = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( clearFlag ) {
                cs.dropCollection( clName );
            }
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private DBCollection createCL() {
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ ReplSize: 2, Group: '" + clGroupName + "' }" );
        return cs.createCollection( clName, option );
    }

    private void putLobs( DBCollection cl, int lobNums ) {
        int lobSize = random.nextInt( 1048576 );
        byte[] lobBytes = new byte[ lobSize ];

        for ( int i = 0; i < lobNums; i++ ) {
            DBLob lob = cl.createLob();
            lob.write( lobBytes );
            ObjectId currOid = lob.getID();
            oids.push( currOid );
            lob.close();
        }
    }

    private void checkLobBeforeTheFault() {
        DBCollection cl = cs.getCollection( clName );
        try {
            // do read lobs
            while ( !oids.isEmpty() ) {
                ObjectId oid = oids.pop();
                DBLob lob = cl.openLob( oid );
                byte[] buff = new byte[ ( int ) lob.getSize() ];
                lob.read( buff );
                lob.close();
            }
        } catch ( BaseException e ) {
            Assert.fail( "the lob is not exist: " + e.getErrorCode()
                    + e.getErrorType() );
        }
    }

    private void checkConsistency( GroupWrapper dataGroup ) {
        List< String > dataUrls = dataGroup.getAllUrls();
        List< ObjectId > result = new ArrayList< ObjectId >();

        // check the list lobs of every node
        int preno = 0;
        for ( int i = 0; i < dataUrls.size(); i++ ) {
            try ( Sequoiadb dataDB = new Sequoiadb( dataUrls.get( i ), "",
                    "" )) {
                DBCollection cl = dataDB
                        .getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                DBCursor listCursor = cl.listLobs();
                int count = 0;
                while ( listCursor.hasNext() ) {
                    BSONObject obj1 = listCursor.getNext();
                    if ( i == 0 ) {
                        ObjectId queryLobId = ( ObjectId ) obj1.get( "Oid" );
                        result.add( queryLobId );
                    } else {
                        count++;
                    }
                }
                if ( preno == 0 ) {
                    preno = result.size();
                } else {
                    if ( preno != count ) {
                        Assert.fail( "the loblist is different." + " the preno="
                                + preno + " count=" + count );
                    }
                }
                listCursor.close();
            } catch ( BaseException e ) {
                Assert.fail( "the lob is not exist: " + e.getErrorCode()
                        + e.getErrorType() );
            }

        }

        Random random = new Random();
        int oidNo = random.nextInt( result.size() );
        ObjectId oid = result.get( oidNo );
        String preMd5 = "";
        for ( int i = 0; i < dataUrls.size(); i++ ) {
            try ( Sequoiadb dataDB = new Sequoiadb( dataUrls.get( i ), "",
                    "" )) {
                DBCollection cl1 = dataDB
                        .getCollectionSpace( SdbTestBase.csName )
                        .getCollection( clName );
                DBLob rLob = cl1.openLob( oid );
                byte[] rbuff = new byte[ 1024 ];
                int readLen = 0;
                ByteBuffer bytebuff = ByteBuffer
                        .allocate( ( int ) rLob.getSize() );
                while ( ( readLen = rLob.read( rbuff ) ) != -1 ) {
                    bytebuff.put( rbuff, 0, readLen );
                }
                bytebuff.rewind();

                String curMd5 = getMd5( bytebuff );

                if ( preMd5 == "" ) {
                    preMd5 = curMd5;
                } else {
                    if ( !preMd5.equals( curMd5 ) ) {
                        Assert.fail( "the loblist is different." + "preMd5="
                                + preMd5 + "  curMd5=" + curMd5 );
                    }
                }
            } catch ( BaseException e ) {
                Assert.fail( "the lob different on the group node: "
                        + e.getErrorCode() + e.getErrorType() );
            }

        }
    }

    private String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";

        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
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
}