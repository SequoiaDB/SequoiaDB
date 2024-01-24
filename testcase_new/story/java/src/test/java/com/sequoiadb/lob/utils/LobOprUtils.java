package com.sequoiadb.lob.utils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Random;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * FileName: Commlib.java public call function for test basicOperation
 * 
 * @author wuyan
 * @Date 2016.9.19
 * @version 1.00
 */
public class LobOprUtils {

    public static ArrayList< String > groupList;

    /**
     * get the buff MD5 value
     * 
     * @param inbuff
     *            the object of need to get the MD5
     * @return the MD5 value
     */
    public static String getMd5( Object inbuff ) {
        MessageDigest md5 = null;
        String value = "";

        try {
            md5 = MessageDigest.getInstance( "MD5" );
            if ( inbuff instanceof ByteBuffer ) {
                md5.update( ( ByteBuffer ) inbuff );
            } else if ( inbuff instanceof String ) {
                md5.update( ( ( String ) inbuff ).getBytes() );
            } else if ( inbuff instanceof byte[] ) {
                md5.update( ( byte[] ) inbuff );
            } else {
                Assert.fail( "invalid parameter!" );
            }
            BigInteger bi = new BigInteger( 1, md5.digest() );
            value = bi.toString( 16 );
        } catch ( NoSuchAlgorithmException e ) {
            e.printStackTrace();
            Assert.fail( "fail to get md5!" + e.getMessage() );
        }
        return value;
    }

    /**
     * generating byte to write lob
     * 
     * @param length
     *            generating byte stream size
     * @return String sb
     */
    public static String getRandomString( int length ) {
        String base = "abcdefahijklmnopqrstuvwxyz0123456789";
        Random random = new Random();

        int times = length / 1024;
        if ( times < 1 ) {
            times = 1;
        }

        int len = length >= 1024 ? 1024 : 0;
        int mulByteNum = length % 1024;

        StringBuffer sb = new StringBuffer();
        for ( int i = 0; i < times; ++i ) {
            if ( i == times - 1 ) {
                len += mulByteNum;
            }

            char ch = base.charAt( random.nextInt( base.length() ) );
            for ( int k = 0; k < len; ++k ) {
                sb.append( ch );
            }

        }
        return sb.toString();
    }

    public static ArrayList< String > getDataGroups( Sequoiadb sdb ) {
        try {
            groupList = sdb.getReplicaGroupNames();
            groupList.remove( "SYSCatalogGroup" );
            groupList.remove( "SYSCoord" );
            groupList.remove( "SYSSpare" );
        } catch ( BaseException e ) {
            Assert.assertTrue( false, "getDataGroups fail " + e.getMessage() );
        }
        return groupList;
    }

    public static String chooseDataGroups( Sequoiadb sdb ) {
        groupList = getDataGroups( sdb );
        int groupsNum = groupList.size();
        int length = ( groupsNum > groupList.size() ) ? groupList.size()
                : groupsNum;
        String ret = "";
        for ( int i = 0; i < length; i++ ) {
            ret = ret + "'" + groupList.get( i ) + "',";
        }
        return ( ret.length() == 0 ) ? ret
                : ret.substring( 0, ret.length() - 1 );
    }

    public static String getSrcGroupName( Sequoiadb sdb, String csName,
            String clName ) {
        String groupName = "";
        String cond = String.format( "{Name:\"%s.%s\"}", csName, clName );
        DBCursor cr = sdb.getSnapshot( 8, cond, null, null );
        groupList = getDataGroups( sdb );
        while ( cr.hasNext() ) {
            BSONObject obj = cr.getNext();

            BasicBSONObject doc = ( BasicBSONObject ) obj;
            doc.getString( "Name" );
            BasicBSONList subdoc = ( BasicBSONList ) doc.get( "CataInfo" );
            BasicBSONObject elem = ( BasicBSONObject ) subdoc.get( 0 );
            groupName = elem.getString( "GroupName" );
        }
        return groupName;
    }

    public static String getSplitGroupName( String groupName ) {
        String tarRgName = "";
        for ( int i = 0; i < groupList.size(); ++i ) {
            String name = groupList.get( i );
            if ( !name.equals( groupName ) ) {
                tarRgName = name;
                break;
            }
        }
        return tarRgName;

    }

    public static void createDomain( Sequoiadb sdb, String domainName ) {
        try {
            BSONObject options = new BasicBSONObject();
            options = ( BSONObject ) JSON.parse( "{'Groups': ["
                    + chooseDataGroups( sdb ) + "],AutoSplit:true}" );
            sdb.createDomain( domainName, options );
        } catch ( BaseException e ) {
            Assert.assertTrue( false,
                    "createDomain fail " + +e.getErrorCode() + e.getMessage() );
        }
    }

    public static DBCollection createCL( CollectionSpace cs, String clName,
            String option ) {
        DBCollection cl = null;
        BSONObject options = ( BSONObject ) JSON.parse( option );
        try {
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }

            cl = cs.createCollection( clName, options );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cl;
    }

    public static DBCollection createCL( CollectionSpace cs, String clName ) {
        return createCL( cs, clName, null );
    }

    public static byte[] appendBuff( byte[] oldBuff, byte[] buff4Append,
            int offset ) {
        byte[] newBuff;
        if ( oldBuff.length >= offset + buff4Append.length ) {
            newBuff = new byte[ oldBuff.length ];
        } else {
            newBuff = new byte[ offset + buff4Append.length ];
        }
        System.arraycopy( oldBuff, 0, newBuff, 0, oldBuff.length );
        System.arraycopy( buff4Append, 0, newBuff, offset, buff4Append.length );
        return newBuff;
    }

