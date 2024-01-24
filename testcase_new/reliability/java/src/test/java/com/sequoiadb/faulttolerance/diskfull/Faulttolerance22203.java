package com.sequoiadb.faulttolerance.diskfull;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.faulttolerance.FaultToleranceUtils;
import com.sequoiadb.lob.LobUtil;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22203::
 *              容错级别为全容错，1个副本状态为:NOSPC或者DEADSYNC，不同replSize的集合中插入数据
 * @author wuyan
 * @Date 2020.06.09
 * @version 1.00
 */
public class Faulttolerance22203 extends SdbTestBase {
    @DataProvider(name = "ftmasks")
    public Object[][] configs() {
        return new Object[][] {
                // ftmask
                { "NOSPC" }, { "DEADSYNC" }, };
    }

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String csName = "cs22203";
    private String clName = "cl22203";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int[] replSizes = { 0, -1, 1, 2 };
    private byte[] lobBuff = LobUtil.getRandomBytes( 1024 * 1024 );

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "StandAlone environment!" );
        }
        groupMgr = GroupMgr.getInstance();
        groupName = groupMgr.getAllDataGroupName().get( 0 );
        if ( !groupMgr.checkBusiness() ) {
            throw new SkipException( "checkBusiness failed" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cs = sdb.createCollectionSpace( csName );

        for ( int i = 0; i < replSizes.length; i++ ) {
            BSONObject option = ( BSONObject ) JSON.parse( "{Group:'"
                    + groupName + "'," + "ReplSize:" + replSizes[ i ] + " }" );
            cs.createCollection( clName + "_" + i, option );
        }
    }

    @Test(dataProvider = "ftmasks")
    public void test( String ftmask ) throws Exception {
        updateConf( ftmask );
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper slave = dataGroup.getSlave();

        DiskFull diskFull = new DiskFull( slave.hostName(), slave.dbPath() );
        diskFull.init();
        diskFull.make();

        TaskMgr mgr = new TaskMgr();
        mgr.addTask( new PutLobs( slave, ftmask ) );
        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        FaultToleranceUtils.checkNodeStatus( slave.connect().toString(),
                ftmask );
        insertAndCheckResult( csName, clName );
        diskFull.restore();
        diskFull.fini();

        FaultToleranceUtils.checkNodeStatus( slave.connect().toString(), "" );
        insertAndCheckResult( csName, clName );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            sdb.dropCollectionSpace( csName );
        } finally {
            deleteConf( groupName );
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class PutLobs extends OperateTask {
        NodeWrapper nodes;
        String ftmask;

        public PutLobs( NodeWrapper nodes, String ftmask ) {
            this.nodes = nodes;
            this.ftmask = ftmask;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb sdb = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = sdb.getCollectionSpace( csName )
                        .getCollection( clName + "_1" );
                String nodeName = nodes.hostName() + ":" + nodes.svcName();
                for ( int i = 0; i < 1000; i++ ) {
                    String ftStatus = FaultToleranceUtils.getNodeFTStatus( sdb,
                            nodeName );
                    if ( ftStatus.equals( ftmask ) ) {
                        break;
                    }
                    DBLob lob = cl.createLob();
                    lob.write( lobBuff );
                    lob.close();
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -105 && e.getErrorCode() != -252 ) {
                    throw e;
                }
            }
        }
    }

    private void insertAndCheckResult( String csName, String clName ) {
        for ( int i = 0; i < replSizes.length; i++ ) {
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName + "_" + i );
            BSONObject record = new BasicBSONObject();
            record.put( "a", "text22203_" + i );
            dbcl.insert( record );
            long count = dbcl.getCount( record );
            Assert.assertEquals( count, 1, "the cl " + clName + "_" + i
                    + " insert context is:" + record );

            ObjectId oid = null;
            try ( DBLob lob = dbcl.createLob()) {
                lob.write( lobBuff );
                oid = lob.getID();
                lob.close();
            }
            try ( DBLob rLob = dbcl.openLob( oid )) {
                byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
                rLob.read( rbuff );
                Assert.assertEquals( rbuff, lobBuff );
            }
        }

    }

    private void updateConf( String ftmask ) {
        BSONObject configs = new BasicBSONObject();
        BSONObject options = new BasicBSONObject();
        configs.put( "ftmask", ftmask );
        configs.put( "ftlevel", 3 );
        configs.put( "ftfusingtimeout", 10 );
        options.put( "GroupName", groupName );
        sdb.updateConfig( configs, options );
    }

    private void deleteConf( String groupName ) {
        BSONObject configs = new BasicBSONObject();
        BSONObject options = new BasicBSONObject();
        configs.put( "ftmask", 1 );
        configs.put( "ftlevel", 1 );
        options.put( "GroupName", groupName );
        sdb.deleteConfig( configs, options );
        sdb.updateConfig( new BasicBSONObject( "ftfusingtimeout", 300 ),
                options );
    }

}
