package com.sequoiadb.index;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.NodeWrapper;
import com.sequoiadb.commlib.StandTestInterface;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.metaopr.commons.MyUtil;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;

import java.util.ArrayList;
import java.util.List;
import java.util.logging.Logger;

import static com.sequoiadb.metaopr.commons.MyUtil.*;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-8-8
 * @Version 1.00
 */
class IndexTestBase implements StandTestInterface {
    private final Logger log = Logger.getLogger( this.getClass().getName() );
    // num of inserting json
    private static final int NUM = 500 * 1000;

    // cs name
    String csName = null;
    // cl name
    String clName = null;
    // array of indexes to be creating
    List< IndexBean > index2Create = new ArrayList<>( 100 );
    // array of indexes must be created in setup
    List< IndexBean > indexAlreadlyCreated = new ArrayList<>( 100 );
    // array of bson to be inserting
    static List< BSONObject > simpleBSONList = new ArrayList<>( NUM );

    private DBCollection _cl = null;
    private Sequoiadb _db = null;

    public IndexTestBase() {
        csName = this.getClass().getSimpleName() + "cs";
        clName = this.getClass().getSimpleName() + "_cl";

        String[] fileds = { "A", "B", "C", "D", "E", "F" };
        for ( int i = 0; i < fileds.length; i++ ) {
            if ( i < fileds.length / 2 ) {
                indexAlreadlyCreated.add( new IndexBean().setName( "index" + i )
                        .setIndexDef( new BasicBSONObject( fileds[ i ], 1 ) ) );
            } else {
                index2Create.add( new IndexBean().setName( "index" + i )
                        .setIndexDef( new BasicBSONObject( fileds[ i ], 1 ) ) );
            }
        }

        for ( int i = 0; i < NUM; i++ ) {
            BSONObject bson = new BasicBSONObject();
            for ( String filed : fileds ) {
                bson.put( filed, i );
            }
            simpleBSONList.add( bson );
        }
    }

    void insertData() {
        openCl();
        _cl.insert( simpleBSONList );
    }

    void split50() {
        openCl();
        _cl.split( "group1", "group2", 50 );
    }

    private void openCl() {
        if ( _cl == null ) {
            Sequoiadb db = MyUtil.getSdb();
            _cl = db.getCollectionSpace( csName ).getCollection( clName );
        }
    }

    void createIndexCl( BSONObject option ) {
        if ( isCsExisted( csName ) == true ) {
            if ( _db.getCollectionSpace( csName )
                    .getCollection( clName ) != null ) {
                _db.close();
                return;
            }
        } else {
            createCS( csName );
        }
        createCl( csName, clName, option );
        openCl();
    }

    void createIndexes( List< IndexBean > indexBeans ) {
        for ( IndexBean indexBean : indexBeans ) {
            MyUtil.createIndex( _cl, indexBean );
        }
    }

    boolean isIndexesAllCreatedInNodes( List< GroupWrapper > groups ) {
        for ( NodeWrapper node : getAllNodes( groups ) ) {
            DBCollection cl = node.connect().getCollectionSpace( csName )
                    .getCollection( clName );
            if ( MyUtil.isIndexAllCreated( cl, index2Create ) == false ) {
                log.severe( "node check failed :" + node );
                return false;
            }
        }
        return true;
    }

    boolean isIndexesAllCreatedInNodes( String... groups )
            throws ReliabilityException {
        GroupMgr mgr = GroupMgr.getInstance();
        List< GroupWrapper > list = new ArrayList<>();
        for ( String group : groups ) {
            list.add( mgr.getGroupByName( group ) );
        }
        return isIndexesAllCreatedInNodes( list );
    }

    boolean isIndexesAllDeletedInNodes( String... groups )
            throws ReliabilityException {
        GroupMgr mgr = GroupMgr.getInstance();
        List< GroupWrapper > list = new ArrayList<>();
        for ( String group : groups ) {
            list.add( mgr.getGroupByName( group ) );
        }
        return isIndexesAllCreatedInNodes( list );
    }

    private List< NodeWrapper > getAllNodes( List< GroupWrapper > groups ) {
        List< NodeWrapper > nodes = new ArrayList<>();
        for ( GroupWrapper group : groups ) {
            nodes.addAll( group.getNodes() );
        }
        return nodes;
    }

    boolean isIndexesAllDeletedInNodes( List< GroupWrapper > groups ) {
        for ( NodeWrapper node : getAllNodes( groups ) ) {
            DBCollection cl = node.connect().getCollectionSpace( csName )
                    .getCollection( clName );
            if ( MyUtil.isIndexAllDeleted( cl, index2Create ) == false ) {
                log.severe( "node check failed :" + node );
                return false;
            }
        }
        return true;
    }

    IndexTask getCreateTask() {
        return IndexTask.getCreateIndexTask( csName, clName, index2Create );
    }

    IndexTask getDeleteTask() {
        return IndexTask.getRemoveIndexTask( csName, clName,
                indexAlreadlyCreated );
    }

    @BeforeClass
    @Override
    public void setup() {
        _db = getSdb();
    }

    @AfterClass
    @Override
    public void tearDown() {
        _db.dropCollectionSpace( csName );
        _db.close();
    }
}
