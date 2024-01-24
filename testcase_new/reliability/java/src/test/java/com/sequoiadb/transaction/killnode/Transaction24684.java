package com.sequoiadb.transaction.killnode;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.*;
import com.sequoiadb.crud.CRUDUtils;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.KillNode;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;

/**
 * @description seqDB-24684:事务中删除数据未提交，主节点异常重启
 * @author zhangyanan
 * @date 2021.11.25
 * @updateUser zhangyanan
 * @updateDate 2021.11.25
 * @updateRemark
 * @version 2.10
 */
@Test
public class Transaction24684 extends SdbTestBase {
    private Sequoiadb sdb;
    private Sequoiadb transDB;
    private DBCollection cl = null;
    private CollectionSpace cs = null;
    private String clGroupName = null;
    private String clName = "cl_24684";
    private GroupMgr groupMgr;
    private int dataNum = 10;
    private int beginNo = 0;
    private ArrayList< BSONObject > allRecords = new ArrayList< BSONObject >();
    private ArrayList< BSONObject > nullRecords = new ArrayList< BSONObject >();

    @BeforeClass
    public void setUp() throws ReliabilityException {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        transDB = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        groupMgr = GroupMgr.getInstance();
        if ( CommLib.isStandAlone( sdb ) ) {
            throw new SkipException( "checkBusiness standlone failed" );
        }
        if ( !groupMgr.checkBusiness( 120 ) ) {
            throw new SkipException( "checkBusiness business failed" );
        }
        clGroupName = groupMgr.getAllDataGroupName().get( 0 );
        BSONObject options = new BasicBSONObject();
        options.put( "Group", clGroupName );
        cs = sdb.getCollectionSpace( SdbTestBase.csName );
        cl = cs.createCollection( clName, options );
        CRUDUtils.insertData( cl, dataNum, beginNo, allRecords );
    }

    @Test
    public void test() throws ReliabilityException, InterruptedException {
        transDB.beginTransaction();
        DBCollection transCl = transDB.getCollectionSpace( csName )
                .getCollection( clName );
        String deleteMatcher = "{$and:[{no:{$gte:" + beginNo + "}},{no:{$lte:"
                + dataNum + "}}]}";
        transCl.delete( deleteMatcher );
        CRUDUtils.checkRecords( cl, nullRecords, "" );

        TaskMgr mgr = new TaskMgr();
        GroupWrapper dataGroup = groupMgr.getGroupByName( clGroupName );
        NodeWrapper dataNodes = dataGroup.getMaster();
        FaultMakeTask task =  KillNode.getFaultMakeTask( dataNodes, 0 );
        mgr.addTask( task );
        mgr.execute();
        if ( !groupMgr.checkBusinessWithLSN() ) {
            throw new SkipException( "checkBusiness business failed" );
        }
        CRUDUtils.checkRecords( cl, allRecords, "" );
    }

    @AfterClass
    public void tearDown() throws InterruptedException {
        try {
            if ( cs.isCollectionExist( clName ) )
                cs.dropCollection( clName );
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
            if ( transDB != null ) {
                transDB.close();
            }
        }
    }

}
