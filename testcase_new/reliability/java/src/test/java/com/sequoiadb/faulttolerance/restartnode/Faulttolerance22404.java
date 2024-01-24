package com.sequoiadb.faulttolerance.restartnode;

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
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22404:: 熔断模式下节点故障，指定replSize为0/-1/3
 * @author wuyan
 * @Date 2020.07.08
 * @version 1.00
 */
public class Faulttolerance22404 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String csName = "cs22404";
    private String clName = "cl22404";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int[] replSizes = { 0, 1, -1, 3 };
    private int ftlevel = 3;

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

        BSONObject option = new BasicBSONObject();
        option.put( "Group", groupName );
        for ( int i = 0; i < replSizes.length; i++ ) {
            option.put( "ReplSize", replSizes[ i ] );
            cs.createCollection( clName + "_" + i, option );
        }
        updateConf( ftlevel );
    }

    @Test
    public void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper slave = dataGroup.getSlave();

        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( slave, 1,
                10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        for ( int i = 0; i < replSizes.length; i++ ) {
            mgr.addTask( new InsertTask( clName + "_" + i ) );
        }

        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
        checkInsertResult( csName, clName );

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

    private class InsertTask extends OperateTask {
        String clName;

        public InsertTask( String clName ) {
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl );
            }
        }
    }

    private void insertData( DBCollection cl ) {
        for ( int i = 0; i < 100; i++ ) {
            ArrayList< BSONObject > records = new ArrayList<>();
            for ( int j = 0; j < 500; j++ ) {
                BSONObject record = new BasicBSONObject();
                record.put( "a", j );
                record.put( "b", j );
                record.put( "test", j );
                record.put( "teststr",
                        "fjsldkfjlksdjflsdljfhjdshfjksdhfsdfhsdjkfhjkdshfj"
                                + "kdshfkjdshfkjsdhfkjshafdkhasdikuhsdjfls"
                                + "hsdjkfhjskdhfkjsdhfjkdshfjkdshfkjhsdjkf"
                                + "aaaaaaaa" );
                records.add( record );
            }
            cl.insert( records );
        }
    }

    private void checkInsertResult( String csName, String clName ) {
        for ( int i = 0; i < replSizes.length; i++ ) {
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName + "_" + i );
            long count = dbcl.getCount();
            long expCount = 50000;
            Assert.assertEquals( count, expCount,
                    "the cl " + clName + "_" + i + " data num is error!" );
        }

    }

    private void updateConf( int ftlevel ) {
        BSONObject configs = new BasicBSONObject();
        BSONObject options = new BasicBSONObject();
        configs.put( "ftlevel", ftlevel );
        configs.put( "ftfusingtimeout", 10 );
        options.put( "GroupName", groupName );
        sdb.updateConfig( configs, options );
    }

    private void deleteConf( String groupName ) {
        BSONObject configs = new BasicBSONObject();
        BSONObject options = new BasicBSONObject();
        configs.put( "ftlevel", 1 );
        options.put( "GroupName", groupName );
        sdb.deleteConfig( configs, options );
        sdb.updateConfig( new BasicBSONObject( "ftfusingtimeout", 300 ),
                options );
    }

}
