package com.sequoiadb.crud.restartnode;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.crud.CRUDUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.NodeRestart;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;
import com.sequoiadb.transaction.common.TransUtil;

/**
 * @Description seqDB-1987:协调节点进程正常退出
 * @author zhangyanan
 * @Date 2021.07.15
 * @version 2.10
 */
public class CRUD1987 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private Sequoiadb sdb1 = null;
    private CollectionSpace cs = null;
    private String clName = "curdcl_1987";
    private DBCollection cl = null;
    private GroupMgr groupMgr = null;
    private String clGroupName = null;
    private ArrayList< BSONObject > allRecords = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > insertRecords1 = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > insertRecords2 = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > insertRecords3 = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > insertRecords4 = new ArrayList< BSONObject >();
    private String killCoordUrl = null;

    @BeforeClass
    public void setup() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        killCoordUrl = TransUtil.getCoordUrl( sdb );
        sdb1 = new Sequoiadb( killCoordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusinessWithLSN( 120 ) ) {
            throw new SkipException( "checkBusiness failed" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        clGroupName = groupMgr.getAllDataGroupName().get( 0 );
        BSONObject options = new BasicBSONObject();
        options.put( "Group", clGroupName );
        options.put( "ReplSize", -1 );
        BSONObject option1 = new BasicBSONObject();
        option1.put( "no", 1 );
        options.put( "ShardingKey", option1 );
        cl = cs.createCollection( clName, options );
    }

    @Test
    public void test() throws Exception {
        NodeWrapper nodeWrapper = TransUtil.getCoordNode( sdb1 );
        FaultMakeTask faultTask = NodeRestart.getFaultMakeTask( nodeWrapper, 0,
                1 );
        TaskMgr mgr = new TaskMgr( faultTask );

        int insertBeginNo1 = 0;
        int insertEndNo1 = 1000;
        threadInsert insert1 = new threadInsert( insertBeginNo1, insertEndNo1,
                insertRecords1 );
        mgr.addTask( insert1 );

        int insertBeginNo2 = 1000;
        int insertEndNo2 = 2000;
        threadInsert insert2 = new threadInsert( insertBeginNo2, insertEndNo2,
                insertRecords2 );
        mgr.addTask( insert2 );

        int insertBeginNo3 = 2000;
        int insertEndNo3 = 3000;
        threadInsert insert3 = new threadInsert( insertBeginNo3, insertEndNo3,
                insertRecords3 );
        mgr.addTask( insert3 );

        mgr.execute();

        Node coord = sdb.getReplicaGroup( "SYSCoord" ).getNode(
                nodeWrapper.hostName(),
                Integer.valueOf( nodeWrapper.svcName() ) );
        coord.start();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );

        int insertBeginNo4 = 3000;
        int insertEndNo4 = 4000;
        CRUDUtils.insertData( cl, ( insertEndNo4 - insertBeginNo4 ),
                insertBeginNo4, insertRecords4 );
        String insert2Matcher = "{$and:[{no:{$gte:" + insertBeginNo4
                + "}},{no:{$lt:" + insertEndNo4 + "}}]}";
        CRUDUtils.checkRecords( cl, insertRecords4, insert2Matcher );

        checkData( insertBeginNo1, insertEndNo1, insertRecords1 );
        checkData( insertBeginNo2, insertEndNo2, insertRecords2 );
        checkData( insertBeginNo3, insertEndNo3, insertRecords3 );
        allRecords.addAll( insertRecords1 );
        allRecords.addAll( insertRecords2 );
        allRecords.addAll( insertRecords3 );
        allRecords.addAll( insertRecords4 );
        Collections.sort( allRecords, new OrderBy() );
        CRUDUtils.checkRecords( cl, allRecords, "" );
    }

    @AfterClass
    public void tearDown() {
        try {
            if ( cs.isCollectionExist( clName ) )
                cs.dropCollection( clName );

        } finally {
            sdb.close();
        }
    }

    public void checkData( int beginNo, int endNo,
            ArrayList< BSONObject > insertRecords ) {
        ArrayList< BSONObject > clRecords = new ArrayList< BSONObject >();
        String matcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lt:" + endNo
                + "}}]}";
        DBCursor cursor = cl.query( matcher, "", "{'no':1}", "" );
        while ( cursor.hasNext() ) {
            clRecords.add( cursor.getNext() );
        }
        cursor.close();
        // 取交集比较线程插入的数据
        Collections.sort( clRecords, new OrderBy() );
        if ( insertRecords.size() != clRecords.size() ) {
            insertRecords.retainAll( clRecords );
            Assert.assertEquals( insertRecords, clRecords );
        } else {
            Assert.assertEquals( insertRecords, clRecords );
        }
    }

    public class OrderBy implements Comparator< BSONObject > {
        public int compare( BSONObject obj1, BSONObject obj2 ) {
            int flag = 0;
            int no1 = ( int ) obj1.get( "no" );
            int no2 = ( int ) obj2.get( "no" );
            if ( no1 > no2 ) {
                flag = 1;
            } else if ( no1 < no2 ) {
                flag = -1;
            }
            return flag;
        }
    }

    private class threadInsert extends OperateTask {
        private int beginNo;
        private int endNo;
        private ArrayList< BSONObject > insertRecords;

        private threadInsert( int beginNo, int endNo,
                ArrayList< BSONObject > insertRecords ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
            this.insertRecords = insertRecords;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( killCoordUrl, "", "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                int recordNum = endNo - beginNo;
                CRUDUtils.insertData( cl, recordNum, beginNo, insertRecords );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_QUIESCED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NODE_BSFAULT
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }
}
