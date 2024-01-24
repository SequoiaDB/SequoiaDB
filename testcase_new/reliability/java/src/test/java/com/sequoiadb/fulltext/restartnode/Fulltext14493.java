package com.sequoiadb.fulltext.restartnode;

import java.util.ArrayList;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.fulltext.FullTextDBUtils;
import com.sequoiadb.fulltext.FullTextUtils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @FileName seqDB-14493：删除集合时主节点正常重启
 * @Author zhaoyu
 * @Date 2019-08-06
 */

public class Fulltext14493 extends SdbTestBase {

    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String csName = "cs14493";
    private String clName = "cl14493";
    private String indexName = "index14493";
    private GroupMgr groupMgr = null;
    private String groupName = null;
    private int insertNum = 10000;
    private int clNum = 30;
    private List< String > cappedNames = new ArrayList< String >();
    private List< String > esIndexNames = new ArrayList< String >();

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
        if ( !FullTextUtils.checkAdapter() ) {
            throw new SkipException( "Check adapter failed" );
        }
        if ( sdb.isCollectionSpaceExist( csName ) ) {
            sdb.dropCollectionSpace( csName );
        }
        cs = sdb.createCollectionSpace( csName );
        for ( int i = 0; i < clNum; i++ ) {
            DBCollection cl = cs.createCollection( clName + "_" + i,
                    ( BSONObject ) JSON
                            .parse( "{Group:'" + groupName + "'}" ) );
            FullTextDBUtils.insertData( cl, insertNum );
            cl.createIndex( indexName, "{a:'text',b:'text',c:'text',d:'text'}",
                    false, false );
            String cappedName = FullTextDBUtils.getCappedName( cl, indexName );
            cappedNames.add( cappedName );
            String esIndexName = FullTextDBUtils.getESIndexName( cl,
                    indexName );
            esIndexNames.add( esIndexName );
        }
    }

    @Test
    public void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( groupName );
        NodeWrapper master = dataGroup.getMaster();

        FaultMakeTask faultMakeTask = NodeRestart.getFaultMakeTask( master, 1,
                10 );
        TaskMgr mgr = new TaskMgr( faultMakeTask );
        mgr.addTask( new DropCLThread() );
        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );
        Assert.assertEquals( dataGroup.checkInspect( 1 ), true );
        Assert.assertEquals( FullTextUtils.checkAdapter(), true );

        for ( int i = 0; i < clNum; i++ ) {
            if ( cs.isCollectionExist( clName + "_" + i ) ) {
                DBCollection cl = cs.getCollection( clName + "_" + i );
                cl.insert( "{a:'text14493'}" );
                Assert.assertTrue( FullTextUtils.isIndexCreated( cl, indexName,
                        insertNum + 1 ) );
            } else {
                Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb,
                        esIndexNames.get( i ), cappedNames.get( i ) ) );
            }
        }

    }

    @AfterClass
    public void tearDown() throws Exception {
        try {
            sdb.dropCollectionSpace( csName );
            for ( int i = 0; i < clNum; i++ ) {
                Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb,
                        esIndexNames.get( i ), cappedNames.get( i ) ) );
            }

        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private class DropCLThread extends OperateTask {
        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" )) {
                CollectionSpace cs = db.getCollectionSpace( csName );
                for ( int i = 0; i < clNum; i++ ) {
                    cs.dropCollection( clName + "_" + i );
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
            }
        }
    }

}
