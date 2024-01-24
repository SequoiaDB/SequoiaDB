package com.sequoiadb.snapshot;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import java.util.List;

/**
 * @Description: seqDB-24325:在没有事务锁等待的 data、coord 节点上查询事务锁等待信息
 * @Author Yang Qincheng
 * @Date 2021.08.27
 */
public class Snapshot24325 extends SdbTestBase {
    private Sequoiadb db;
    private Sequoiadb dataNode = null;
    private final String clName = "cl_24325";

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
        try( Sequoiadb db1 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
             Sequoiadb db2 = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
             Sequoiadb db3 = new Sequoiadb( SdbTestBase.coordUrl, "", "" ) ){

            db1.beginTransaction();
            db2.beginTransaction();
            db3.beginTransaction();

            query( db1 );
            query( db2 );
            query( db3 );

            checkSnapshot( db );

            // dataNode is null means standalone mode
            if ( dataNode != null ){
                // if cluster mode, we should check data node
                checkSnapshot( dataNode );
            }

            db1.commit();
            db2.commit();
            db3.commit();
        }
    }

    private void query( Sequoiadb db ){
        DBCollection cl = db.getCollectionSpace( csName ).getCollection( clName );
        DBCursor cursor = cl.query();
        while ( cursor.hasNext() ){
            cursor.getNext();
        }
    }

    private void checkSnapshot( Sequoiadb db ){
        try( DBCursor cursor = db.getSnapshot( Sequoiadb.SDB_SNAP_TRANSWAITS, "","","" ) ){
            Assert.assertFalse( cursor.hasNext() );
        }
    }
}
