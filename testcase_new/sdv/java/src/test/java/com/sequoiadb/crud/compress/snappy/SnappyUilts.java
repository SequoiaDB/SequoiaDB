package com.sequoiadb.crud.compress.snappy;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class SnappyUilts extends SdbTestBase {

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

    public static boolean OneGroupMode( Sequoiadb sdb ) {
        if ( getDataGroups( sdb ).size() < 2 ) {
            System.out.printf( "only one group" );
            return true;
        }
        return false;
    }

    public static ArrayList< String > getDataGroups( Sequoiadb sdb )
            throws BaseException {
        ArrayList< String > groupList;
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

    @SuppressWarnings("deprecation")
    public static void insertData( DBCollection cl, int recSum ) {
        List< BSONObject > recs = new ArrayList< >();
        for ( int i = 0; i < recSum; i++ ) {
            BSONObject rec = new BasicBSONObject();
            rec.put( "a", i );
            rec.put( "b", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" );
            recs.add( rec );
        }
        cl.bulkInsert( recs, 0 );
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
        String url = db.getReplicaGroup( dataGroupName ).getMaster()
                .getNodeName();
        return new Sequoiadb( url, "", "" );
    }

    @SuppressWarnings("deprecation")
    public static void checkCompression( Sequoiadb dataDB, String clName ) {
        int tryTimes = 10;
        boolean compressed = false;
        for ( int i = 0; i < tryTimes; i++ ) {
            // get details of snapshot
            BSONObject nameBSON = new BasicBSONObject();
            nameBSON.put( "Name", csName + "." + clName );
            DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
            dataDB.setSessionAttr(
                    ( BSONObject ) JSON.parse( "{PreferedInstance:'M'}" ) );
            // TODO: test code. to be delete
            if ( !snapshot.hasNext() ) {
                CollectionSpace cs = dataDB.getCollectionSpace( csName );
                throw new BaseException( "snapshot is not exist. cl exists: "
                        + cs.isCollectionExist( clName ) );
            }
            BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                    .get( "Details" );
            BSONObject detail = ( BSONObject ) details.get( 0 );
            snapshot.close();

            // judge whether data is compressed
            boolean ratioRight = ( double ) detail
                    .get( "CurrentCompressionRatio" ) < 1;
            boolean attrRight = ( ( String ) detail.get( "Attribute" ) )
                    .equals( "Compressed" );
            boolean typeRight = ( ( String ) detail.get( "CompressionType" ) )
                    .equals( "snappy" );
            if ( ratioRight && attrRight && typeRight ) {
                compressed = true;
                break;
            }

            // try again after 1 second. compression needs time.
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        }
        if ( !compressed ) {
            Assert.fail( "data is not compressed!" );
        }
    }
}
