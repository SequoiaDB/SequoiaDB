package com.sequoiadb.snapshot;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description: seqDB-24328:在没有事务的 data、coord 节点上执行死锁检测
 * @Author Yang Qincheng
 * @Date 2021.08.28
 */
public class Snapshot24328 extends SdbTestBase {
    private Sequoiadb db;
    private Sequoiadb dataNode = null;
    private final String clName = "cl_24328";

    @BeforeClass
    public void setUp(){
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCollection cl = db.getCollectionSpace( csName ).createCollection( clName );
        SnapshotUtil.insertData( cl );

        if ( !CommLib.isStandAlone( db ) ){
            List< String > groupNameList =  CommLib.getCLGroups( cl );
            String nodeName = db.getReplicaGroup( groupNameList.get(0) ).getMaster().getNodeName();
            dataNode = new Sequoiadb( nodeName, "", "" );
        }
    }
    @AfterClass
    public void tearDown(){
        try {
            db.getCollectionSpace( csName ).dropCollection( clName );
        }finally {
            db.close();
            if ( dataNode != null ){
                dataNode.close();
            }
        }
    }

    @Test
    public void test(){
        checkSnapshot( db );

        // dataNode is null means standalone mode
        if ( dataNode != null ){
            // if cluster mode, we should check data node
            checkSnapshot( dataNode );
        }
    }

    private void checkSnapshot( Sequoiadb db ){
        try( DBCursor cursor = db.getSnapshot(Sequoiadb.SDB_SNAP_TRANSDEADLOCK, "","","") ){
            Assert.assertFalse( cursor.hasNext() );
        }
    }
}
