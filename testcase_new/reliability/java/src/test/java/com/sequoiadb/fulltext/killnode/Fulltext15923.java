package com.sequoiadb.fulltext.killnode;

import java.util.List;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.commlib.Ssh;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-15923: cs下存在1个cl，全量同步clean cs阶段清理无效集合对应的全文索引processor
 * @date 2019/8/13
 */
public class Fulltext15923 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private DBCollection cl = null;
    private String groupName = "";
    private String clName = "cl_15923";
    private CollectionSpace cs = null;
    private String indexName = "fullTextIndex_15923";

    @BeforeClass()
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "isStandAlone() TRUE, STANDALONE MODE" );
        }
        if ( !groupMgr.checkBusiness( 120 ) ) {
            throw new SkipException( "checkBusiness() FAIL, GROUP ERROR" );
        }
        if ( !FullTextUtils.checkAdapter() ) {
            throw new SkipException( "Check adapter failed" );
        }
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        groupName = groupNames.get( 0 );
        cs = sdb.getCollectionSpace( csName );
        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group: '" + groupName + "'}" ) );
        cl.createIndex( indexName, "{a:'text'}", false, false );
    }

    @Test()
    public void Test() throws Exception {
        Node slave = sdb.getReplicaGroup( groupName ).getSlave();
        String remoteHostName = slave.getHostName();
        Ssh ssh = new Ssh( remoteHostName, "root", SdbTestBase.rootPwd );
        String installDir = ssh.getSdbInstallDir();
        String command = installDir + "/bin/sdbcmtop";
        ssh.exec( command );

        TaskMgr taskMgr1 = new TaskMgr();
        taskMgr1.addTask( new InsertTask( clName ) );
        taskMgr1.start();

        TaskMgr taskMgr2 = new TaskMgr();
        taskMgr2.addTask( new KillNodeTask( slave, ssh ) );
        long count = 0;
        long num = 100000;
        while ( count < num ) {
            count = cl.getCount();
            Thread.sleep( 1000 );
        }
        taskMgr2.start();
        taskMgr2.join();
        taskMgr1.join();
        taskMgr1.fini();
        taskMgr2.fini();

        Assert.assertTrue( taskMgr1.isAllSuccess(), taskMgr1.getErrorMsg() );
        Assert.assertTrue( taskMgr2.isAllSuccess(), taskMgr2.getErrorMsg() );
        Assert.assertTrue( FullTextUtils.checkAdapter() );

        FullTextDBUtils.dropCollection( cs, clName );

        cl = cs.createCollection( clName,
                ( BSONObject ) JSON.parse( "{Group: '" + groupName + "'}" ) );
        cl.createIndex( indexName, "{a:'text'}", false, false );
        cl.insert( "{a : 'Only one record'}" );

        command = "service sdbcm start";
        ssh.exec( command );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, indexName, 1 ) );
    }

    private class InsertTask extends OperateTask {
        private Sequoiadb db = null;
        private DBCollection cl = null;
        private String clName = null;

        public InsertTask( String clName ) {
            this.clName = clName;
        }

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            cl = db.getCollectionSpace( csName ).getCollection( clName );
            for ( int i = 0; i < 20; i++ ) {
                FullTextDBUtils.insertData( cl, 10000 );
            }
        }
    }

    private class KillNodeTask extends OperateTask {
        private Node slave = null;
        private Ssh ssh = null;

        public KillNodeTask( Node slave, Ssh ssh ) {
            // TODO Auto-generated constructor stub
            this.slave = slave;
            this.ssh = ssh;
        }

        @Override
        public void exec() throws Exception {
            // TODO Auto-generated method stub
            int remotePort = slave.getPort();
            String command = "lsof -i TCP:" + remotePort
                    + " -sTCP:LISTEN| sed '1d' | awk '{print $2}'";
            ssh.exec( command );
            String pid = ssh.getStdout().substring( 0,
                    ssh.getStdout().length() - 1 );
            command = "kill -9 " + pid;
            ssh.exec( command );
        }
    }

    @AfterClass()
    public void tearDown() {
        FullTextDBUtils.dropCollection( cs, csName );
        sdb.close();
    }
}
