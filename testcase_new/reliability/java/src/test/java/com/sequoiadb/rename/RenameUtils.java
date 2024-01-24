package com.sequoiadb.rename;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.SdbTestBase;

/**
 * @Description RenameUtils.java
 * @author luweikang
 * @date 2018年11月7日
 */
public class RenameUtils {

    public static void checkRenameCSResult( Sequoiadb db, String oldCSName,
            String newCSName, int clNum ) {

        if ( db.isCollectionSpaceExist( oldCSName ) ) {
            Assert.fail( "cs it's been renamed, It shouldn't exist" );
        }

        // if cs is not empty, check cl full name
        if ( clNum != 0 ) {
            checkCLFullName( db, newCSName, clNum );
        } else {
            if ( !db.isCollectionSpaceExist( newCSName ) ) {
                Assert.fail( "new cs name is not exist: " + newCSName );
            }
        }
    }

    public static void checkRenameCLResult( Sequoiadb db, String csName,
            String oldCLName, String newCLName ) {

        CollectionSpace cs = db.getCollectionSpace( csName );
        if ( cs.isCollectionExist( oldCLName ) ) {
            Assert.fail( "cl already rename, should not exist" );
        }
        if ( !cs.isCollectionExist( newCLName ) ) {
            Assert.fail( "cl is been rename, should exist" );
        }
    }

    private static void checkCLFullName( Sequoiadb db, String newCSName,
            int clNum ) {
        DBCursor csSnapshotCur = null;
        int times = 0;
        for ( int k = 0; k < 50; k++ ) {
            try {
                csSnapshotCur = db.getSnapshot(
                        Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                        "{'Name':'" + newCSName + "'}", "", "" );
                if ( !csSnapshotCur.hasNext() ) {
                    Assert.fail( "cs it's not exist, csName: " + newCSName );
                }

                BSONObject obj = csSnapshotCur.getNext();
                BasicBSONList cls = ( BasicBSONList ) obj.get( "Collection" );
                if ( cls.size() != clNum ) {
                    times++;
                    if ( times == 50 ) {
                        Assert.fail( "cl count error, exp: " + clNum + ",act :"
                                + cls.size() );
                    }
                    try {
                        Thread.sleep( 100 );
                    } catch ( InterruptedException e ) {
                        Assert.fail( e.getMessage() );
                    }
                    continue;
                }
                for ( int i = 0; i < cls.size(); i++ ) {
                    BSONObject ele = ( BSONObject ) cls.get( i );
                    String name = ( String ) ele.get( "Name" );
                    String csName = name.split( "\\." )[ 0 ];
                    if ( !csName.equals( newCSName ) ) {
                        Assert.fail( "cs name contrast error, exp: " + newCSName
                                + " act: " + csName );
                    }
                }
            } finally {
                if ( csSnapshotCur != null ) {
                    csSnapshotCur.close();
                }
            }
            break;
        }
    }

    public static void insertData( DBCollection cl ) {
        insertData( cl, 1000 );
    }

    public static void insertData( DBCollection cl, int recordNum ) {
        if ( recordNum < 1 ) {
            recordNum = 1;
        }

        /*
         * if recordNum > 1000, this will insert ((int)recordNum/1000)*1000
         * record, else don't do anything such as will insert {a: 0} ~ {a: 999}
         */
        int times = recordNum / 1000;
        for ( int i = 0; i < times; i++ ) {
            List< BSONObject > data = new ArrayList< BSONObject >();
            for ( int j = 0; j < 1000; j++ ) {
                BSONObject record = new BasicBSONObject();
                record.put( "a", i * 1000 + j );
                record.put( "no", "No." + i * 1000 + j );
                record.put( "phone", 13700000000L + i * 1000 + j );
                record.put( "text",
                        "Test ReName, This is the test statement used to populate the data" );
                data.add( record );
            }
            cl.insert( data );
        }

        /*
         * if recordNum%1000 > 0, this will insert the remainder record such as
         * will insert {a: 1000} ~ {a: 1999}
         */
        List< BSONObject > dataA = new ArrayList< BSONObject >();
        for ( int k = 0; k < recordNum % 1000; k++ ) {
            BSONObject record = new BasicBSONObject();
            record.put( "a", times * 1000 + k );
            record.put( "no", "No." + times * 1000 + k );
            record.put( "phone", 13700000000L + times * 1000 + k );
            record.put( "text",
                    "Test ReName, This is the test statement used to populate the data" );
            dataA.add( record );
        }
        if ( dataA.size() != 0 ) {
            cl.insert( dataA );
        }
    }

    /**
     * @param newCSName
     * @param oldCSName
     */
    public static void retryRenameCS( String oldCSName, String newCSName ) {
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        boolean oldCSExist = db.isCollectionSpaceExist( oldCSName );
        boolean newCSExist = db.isCollectionSpaceExist( newCSName );
        try {
            if ( oldCSExist && !newCSExist ) {
                db.renameCollectionSpace( oldCSName, newCSName );
            } else if ( !oldCSExist && newCSExist ) {
                db.renameCollectionSpace( newCSName, oldCSName );
                db.renameCollectionSpace( oldCSName, newCSName );
            } else if ( !oldCSExist && !newCSExist ) {
                Assert.fail( "rename cs,old and new cs notExist" );
            } else {
                Assert.fail( "rename cs,old and new cs all exist" );
            }
        } finally {
            db.close();
        }
    }

    /**
     * @param csName
     * @param oldCLName
     * @param newCLName
     */
    public static void retryRenameCL( String csName, String oldCLName,
            String newCLName ) {
        Sequoiadb db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        CollectionSpace cs = db.getCollectionSpace( csName );
        boolean oldCLExist = cs.isCollectionExist( oldCLName );
        boolean newCLExist = cs.isCollectionExist( newCLName );
        try {
            if ( oldCLExist && !newCLExist ) {
                cs.renameCollection( oldCLName, newCLName );
            } else if ( !oldCLExist && newCLExist ) {
                cs.renameCollection( newCLName, oldCLName );
                cs.renameCollection( oldCLName, newCLName );
            } else if ( !oldCLExist && !newCLExist ) {
                Assert.fail( "rename cs,old and new cs notExist" );
            } else {
                Assert.fail( "rename cs,old and new cs exist" );
            }
        } finally {
            db.close();
        }
    }
}
