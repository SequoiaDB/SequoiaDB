package com.sequoiadb.datasync.restartnode;

import com.sequoiadb.base.*;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasync.Utils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.*;

/**
 * @FileName seqDB-3195: 创建索引过程中备节点节点正常重启，该备节点为同步的源节点 seqDB-3204:
 *           创建索引过程中备节点节点正常重启，该备节点为同步的目的节点
 * @Author linsuqiang
 * @Date 2017-03-29
 * @Version 1.00
 */

/*
 * 1.创建CS，CL 2.指定所有选项批量创建索引 3.过程中构造 bin/sdbstop -p port && bin/sdbstart -c
 * conf/local/port 4.继续创建 5.过程中恢复 注：ReplSize =
 * 2,随机断一个备节点时，该节点有可能是同步的源节点，也有可能是同步的目的节点。
 */

public class CreateIndex3195 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String clName = "cl_3195";
    private String clGroupName = null;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {

            groupMgr = GroupMgr.getInstance();
            if ( !groupMgr.checkBusiness() ) {
                throw new SkipException( "checkBusiness failed" );
            }

            db = new Sequoiadb( coordUrl, "", "" );
            clGroupName = groupMgr.getAllDataGroupName().get( 0 );
            createCL( db );
            insertData( db );
        } catch ( ReliabilityException e ) {
            Assert.fail( this.getClass().getName()
                    + " setUp error, error description:" + e.getMessage()
                    + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @Test(enabled = false)
    public void test() {
        Sequoiadb db = null;
        try {
            GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
            NodeWrapper slvNode = dataGroup.getSlave();

            FaultMakeTask faultTask = NodeRestart.getFaultMakeTask( slvNode, 1,
                    10 );
            TaskMgr mgr = new TaskMgr( faultTask );
            CreateIdxTask cTask = new CreateIdxTask();
            mgr.addTask( cTask );
            mgr.execute();
            Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );

            if ( !groupMgr.checkBusinessWithLSN( 600 ) ) {
                Assert.fail( "checkBusinessWithLSN() occurs timeout" );
            }

            checkConsistency( dataGroup );
            checkExplain( dataGroup );
            runSuccess = true;
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            Assert.fail( e.getMessage() );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if ( !runSuccess ) {
            throw new SkipException( "to save environment" );
        }
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            CollectionSpace commCS = db.getCollectionSpace( csName );
            commCS.dropCollection( clName );
        } catch ( BaseException e ) {
            Assert.fail(
                    e.getMessage() + "\r\n" + Utils.getKeyStack( e, this ) );
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
    }

    private void createCL( Sequoiadb db ) {
        CollectionSpace commCS = db.getCollectionSpace( csName );
        BSONObject option = ( BSONObject ) JSON
                .parse( "{ Group: '" + clGroupName + "', ReplSize: 2 }" );
        commCS.createCollection( clName, option );
    }

    private void insertData( Sequoiadb db ) {
        DBCollection cl = db.getCollectionSpace( csName )
                .getCollection( clName );
        List< BSONObject > recs = new ArrayList< BSONObject >();
        int total = 10000;
        for ( int i = 0; i < total; i++ ) {
            BSONObject rec = ( BSONObject ) JSON
                    .parse( "{ a" + i + ": " + i + " }" );
            recs.add( rec );
        }
        cl.insert( recs, DBCollection.FLG_INSERT_CONTONDUP );
    }

    private class CreateIdxTask extends OperateTask {
        @Override
        public void exec() throws Exception {
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCollection cl = db.getCollectionSpace( csName )
                        .getCollection( clName );
                for ( int i = 0; i < 60; i++ ) {
                    String idxName = "idx_" + i;
                    BSONObject key = ( BSONObject ) JSON
                            .parse( "{ a" + i + ": 1 }" );
                    boolean isUnique = i % 2 == 0 ? true : false;
                    boolean enforced = false;
                    int sortBufferSize = i * 2;
                    cl.createIndex( idxName, key, isUnique, enforced,
                            sortBufferSize );
                }
            } catch ( BaseException e ) {
                e.printStackTrace();
                throw e;
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
        }
    }

    private void checkConsistency( GroupWrapper dataGroup ) {
        String lastCompareInfo = "";
        List< String > clNames = new ArrayList< String >();
        clNames.add( clName );

        if ( !Utils.checkIndexConsistency( dataGroup, csName, clNames,
                lastCompareInfo ) ) {
            System.out.println( lastCompareInfo );
            Assert.fail( "data is different. see the detail in console" );
        }
    }

    private void checkExplain( GroupWrapper dataGroup ) {
        List< String > dataUrls = dataGroup.getAllUrls();
        for ( String dataUrl : dataUrls ) {
            Sequoiadb dataDB = new Sequoiadb( dataUrl, "", "" );
            DBCollection cl = dataDB.getCollectionSpace( csName )
                    .getCollection( clName );
            List< String > idxNames = getIdxNames( cl );
            for ( String idxName : idxNames ) {
                if ( !isExplainOk( cl, idxName ) ) {
                    Assert.fail( idxName + " does not work" );
                }
            }
            dataDB.close();
        }
    }

    private boolean isExplainOk( DBCollection cl, String idxName ) {
        BSONObject hint = ( BSONObject ) JSON
                .parse( "{ '': '" + idxName + "' }" );
        BSONObject run = ( BSONObject ) JSON.parse( "{ Run: true }" );
        DBCursor cursor = cl.explain( null, null, null, hint, 0, -1,
                DBQuery.FLG_QUERY_FORCE_HINT, run );
        BSONObject plan = cursor.getNext();
        cursor.close();

        if ( !( plan.get( "ScanType" ) ).equals( "ixscan" )
                || !( plan.get( "IndexName" ) ).equals( idxName ) ) {
            System.out.println( "index: " + idxName );
            System.out.println( "explain:" + plan );
            return false;
        }
        return true;
    }

    private List< String > getIdxNames( DBCollection cl ) {
        DBCursor cursor = cl.getIndexes();
        List< String > idxNames = new ArrayList< String >();
        while ( cursor.hasNext() ) {
            String idxName = ( String ) ( ( BSONObject ) cursor.getNext()
                    .get( "IndexDef" ) ).get( "name" );
            idxNames.add( idxName );
        }
        cursor.close();
        return idxNames;
    }
}