    /**
     * generating byte to write lob
     *
     * @param length
     *            generating byte stream size
     * @return byte[] bytes
     */
    public static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    /**
     * convenient method for creating lob and write some data to this lob.
     *
     * @param dbcl
     * @param id
     * @return
     */
    static ObjectId createAndWriteLob( DBCollection dbcl, ObjectId id,
            byte[] data ) {
        DBLob lob = dbcl.createLob( id );
        lob.write( data );
        lob.close();
        return lob.getID();
    }

    public static ObjectId createAndWriteLob( DBCollection dbcl, byte[] data ) {
        return createAndWriteLob( dbcl, null, data );
    }

    static ObjectId createAndWriteLob( Sequoiadb db, String csName,
            String clName, ObjectId id, byte[] data ) {
        return createAndWriteLob(
                db.getCollectionSpace( csName ).getCollection( clName ), id,
                data );
    }

    static ObjectId createAndWriteLob( Sequoiadb db, String csName,
            String clName, byte[] data ) {
        return createAndWriteLob(
                db.getCollectionSpace( csName ).getCollection( clName ), data );
    }

    static byte[] seekAndReadLob( DBCollection dbcl, ObjectId lobid,
            int readSize, int offset ) {
        byte[] b = new byte[ readSize ];
        try ( DBLob lob = dbcl.openLob( lobid )) {
            lob.seek( offset, DBLob.SDB_LOB_SEEK_SET );
            lob.read( b );
        }
        return b;
    }

    public static void checkSplitResult( Sequoiadb sdb, String csName,
            String clName, ArrayList< String > splitGroupNames,
            double expErrorValue ) {
        DBCollection cl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        DBCursor listCursor = cl.listLobs();
        int count = 0;
        while ( listCursor.hasNext() ) {
            count++;
            listCursor.getNext();
        }
        listCursor.close();

        int actListNums = 0;
        for ( int i = 0; i < splitGroupNames.size(); i++ ) {
            String nodeName = sdb.getReplicaGroup( splitGroupNames.get( i ) )
                    .getMaster().getNodeName();

            try ( Sequoiadb dataDB = new Sequoiadb( nodeName, "", "" )) {
                DBCollection dataCL = dataDB.getCollectionSpace( csName )
                        .getCollection( clName );
                DBCursor listLobs = dataCL.listLobs();
                int subCount = 0;
                while ( listLobs.hasNext() ) {
                    subCount++;
                    listLobs.getNext();
                }
                listLobs.close();
                actListNums += subCount;
                // list lobs deviation value is less than 50 percent after split
                double expRate = count / splitGroupNames.size();
                double actErrorValue = Math.abs( expRate - subCount ) / expRate;
                if ( actErrorValue > expErrorValue ) {
                    Assert.assertTrue( false,
                            "errorValue: " + actErrorValue + " subCont:"
                                    + subCount + "  explistnum:" + count );
                }
            }
        }
        // sum of query results on each group is equal to the results of coord
        Assert.assertEquals( actListNums, count,
                "list lobs error." + "allCount:" + actListNums );
    }

    public static void assertByteArrayEqual( byte[] actual, byte[] expect ) {
        assertByteArrayEqual( actual, expect, "" );
    }

    public static void assertByteArrayEqual( byte[] actual, byte[] expect,
            String msg ) {
        if ( !Arrays.equals( actual, expect ) ) {
            String workDirPath = SdbTestBase.getWorkDir();
            File workDir = new File( workDirPath );
            if ( !workDir.isDirectory() )
                throw new RuntimeException(
                        "the path can not use: " + workDirPath );

            String callerClassName = getCallerName();
            File fileActual = new File( workDirPath + File.separator
                    + callerClassName + "_actual" );
            File fileExpect = new File( workDirPath + File.separator
                    + callerClassName + "_expect" );
            try {
                if ( fileActual.exists() ) {
                    fileActual.delete();
                    fileActual.createNewFile();
                }
                if ( fileExpect.exists() ) {
                    fileExpect.delete();
                    fileExpect.createNewFile();
                }

                try ( FileOutputStream out = new FileOutputStream(
                        fileActual )) {
                    out.write( actual );
                    out.flush();
                }
                try ( FileOutputStream out = new FileOutputStream(
                        fileExpect )) {
                    out.write( expect );
                    out.flush();
                }

                Assert.fail( msg + "; data is written into files in "
                        + workDirPath );
            } catch ( FileNotFoundException e ) {
                e.printStackTrace();
            } catch ( IOException e ) {
                e.printStackTrace();
            }
        }
    }

    public static String getCallerName() {
        StackTraceElement[] stackTrace = Thread.currentThread().getStackTrace();
        String thisClassName = stackTrace[ 1 ].getClassName();
        String currClassName = null;
        for ( int i = 2; i < stackTrace.length; ++i ) {
            currClassName = stackTrace[ i ].getClassName();
            if ( !currClassName.equals( thisClassName ) ) {
                break;
            }
        }
        String classFullName = currClassName;
        String[] classNameArr = classFullName.split( "\\." );
        String simpleClassName = classNameArr[ classNameArr.length - 1 ];
        return simpleClassName;
    }

}
