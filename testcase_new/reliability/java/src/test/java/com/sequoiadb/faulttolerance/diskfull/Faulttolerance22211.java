package com.sequoiadb.faulttolerance.diskfull;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.DiskFull;
import com.sequoiadb.faulttolerance.FaultToleranceUtils;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22211 主节点磁盘满，备节点一段时间后lsn跟上主节点
 * @author luweikang
 * @date 2020年6月5日
 */
public class Faulttolerance22211 extends SdbTestBase {

    private String csName = "cs22211";
    private String clName = "cl22211";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private DBCollection cl = null;

    @BeforeClass
    public void setUp() throws ReliabilityException {

        groupMgr = GroupMgr.getInstance();

        // CheckBusiness(true),检测当前集群环境，若存在异常返回false，
        if ( !groupMgr.checkBusinessWithLSN( 20 ) ) {
            throw new SkipException( "checkBusinessWithLSN return false" );
        }

        groupName = groupMgr.getAllDataGroupName().get( 0 );

        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }

        cs = sdb.createCollectionSpace( csName );
        cl = cs.createCollection( clName,
                new BasicBSONObject( "Group", groupName ) );

        BSONObject configs = new BasicBSONObject();
        configs.put( "ftlevel", 3 );
        configs.put( "ftconfirmperiod", 3 );
        configs.put( "ftfusingtimeout", 10 );
        sdb.updateConfig( configs );

    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper dataMaster = dataGroup.getMaster();

        TaskMgr mgr = new TaskMgr();
        DiskFull diskFull = new DiskFull( dataMaster.hostName(),
                SdbTestBase.reservedDir, 100 );

        for ( int i = 0; i < 10; i++ ) {
            mgr.addTask( new insert() );
        }

        diskFull.init();
        diskFull.make();
        diskFull.checkMakeResult();

        mgr.execute();

        Assert.assertTrue( mgr.isAllSuccess(), mgr.getErrorMsg() );

        for ( int i = 0; i < 100; i++ ) {
            try {
                Thread.sleep( 100 );
                cl.insert(
                        "{a: 1, b:'hssdkfjldskjfkdjfhsjkddsdfjlsdkjflksjdflkjdslf"
                                + "sklfhsdksdfjdslkfjlksdjflksjdflksjfslksdfdjlfh"
                                + "kjdshfkjdhsfhfjsdafjdslfjlkdsjflkasdsadsadsdsf"
                                + "dfjkwerpnmpqnwerpqiwneioqqueiwqheasdhiuasdhf'}" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -11 ) {
                    throw e;
                } else {
                    if ( i == 99 ) {
                        Assert.fail( "10 seconds still not executed." );
                    }
                }
            }
        }

        String ftStatus = FaultToleranceUtils.getNodeFTStatus( sdb,
                dataMaster.hostName() + ":" + dataMaster.svcName() );
        if ( !ftStatus.equals( "NOSPC" )
                && !ftStatus.equals( "NOSPC|DEADSYNC" ) ) {
            Assert.fail(
                    "check node FTStatus error, exp: NOSPC, act: " + ftStatus );
        }
        String actNodeName = dataGroup.getMaster().hostName() + ":"
                + dataGroup.getMaster().svcName();
        String expNodeName = dataMaster.hostName() + ":" + dataMaster.svcName();
        Assert.assertNotEquals( actNodeName, expNodeName );

        diskFull.restore();
        diskFull.checkRestoreResult();
        diskFull.fini();

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 120 ) );
    }

    @AfterClass
    public void tearDown() {
        try {
            BSONObject configs = new BasicBSONObject();
            configs.put( "ftlevel", 1 );
            configs.put( "ftconfirmperiod", 1 );
            sdb.deleteConfig( configs, new BasicBSONObject() );
            sdb.updateConfig( new BasicBSONObject( "ftfusingtimeout", 300 ) );
            sdb.dropCollectionSpace( csName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }

        }
    }

    class insert extends OperateTask {

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 10000; i++ ) {
                    ArrayList< BSONObject > records = new ArrayList<>();
                    for ( int j = 0; j < 100; j++ ) {
                        BSONObject record = new BasicBSONObject();
                        record.put( "a", j );
                        record.put( "b", j );
                        record.put( "order", j );
                        record.put( "str",
                                "fjsldkfjlksdjflsdljfhjdshfjksdhfsdfhsdjkfhjkdshfj"
                                        + "kdshfkjdshfkjsdhfkjshafdkhasdikuhsdjfls"
                                        + "hsdjkfhjskdhfkjsdhfjkdshfjkdshfkjhsdjkf"
                                        + "hsdkjfhsdsafnweuhfuiwnqefiuokdjf" );
                        records.add( record );
                    }
                    cl.insert( records );
                }
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -11 ) {
                    throw e;
                }
            }
        }
    }

}
