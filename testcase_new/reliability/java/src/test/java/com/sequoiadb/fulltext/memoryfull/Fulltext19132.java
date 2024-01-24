package com.sequoiadb.fulltext.memoryfull;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.faultmodule.fault.FaultName;
import com.sequoiadb.faultmodule.task.FaultTask;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;

/**
 * 
 * @description seqDB-19132:全文检索记录时备节点适配内存不足
 * @author yinzhen
 * @date 2019年9月5日
 */
public class Fulltext19132 extends SdbTestBase {
    private String clName = "cl19132";
    private String fulltextName = "idx19132";
    private String groupName;
    private Sequoiadb sdb;
    private GroupMgr groupMgr;
    private DBCollection cl;

    @BeforeClass()
    public void setUp() throws Exception {
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

        groupName = CommLib.getDataGroupNames( sdb ).get( 0 );
        cl = sdb.getCollectionSpace( SdbTestBase.csName ).createCollection(
                clName,
                ( BSONObject ) JSON.parse( "{Group:'" + groupName + "'}" ) );
        cl.createIndex( fulltextName, "{'a':'text', 'b':'text', 'c':'text'}",
                false, false );
        FullTextDBUtils.insertData( cl, 10000 );
        Assert.assertTrue(
                FullTextUtils.isIndexCreated( cl, fulltextName, 10000 ) );
    }

    @Test
    public void test() throws Exception {
        sdb.setSessionAttr(
                ( BSONObject ) JSON.parse( "{'PreferedInstance':'S'}" ) );
        FaultTask task = FaultTask.getFault( FaultName.MEMORYLIMIT );
        try {
            Node node = sdb.getReplicaGroup( groupName ).getMaster();
            List< String > nAddrs = CommLib.getNodeAddress( sdb, groupName );
            for ( String addr : nAddrs ) {
                String svcName = addr.split( ":" )[ 1 ];
                if ( svcName.equals( String.valueOf( node.getPort() ) ) ) {
                    continue;
                } else {
                    svcName = svcName.substring( 0, svcName.length() - 1 )
                            + "7";
                    task.make( node.getHostName(), svcName, "root",
                            SdbTestBase.rootPwd );
                }
            }
            Assert.assertTrue(
                    FullTextUtils.isRecordEqualsByMulQueryMode( cl ) );
        } finally {
            task.restore();
        }

        FullTextDBUtils.insertData( cl, 1000 );
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, fulltextName,
                ( int ) cl.getCount() ) );
        Assert.assertTrue( FullTextUtils.checkAdapter() );
    }

    @AfterClass
    public void tearDown() {
        try {
            sdb.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( clName );
        } finally {
            sdb.close();
        }
    }
}
