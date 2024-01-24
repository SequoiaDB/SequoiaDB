package com.sequoiadb.faulttolerance.restartnode;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-22402熔断模式下节点故障，指定replSize为0/-1/3
 *              seqDB-22403半容错模式下节点故障，指定replSize为0/-1/3
 * @author wuyan
 * @Date 2020.07.08
 * @version 1.00
 */
public class Faulttolerance22402_22403 extends SdbTestBase {
    @DataProvider(name = "ftlevel")
    public Object[][] configs() {
        return new Object[][] {
                // ftlevel
                { 1 }, { 2 }, };
    }

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String csName = "cs22402";
    private String clName = "cl22402";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int[] replSizes = { 0, 1, -1, 3 };

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
    }

    @Test(dataProvider = "ftlevel")
    public void test( int ftlevel ) throws Exception {
        updateConf( ftlevel );
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper slave = dataGroup.getSlave();

        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( slave, 1,
                10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        mgr.addTask( new InsertFailTask( clName + "_0" ) );
        mgr.addTask( new InsertFailTask( clName + "_3" ) );
        mgr.addTask( new InsertSuccessTask( clName + "_1" ) );
        mgr.addTask( new InsertSuccessTask( clName + "_2" ) );

        mgr.execute();
        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
        checkInsertResult( csName, clName );
        removeData( csName, clName );
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

    private class InsertFailTask extends OperateTask {
        String clName;

        public InsertFailTask( String clName ) {
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                insertData( cl );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -105 && e.getErrorCode() != -252 ) {
                    throw e;
                }
            }
        }
    }

    private class InsertSuccessTask extends OperateTask {
        String clName;

        public InsertSuccessTask( String clName ) {
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
            if ( i == 1 || i == 2 ) {
                Assert.assertEquals( count, expCount,
                        "the cl " + clName + "_" + i + " data num is error!" );
            } else {
                if ( count >= expCount ) {
                    Assert.fail( "the cl " + clName + "_" + i
                            + " data num is error! act num is " + count );
                }
            }

            // insert again
            BSONObject record = new BasicBSONObject();
            record.put( "a", "text22402_" + i );
            dbcl.insert( record );
            long count1 = dbcl.getCount( record );
            Assert.assertEquals( count1, 1, "the cl " + clName + "_" + i
                    + " insert context is:" + record );

        }

    }

    private void removeData( String csName, String clName ) {
        for ( int i = 0; i < replSizes.length; i++ ) {
            DBCollection dbcl = sdb.getCollectionSpace( csName )
                    .getCollection( clName + "_" + i );
            dbcl.delete( "" );
            long count = dbcl.getCount();
            Assert.assertEquals( count, 0 );
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
