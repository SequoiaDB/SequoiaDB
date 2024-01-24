package com.sequoiadb.clustermanager;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

import static org.testng.Assert.assertFalse;
import static org.testng.Assert.assertNotNull;
import static org.testng.AssertJUnit.assertTrue;

/**
 * Created by laojingtang on 17-11-29.
 */
public class TestGetSlave13645 extends SdbTestBase {
    private Sequoiadb db;
    private List< String > groupnames = new ArrayList<>( 10 );

    @BeforeClass
    public void setup() {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        DBCursor cursor = db.listReplicaGroups();
        while ( cursor.hasNext() ) {
            BSONObject object = cursor.getNext();
            String name = object.get( "GroupName" ).toString();
            groupnames.add( name );
        }
    }

    @AfterClass
    public void teardown() {
        db.close();
    }

    @Test
    public void test() {
        for ( String groupname : groupnames ) {
            if ( groupname.equals( "SYSCoord" ) )
                continue;
            ReplicaGroup rg = db.getReplicaGroup( groupname );
            boolean isSingleNode = false;
            if ( getRgNodeNum( rg ) == 1 ) {
                isSingleNode = true;
            }

            // test normal
            Node node = rg.getSlave();
            if ( isSingleNode )
                assertTrue( isMaster( rg, node ) );
            else
                assertFalse( isMaster( rg, node ) );

            // test 1~7
            node = rg.getSlave( 1, 2, 3, 4, 5, 6, 7 );
            if ( isSingleNode )
                assertTrue( isMaster( rg, node ) );
            else
                assertFalse( isMaster( rg, node ) );

            // test 1
            node = rg.getSlave( 1 );
            assertNotNull( node );

            // test 1 2 7
            node = rg.getSlave( 1, 2, 7 );
            if ( isSingleNode )
                assertTrue( isMaster( rg, node ) );
            else
                assertFalse( isMaster( rg, node ) );

            // test 8
            try {
                rg.getSlave( 8 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_INVALIDARG
                        .getErrorCode() )
                    throw e;
            }

            // test 1,2,3,4,5,6,7,8
            try {
                rg.getSlave( 1, 2, 3, 4, 5, 6, 7, 8 );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_INVALIDARG
                        .getErrorCode() )
                    throw e;
            }

        }
    }

    private boolean isMaster( ReplicaGroup rg, Node node ) {
        Node master = rg.getMaster();
        return master.getNodeName().equals( node.getNodeName() )
                && master.getHostName().equals( node.getHostName() )
                && master.getNodeId() == node.getNodeId()
                && master.getPort() == node.getPort() ? true : false;

    }

    private int getRgNodeNum( ReplicaGroup rg ) {
        BSONObject object = rg.getDetail();
        BasicBSONList list = ( BasicBSONList ) object.get( "Group" );
        return list.size();
    }
}
