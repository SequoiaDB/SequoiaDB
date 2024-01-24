package com.sequoiadb.lzw;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class LzwUtils3 extends SdbTestBase {
    public static ArrayList< String > groupList;

    public static boolean isStandAlone( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -159 ) {
                System.out.printf( "run mode is standalone" );
                return true;
            }
        }
        return false;
    }

    @SuppressWarnings("deprecation")
    public static boolean isDictExist( DBCollection cl, String dataGroupName ) {
        // connect to data node of cl
        Sequoiadb dataDB = null;
        DBCursor snapshot = null;
        try {
            Sequoiadb db = cl.getSequoiadb();
            String url = db.getReplicaGroup( dataGroupName ).getMaster()
                    .getNodeName();
            dataDB = new Sequoiadb( url, "", "" );

            // get details of snapshot
            BSONObject nameBSON = new BasicBSONObject();
            nameBSON.put( "Name", cl.getFullName() );
            snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
            if ( !snapshot.hasNext() ) {
                CollectionSpace cs = dataDB.getCollectionSpace( csName );
                throw new BaseException( -10000,
                        "snapshot is not exist. cl exists: "
                                + cs.isCollectionExist( cl.getFullName() ) );
            }
            BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                    .get( "Details" );
            BSONObject detail = ( BSONObject ) details.get( 0 );
            // judge whether dictionary is created
            return ( boolean ) detail.get( "DictionaryCreated" );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        } finally {
            if ( dataDB != null ) {
                dataDB.closeAllCursors();
                dataDB.disconnect();
            }
        }
        return false;

    }

    public static void waitCreateDict( DBCollection cl, String dataGroupName ) {
        try {
            for ( int i = 0; i < 60 * 60; i++ ) {
                if ( LzwUtils3.isDictExist( cl, dataGroupName ) ) {
                    return;
                }
                Thread.sleep( 1000 );
            }
            Assert.fail( "watting for create dict faile time(60*60s)" );
        } catch ( BaseException e ) {
            throw e;
        } catch ( InterruptedException e ) {
            e.printStackTrace();
        }
    }

    public static boolean OneGroupMode( Sequoiadb sdb ) {
        if ( getDataGroups( sdb ).size() < 2 ) {
            System.out.printf( "only one group" );
            return true;
        }
        return false;
    }

    public static ArrayList< String > getDataGroups( Sequoiadb sdb )
            throws BaseException {
        try {
            groupList = sdb.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            e.printStackTrace();
            throw e;
        }
        return groupList;
    }

    public static void checkCompressed( DBCollection cl,
            String dataGroupName ) {
        // connect to data node of cl
        Sequoiadb db = cl.getSequoiadb();
        Sequoiadb dataDB = getDataDB( db, dataGroupName );
        checkCompression( dataDB, cl.getName() );
    }

    public static Sequoiadb getDataDB( Sequoiadb db, String dataGroupName ) {
        // connect to data node of cl
        return db.getReplicaGroup( dataGroupName ).getMaster().connect();
    }

    public static void checkCompression( Sequoiadb dataDB, String clName ) {
        // get details of snapshot
        BSONObject nameBSON = new BasicBSONObject();
        nameBSON.put( "Name", csName + "." + clName );
        DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
        BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                .get( "Details" );
        BSONObject detail = ( BSONObject ) details.get( 0 );

        // judge whether data is compressed
        double ration = ( double ) detail.get( "CurrentCompressionRatio" );
        boolean ratioRight = ration < 1;
        String attr = ( String ) detail.get( "Attribute" );
        boolean attrRight = attr.equals( "Compressed" );
        String type = ( String ) detail.get( "CompressionType" );
        boolean typeRight = type.equals( "lzw" );
        if ( !( ratioRight && attrRight && typeRight ) ) {
            Assert.fail( "data is not compressed:" + ration + " " + attr + " "
                    + type );
        }
    }
}