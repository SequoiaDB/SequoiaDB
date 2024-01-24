package com.sequoiadb.crud.killnode;

import java.util.ArrayList;
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
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.crud.CRUDUtils;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.OperateTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @Description seqDB-1554:数据操作过程中备节点进程异常退出
 * @author zhangyanan
 * @Date 2021.07.15
 * @version 2.10
 */
public class CRUD1554 extends SdbTestBase {
    private Sequoiadb sdb = null;
    private CollectionSpace cs = null;
    private String clName = "curdcl_1554";
    private DBCollection cl = null;
    private GroupMgr groupMgr = null;
    private String clGroupName = null;
    private ArrayList< BSONObject > allRecords = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > insertRecords1 = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > insertRecords2 = new ArrayList< BSONObject >();

    @BeforeClass
    public void setup() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( !groupMgr.checkBusinessWithLSN( 120 ) ) {
            throw new SkipException( "checkBusiness failed" );
        }
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        clGroupName = groupMgr.getAllDataGroupName().get( 0 );
        BSONObject options = new BasicBSONObject();
        options.put( "Group", clGroupName );
        BSONObject option1 = new BasicBSONObject();
        option1.put( "no", 1 );
        options.put( "ShardingKey", option1 );
        cl = cs.createCollection( clName, options );
        int dataNum = 4000;
        int beginNo = 0;
        CRUDUtils.insertData( cl, dataNum, beginNo, allRecords );
    }

    @Test
    public void test() throws Exception {
        GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
        NodeWrapper priNode = dataGroup.getSlave();
        FaultMakeTask faultTask = KillNode.getFaultMakeTask( priNode.hostName(),
                priNode.svcName(), 0 );
        TaskMgr mgr = new TaskMgr( faultTask );
        int insertBeginNo = 4000;
        int insertEndNo = 5000;
        threadInsert insert = new threadInsert( insertBeginNo, insertEndNo );
        mgr.addTask( insert );

        int deleteBeginNo = 3000;
        int deleteEndNo = 4000;
        threadDelete delete = new threadDelete( deleteBeginNo, deleteEndNo );
        mgr.addTask( delete );

        int updateBeginNo = 0;
        int updateEndNo = 1000;
        threadUpdate update = new threadUpdate( updateBeginNo, updateEndNo );
        mgr.addTask( update );

        int queryBeginNo = 1000;
        int queryEndNo = 2000;
        threadQuery query = new threadQuery( queryBeginNo, queryEndNo );
        mgr.addTask( query );
        mgr.execute();

        Assert.assertEquals( mgr.isAllSuccess(), true, mgr.getErrorMsg() );
        Assert.assertEquals( groupMgr.checkBusinessWithLSN( 600 ), true );

        expectedData( updateBeginNo, updateEndNo, deleteBeginNo, deleteEndNo );

        checkData( updateBeginNo, updateEndNo, deleteBeginNo, deleteEndNo,
                insertBeginNo, insertEndNo, queryBeginNo, queryEndNo );

        int insert2BeginNo = 5000;
        int insert2EndNo = 6000;

        CRUDUtils.insertData( cl, ( insert2EndNo - insert2BeginNo ),
                insert2BeginNo, insertRecords2 );
        String insert2Matcher = "{$and:[{no:{$gte:" + insert2BeginNo
                + "}},{no:{$lt:" + insert2EndNo + "}}]}";
        CRUDUtils.checkRecords( cl, insertRecords2, insert2Matcher );

        allRecords.addAll( insertRecords2 );
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

    private void expectedData( int updateBeginNo, int updateEndNo,
            int deleteBeginNo, int deleteEndNo ) {
        // 构造update的数据
        for ( int i = updateBeginNo; i < updateEndNo; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj = allRecords.get( i );
            obj.put( "testa", "updatetest" + i );
        }
        // 构造insert的数据
        allRecords.addAll( insertRecords1 );
        // 构造delete的数据
        List< BSONObject > sublist = allRecords.subList( deleteBeginNo,
                deleteEndNo );
        allRecords.removeAll( sublist );
    }

    private void checkData( int updateBeginNo, int updateEndNo,
            int deleteBeginNo, int deleteEndNo, int insertBeginNo,
            int insertEndNo, int queryBeginNo, int queryEndNo ) {
        // 校验插入线程数据
        String insertMatcher = "{$and:[{no:{$gte:" + insertBeginNo
                + "}},{no:{$lt:" + insertEndNo + "}}]}";
        CRUDUtils.checkRecords( cl, insertRecords1, insertMatcher );
        // 校验删除线程数据
        ArrayList< BSONObject > deleteSublist = new ArrayList< BSONObject >();
        String deleteMatcher = "{$and:[{no:{$gte:" + deleteBeginNo
                + "}},{no:{$lt:" + deleteEndNo + "}}]}";
        CRUDUtils.checkRecords( cl, deleteSublist, deleteMatcher );
        // 校验修改线程数据
        List< BSONObject > updateSublist = allRecords.subList( updateBeginNo,
                updateEndNo );
        String updateMatcher = "{$and:[{no:{$gte:" + updateBeginNo
                + "}},{no:{$lt:" + updateEndNo + "}}]}";
        CRUDUtils.checkRecords( cl, updateSublist, updateMatcher );
        // 校验查询线程数据
        List< BSONObject > querSublist = allRecords.subList( queryBeginNo,
                queryEndNo );
        String queryMatcher = "{$and:[{no:{$gte:" + queryBeginNo
                + "}},{no:{$lt:" + queryEndNo + "}}]}";
        CRUDUtils.checkRecords( cl, querSublist, queryMatcher );
    }

    public void insertData( DBCollection dbcl, int recordNum, int beginNo,
            ArrayList< BSONObject > insertRecord ) {
        for ( int i = beginNo; i < beginNo + recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", i );
            obj.put( "no", i );
            obj.put( "num", i );
            dbcl.insert( obj );
            insertRecord.add( obj );
        }

    }

    public void deleteData( DBCollection dbcl, int recordNum, int beginNo ) {
        for ( int i = beginNo; i < beginNo + recordNum; i++ ) {
            BSONObject obj = new BasicBSONObject();
            obj.put( "testa", i );
            obj.put( "no", i );
            obj.put( "num", i );
            dbcl.delete( obj );
        }
    }

    public void updateData( DBCollection dbcl, int recordNum, int beginNo ) {
        for ( int i = beginNo; i < ( beginNo + recordNum ); i++ ) {
            String matcher = "{no:" + i + "}";
            String modifier = "{$set:{testa:'updatetest" + i + "'}}";
            dbcl.update( matcher, modifier, "" );
        }
    }

    public void queryData( DBCollection dbcl, int recordNum, int beginNo ) {
        for ( int i = beginNo; i < ( beginNo + recordNum ); i++ ) {
            String matcher = "{no:{$et:" + i + "}}";
            dbcl.query( matcher, "", "{'no':1}", "" );
        }
    }

    private class threadInsert extends OperateTask {

        private int beginNo;
        private int endNo;

        private threadInsert( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                int recordNum = endNo - beginNo;
                insertData( cl, recordNum, beginNo, insertRecords1 );
            }
        }
    }

    private class threadDelete extends OperateTask {

        private int beginNo;
        private int endNo;

        private threadDelete( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;

        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                int recordNum = endNo - beginNo;
                deleteData( cl, recordNum, beginNo );
            }
        }
    }

    private class threadUpdate extends OperateTask {
        private int beginNo;
        private int endNo;

        private threadUpdate( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;

        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                int recordNum = endNo - beginNo;
                updateData( cl, recordNum, beginNo );
            }
        }

    }

    private class threadQuery extends OperateTask {

        private int beginNo;
        private int endNo;

        private threadQuery( int beginNo, int endNo ) {
            this.beginNo = beginNo;
            this.endNo = endNo;
        }

        @Override
        public void exec() throws Exception {
            try ( Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "",
                    "" ) ;) {
                CollectionSpace cs1 = db
                        .getCollectionSpace( SdbTestBase.csName );
                DBCollection cl = cs1.getCollection( clName );
                int recordNum = endNo - beginNo;
                queryData( cl, recordNum, beginNo );
            }
        }
    }
}
