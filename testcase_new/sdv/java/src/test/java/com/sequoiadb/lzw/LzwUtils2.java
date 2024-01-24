package com.sequoiadb.lzw;

import java.util.ArrayList;
import java.util.Random;

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

public class LzwUtils2 extends SdbTestBase {
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

    /**
     * judge whether dictionary of lzw is created
     * 
     * @param cl
     * @param dataGroupName
     * @return boolean
     */
    @SuppressWarnings({ "deprecation", "resource" })
    public static boolean isDictExist( DBCollection cl, String dataGroupName ) {
        // connect to data node of cl
        Sequoiadb db = cl.getSequoiadb();
        String url = db.getReplicaGroup( dataGroupName ).getMaster()
                .getNodeName();
        Sequoiadb dataDB = null;
        try {
            dataDB = new Sequoiadb( url, "", "" );
            // get details of snapshot
            BSONObject nameBSON = new BasicBSONObject();
            nameBSON.put( "Name", cl.getFullName() );
            DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null, null );
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
        } finally {
            dataDB.disconnect();
        }
    }

    public static void waitCreateDict( DBCollection cl, String dataGroupName ) {
        int timeout = 300;
        while ( !LzwUtils2.isDictExist( cl, dataGroupName ) ) {
            if ( timeout > 0 ) {
                try {
                    Thread.sleep( 1000 );
                    timeout--;
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
            } else {
                Assert.fail( "timeout, dictionary not created." );
            }
        }
    }

    /**
     * get data groups
     * 
     * @param db
     * @return groupList
     */
    public static ArrayList< String > getDataGroups( Sequoiadb db ) {
        ArrayList< String > groupList = null;
        try {
            groupList = db.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "getDataGroups fail " + e.getMessage() );
        }
        return groupList;
    }

    /**
     * get a random string
     * 
     * @param length
     * @return
     */
    public static String getRandomString( int length ) {
        String base = "abc";
        Random random = new Random();
        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < length; i++ ) {
            int index = random.nextInt( base.length() );
            sb.append( base.charAt( index ) );
        }
        return sb.toString();
    }

    /**
     * check whether data is compressed
     * 
     * @param cl
     * @param dataGroupName
     */
    @SuppressWarnings({ "deprecation", "resource" })
    public static void checkCompressed( DBCollection cl,
            String dataGroupName ) {
        int tryTimes = 10;
        boolean isCompressed = false;
        Sequoiadb db = cl.getSequoiadb();
        String url = db.getReplicaGroup( dataGroupName ).getMaster()
                .getNodeName();
        Sequoiadb dataDB = null;
        try {
            dataDB = new Sequoiadb( url, "", "" );
            for ( int i = 0; i < tryTimes; i++ ) {
                // connect to data node of cl
                // get details of snapshot
                BSONObject nameBSON = new BasicBSONObject();
                nameBSON.put( "Name", cl.getFullName() );
                DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null,
                        null );
                BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                        .get( "Details" );
                BSONObject detail = ( BSONObject ) details.get( 0 );

                // judge whether data is compressed
                boolean ratioRight = ( double ) detail
                        .get( "CurrentCompressionRatio" ) < 1;
                boolean attrRight = ( ( String ) detail.get( "Attribute" ) )
                        .equals( "Compressed" );
                boolean typeRight = ( ( String ) detail
                        .get( "CompressionType" ) ).equals( "lzw" );
                if ( ratioRight && attrRight && typeRight ) {
                    isCompressed = true;
                }

                // try again after 1 second
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    e.printStackTrace();
                }
            }
        } finally {
            if ( dataDB != null ) {
                dataDB.disconnect();
            }
        }
        if ( !isCompressed ) {
            throw new BaseException( -10000, "data is not compressed" );
        }
    }
}
