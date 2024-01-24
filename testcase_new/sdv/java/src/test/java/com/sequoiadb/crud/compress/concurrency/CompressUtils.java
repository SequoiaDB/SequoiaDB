package com.sequoiadb.crud.compress.concurrency;

import java.util.ArrayList;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

public class CompressUtils extends SdbTestBase {
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

    @SuppressWarnings({ "resource", "deprecation" })
    public static void checkCompressed( DBCollection cl, String dataGroupName,
            String compressionType ) {
        int tryTimes = 10;
        boolean compressed = false;
        Sequoiadb db = cl.getSequoiadb();
        String url = db.getReplicaGroup( dataGroupName ).getMaster()
                .getNodeName();
        // connect to data node of cl
        Sequoiadb dataDB = null;
        try {
            dataDB = new Sequoiadb( url, "", "" );
            for ( int i = 0; i < tryTimes; i++ ) {

                // get details of snapshot
                BSONObject nameBSON = new BasicBSONObject();
                nameBSON.put( "Name", cl.getFullName() );
                DBCursor snapshot = dataDB.getSnapshot( 4, nameBSON, null,
                        null );
                BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                        .get( "Details" );
                BSONObject detail = ( BSONObject ) details.get( 0 );

                // judge whether data is compressed
                if ( compressionType != null ) {
                    double expRatio = 1.0;
                    if ( compressionType.equals( "lzw" ) ) {
                        expRatio = 0.9;
                    } else if ( compressionType.equals( "snappy" ) ) {
                        expRatio = 0.5;
                    }
                    boolean ratioRight = ( double ) detail
                            .get( "CurrentCompressionRatio" ) < expRatio;
                    boolean attrRight = ( ( String ) detail.get( "Attribute" ) )
                            .equals( "Compressed" );
                    boolean typeRight = ( ( String ) detail
                            .get( "CompressionType" ) )
                                    .equals( compressionType );
                    if ( ratioRight && attrRight && typeRight ) {
                        compressed = true;
                        break;
                    }
                } else {
                    boolean typeRight = ( ( String ) detail
                            .get( "CompressionType" ) ).equals( "" );
                    compressed = typeRight;
                    break;
                }

                // try again after 1 second. compression needs time
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
        if ( !compressed ) {
            throw new BaseException( "data is not compressed" );
        }
    }

    @SuppressWarnings("deprecation")
    public static void waitCreateDict( DBCollection cl, String dataGroupName ) {
        int passSecond = 0;
        int waitSecond = 120;
        for ( passSecond = 0; passSecond < waitSecond; passSecond++ ) {
            try {
                Thread.sleep( 1000 );
                if ( CompressUtils.isDictExist( cl, dataGroupName ) ) {
                    break;
                }
            } catch ( BaseException e ) {
                throw e;
            } catch ( InterruptedException e ) {
                e.printStackTrace();
            }
        }
        if ( passSecond == waitSecond ) {
            throw new BaseException( "fail to create dictionary" );
        }
    }

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
            BasicBSONList details = ( BasicBSONList ) snapshot.getNext()
                    .get( "Details" );
            BSONObject detail = ( BSONObject ) details.get( 0 );
            // judge whether dictionary is created
            return ( boolean ) detail.get( "DictionaryCreated" );
        } finally {
            if ( dataDB != null ) {
                dataDB.disconnect();
            }
        }
    }

    public static ArrayList< String > getDataGroups( Sequoiadb db ) {
        ArrayList< String > groupList = null;
        try {
            groupList = db.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            throw e;
        }
        return groupList;
    }

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
}
