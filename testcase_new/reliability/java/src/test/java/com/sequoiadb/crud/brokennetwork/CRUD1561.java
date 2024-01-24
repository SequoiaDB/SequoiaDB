package com.sequoiadb.crud.brokennetwork;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.crud.CRUDUtils;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-1561:协调节点与数据主节点的网络闪断
 * @author zhangyanan
 * @Date 2021.07.15
 * @version 2.10
 */
public class CRUD1561 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "curdcl_1561";
    private DBCollection cl = null;
    private GroupMgr groupMgr = null;
    private String clGroupName = null;
    private static ArrayList< BSONObject > allRecords = new ArrayList< BSONObject >();

    @BeforeClass
    public void setup() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusinessWithLSN() ) {
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
        ArrayList< BSONObject > insertRecords1 = new ArrayList< BSONObject >();
        ArrayList< BSONObject > insertRecords2 = new ArrayList< BSONObject >();
        ArrayList< BSONObject > insertRecords3 = new ArrayList< BSONObject >();
        GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
        String dataHost = dataGroup.getMaster().hostName();

        FaultMakeTask faultTask = BrokenNetwork.getFaultMakeTask( dataHost, 0,
                10 );
        TaskMgr mgr = new TaskMgr( faultTask );

        int insertBeginNo1 = 0;
        int insertEndNo1 = 20000;
        threadInsert insert1 = new threadInsert( insertBeginNo1, insertEndNo1,
                insertRecords1 );
        mgr.addTask( insert1 );

        int insertBeginNo2 = 20000;
        int insertEndNo2 = 40000;
        threadInsert insert2 = new threadInsert( insertBeginNo2, insertEndNo2,
                insertRecords2 );
        mgr.addTask( insert2 );

        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );

        int insertBeginNo3 = 40000;
        int insertEndNo3 = 50000;
        insertData( cl, insertBeginNo3, insertEndNo3, insertRecords3 );
        // 校验插入线程1数据
        checkData( insertBeginNo1, insertEndNo1, insertRecords1 );
        // 校验插入线程2数据
        checkData( insertBeginNo2, insertEndNo2, insertRecords2 );

        String matcher = "{$and:[{no:{$gte:" + insertBeginNo3 + "}},{no:{$lt:"
                + insertEndNo3 + "}}]}";
        // 校验节点异常恢复后插入的数据
        CRUDUtils.checkRecords( cl, insertRecords3, matcher );
        // 校验数据是否在插入数据的范围内
        matcher = "{no:{$gt:" + insertEndNo3 + "}}";
        ArrayList< BSONObject > actAllRecords = new ArrayList< BSONObject >();
        DBCursor actcursor = cl.query( matcher, "", "{'no':1}", "" );
        while ( actcursor.hasNext() ) {
            actAllRecords.add( actcursor.getNext() );
        }
        actcursor.close();
        if ( actAllRecords.size() != 0 ) {
            Assert.fail(
                    "the expRecords not containsAll actRecords! actAllRecords = "
                            + actAllRecords );
        }
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
        Collections.sort( clRecords, new CRUDUtils.OrderBy("no") );
        if ( clRecords.size() > 10000 ) {
            List< BSONObject > expClRecords = insertRecords.subList( 0, 10000 );
            List< BSONObject > actClRecords = clRecords.subList( 0, 10000 );
            Assert.assertEquals( actClRecords, expClRecords );
            List< BSONObject > expClRecords1 = clRecords.subList( 10000,
                    clRecords.size() );
            if ( !insertRecords.containsAll( expClRecords1 ) ) {
                Assert.fail( "the expdata not containsAll actdate " );
            }
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
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                insertData( cl, beginNo, endNo, insertRecords );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NETWORK.getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_COORD_REMOTE_DISC
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NOT_PRIMARY
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_WAIT_SYNC_FAILED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_UPDATE_CAT_FAILED
                                .getErrorCode()
                        && e.getErrorCode() != SDBError.SDB_CLS_NODE_NOT_ENOUGH
                                .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void insertData( DBCollection dbcl, int beginNo, int endNo,
            ArrayList< BSONObject > insertRecords ) {
        int batchNum = 10000;
        int recordNum = endNo - beginNo;
        if ( recordNum < batchNum ) {
            batchNum = recordNum;
        }
        for ( int i = 0; i < recordNum / batchNum; i++ ) {
            List< BSONObject > batchRecords = new ArrayList< BSONObject >();
            for ( int j = 0; j < batchNum; j++ ) {
                int value = beginNo++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "testa", value );
                obj.put( "no", value );
                obj.put( "num", value );
                batchRecords.add( obj );
            }
            insertRecords.addAll( batchRecords );
            allRecords.addAll( batchRecords );
            dbcl.insert( batchRecords );
            batchRecords.clear();
        }
    }

}
