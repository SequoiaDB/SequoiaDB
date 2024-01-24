package com.sequoiadb.rename;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BSONDecimal;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.DBLob;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.base.Node.NodeStatus;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Description RenameUtil.java
 * @author luweikang
 * @date 2018年10月17日
 */

public class RenameUtil extends SdbTestBase {

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

    public static List< ObjectId > putLob( DBCollection cl, byte[] data,
            int lobNum ) {

        List< ObjectId > idList = new ArrayList< ObjectId >();
        for ( int i = 0; i < lobNum; i++ ) {
            DBLob lob = null;
            try {
                lob = cl.createLob();
                lob.write( data );
                idList.add( lob.getID() );
            } finally {
                if ( lob != null ) {
                    lob.close();
                }
            }
        }
        return idList;
    }

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

    public static void removeCS( Sequoiadb db, String csName ) {
        if ( db.isCollectionSpaceExist( csName ) ) {
            db.dropCollectionSpace( csName );
        }
    }

    public static CollectionSpace createCS( Sequoiadb db, String csName ) {
        return createCS( db, csName, null );
    }

    public static CollectionSpace createCS( Sequoiadb db, String csName,
            String option ) {
        CollectionSpace cs = null;
        BSONObject options = ( BSONObject ) JSON.parse( option );
        try {
            if ( db.isCollectionSpaceExist( csName ) ) {
                db.dropCollectionSpace( csName );
                ;
            }

            cs = db.createCollectionSpace( csName, options );
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
        return cs;
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

    public static void retryToRenameCS( Sequoiadb db, String oldCSName,
            String newCSName ) {

        db.renameCollectionSpace( oldCSName, newCSName );

        db.renameCollectionSpace( newCSName, oldCSName );

    }

    public static void retryToRenameCL( Sequoiadb db, String csName,
            String oldCLName, String newCLName ) {

        CollectionSpace cs = db.getCollectionSpace( csName );

        cs.renameCollection( oldCLName, newCLName );

        cs.renameCollection( newCLName, oldCLName );

    }

    public static String getGroupName( Sequoiadb sdb ) {
        ArrayList< String > rgNames = CommLib.getDataGroupNames( sdb );
        int serino = ( int ) ( Math.random() * rgNames.size() );
        String groupName = rgNames.get( serino );
        return groupName;
    }

    public static ArrayList< BSONObject > insertDatas( DBCollection dbcl,
            int insertNums, int beginNo ) {
        int batchNums = 10000;
        int times = insertNums / batchNums;
        int remainder = insertNums % batchNums;
        if ( times == 0 ) {
            times = 1;
        }

        ArrayList< BSONObject > insertRecords = new ArrayList< BSONObject >();
        for ( int k = 0; k < times; k++ ) {
            if ( remainder != 0 && k == times - 1 ) {
                batchNums = remainder;
            }

            ArrayList< BSONObject > insertRecord = new ArrayList< BSONObject >();
            for ( int i = 0; i < batchNums; i++ ) {
                int count = beginNo++;
                BSONObject obj = new BasicBSONObject();
                obj.put( "testa", "test" + count );
                String str = "32345.06789123456" + count;
                BSONDecimal decimal = new BSONDecimal( str );
                obj.put( "decimala", decimal );
                obj.put( "no", count );
                obj.put( "order", count );
                obj.put( "inta", count );
                obj.put( "ftest", count + 0.2345 );
                obj.put( "str", "test_" + String.valueOf( count ) );

                insertRecord.add( obj );
                insertRecords.add( obj );
            }
            dbcl.insert( insertRecord );
        }
        return insertRecords;
    }

    public static int getNodeNum( Sequoiadb sdb, String groupName ) {
        ReplicaGroup rg = sdb.getReplicaGroup( groupName );
        @SuppressWarnings("deprecation")
        int num = rg.getNodeNum( NodeStatus.SDB_NODE_ALL );
        return num;
    }

    public static void checkRecords( DBCollection dbcl,
            List< BSONObject > expRecords, String orderBy ) {
        DBCursor cursor = dbcl.query( "", "", orderBy, "" );
        int count = 0;
        while ( cursor.hasNext() ) {
            BSONObject record = cursor.getNext();
            BSONObject expRecord = expRecords.get( count++ );
            Assert.assertEquals( record, expRecord );
        }
        cursor.close();
        Assert.assertEquals( count, expRecords.size() );
    }

}
