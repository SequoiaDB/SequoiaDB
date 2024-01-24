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

/**
 * @Description seqDB-15926:cs下存在多个cl均有效，全量同步clean cs节点清理无效集合对应的全文索引processor
 * @author zhaoxiaoni
 * @date 2019/8/13
 */
public class Fulltext15926 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private GroupMgr groupMgr = null;
    private String groupName = "";
    private CollectionSpace cs = null;

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
        cs = sdb.getCollectionSpace( csName );
        groupName = groupNames.get( 0 );
        for ( int i = 0; i < 10; i++ ) {
            DBCollection cl = cs.createCollection( "cl_15926_" + i,
                    ( BSONObject ) JSON
                            .parse( "{Group: '" + groupName + "'}" ) );
            cl.createIndex( "indexName_15926_" + i, "{a:'text'}", false,
                    false );
        }
    }

    @Test()
    public void Test() throws Exception {
        Node slave = sdb.getReplicaGroup( groupName ).getSlave();
        String remoteHostName = slave.getHostName();
        Ssh ssh = new Ssh( remoteHostName, "root", SdbTestBase.rootPwd );
        String installDir = ssh.getSdbInstallDir();
        String command = installDir + "/bin/sdbcmtop";
        ssh.exec( command );

        for ( int i = 0; i < 10; i++ ) {
            DBCollection cl = cs.getCollection( "cl_15926_" + i );
            FullTextDBUtils.insertData( cl, 10000 );
        }

        int remotePort = slave.getPort();
        command = "lsof -iTCP:" + remotePort
                + " -sTCP:LISTEN | sed '1d' | awk '{print $2}'";
        ssh.exec( command );
        String pid = ssh.getStdout().substring( 0,
                ssh.getStdout().length() - 1 );
        command = "kill -9 " + pid;
        ssh.exec( command );

        CollectionSpace cs = sdb.getCollectionSpace( csName );
        for ( int i = 0; i < 10; i++ ) {
            FullTextDBUtils.dropCollection( cs, "cl_15926_" + i );
        }

        for ( int i = 0; i < 10; i++ ) {
            DBCollection cl = cs.createCollection( "cl_15926_" + i,
                    ( BSONObject ) JSON
                            .parse( "{Group: '" + groupName + "'}" ) );
            cl.createIndex( "indexName_15926_" + i, "{a:'text'}", false,
                    false );
            cl.insert( "{a : 'Only one record'}" );
        }

        command = "service sdbcm start";
        ssh.exec( command );

        Assert.assertTrue( groupMgr.checkBusinessWithLSN( 600 ) );
        Assert.assertTrue( FullTextUtils.checkAdapter() );

        for ( int i = 0; i < 10; i++ ) {
            DBCollection cl = cs.getCollection( "cl_15926_" + i );
            Assert.assertTrue( FullTextUtils.isIndexCreated( cl,
                    "indexName_15926_" + i, 1 ) );
        }
    }

    @AfterClass()
    public void tearDown() {
        for ( int i = 0; i < 10; i++ ) {
            FullTextDBUtils.dropCollection( cs, "cl_15926_" + i );
        }
        sdb.close();
    }
}
