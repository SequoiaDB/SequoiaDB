package com.sequoiadb.subcl.brokennetwork.commlib;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.Assert;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * @author linsuqiang
 * @version 1.00
 */

public class Utils {
    public static final int SCLNUM = 500;
    public static final int RANGE_WIDTH = 100;

    public static void createMclAndScl( Sequoiadb db, String mclName ) {
        CollectionSpace commCS = db.getCollectionSpace( SdbTestBase.csName );
        commCS.createCollection( mclName, ( BSONObject ) JSON.parse(
                "{ ShardingKey: { a: 1 }, ShardingType: 'range', IsMainCL: true }" ) );
        for ( int i = 0; i < SCLNUM; i++ ) {
            String sclName = mclName + "_" + i;
            commCS.createCollection( sclName );
        }
    }

    public static void createMclAndScl( Sequoiadb db, String mclName,
            String clGroup ) {
        CollectionSpace commCS = db.getCollectionSpace( SdbTestBase.csName );
        commCS.createCollection( mclName,
                ( BSONObject ) JSON.parse(
                        "{ ShardingKey: { a: 1 }, ShardingType: 'range', "
                                + "IsMainCL: true, Group: '" + clGroup
                                + "', ReplSize: 0 }" ) );
        for ( int i = 0; i < SCLNUM; i++ ) {
            String sclName = mclName + "_" + i;
            commCS.createCollection( sclName, ( BSONObject ) JSON
                    .parse( "{ Group: '" + clGroup + "', ReplSize: 0 }" ) );
        }
    }

    public static void attachAllScl( Sequoiadb db, String mclName ) {
        DBCollection mcl = db.getCollectionSpace( SdbTestBase.csName )
                .getCollection( mclName );
        int rangeStart = 0;
        for ( int i = 0; i < SCLNUM; i++ ) {
            int rangeEnd = rangeStart + RANGE_WIDTH;
            String sclFullName = SdbTestBase.csName + "." + mclName + "_" + i;
            mcl.attachCollection( sclFullName,
                    ( BSONObject ) JSON.parse( "{ LowBound: { a: " + rangeStart
                            + " }, " + "UpBound: { a: " + rangeEnd + " } }" ) );
            rangeStart += RANGE_WIDTH;
        }
    }

    public static void dropMclAndScl( Sequoiadb db, String mclName ) {
        CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
        for ( int i = 0; i < SCLNUM; i++ ) {
            String sclName = mclName + "_" + i;
            cs.dropCollection( sclName );
        }
        cs.dropCollection( mclName );
    }

    public static void checkIntegrated( Sequoiadb db, String mclName ) {
        CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
        if ( !cs.isCollectionExist( mclName ) ) {
            Assert.fail( "mcl: " + mclName + " not found" );
        }
        for ( int i = 0; i < SCLNUM; i++ ) {
            String sclName = mclName + "_" + i;
            if ( !cs.isCollectionExist( sclName ) ) {
                Assert.fail( "scl: " + sclName + " not found" );
            }
        }
    }

    public static void checkAttached( Sequoiadb db, String mclName,
            int attachedSclCnt ) {
        try {
            CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
            DBCollection mcl = cs.getCollection( mclName );
            int randomNum = ( new Random() ).nextInt( RANGE_WIDTH );
            for ( int i = 0; i < attachedSclCnt; i++ ) {
                mcl.insert( "{ a: " + randomNum + " }" );
                randomNum += RANGE_WIDTH;
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public static void checkDetached( Sequoiadb db, String mclName,
            int detachedSclCnt ) throws ReliabilityException {
        try {
            CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
            // check scl detached
            int randomNum = ( new Random() ).nextInt( RANGE_WIDTH );
            DBCollection mcl = cs.getCollection( mclName );
            for ( int i = 0; i < detachedSclCnt; i++ ) {
                String sclName = mclName + "_" + i;
                DBCollection scl = cs.getCollection( sclName );
                scl.insert( "{ a: 1 }" );
                int valueInSclRange = randomNum + i * RANGE_WIDTH;
                try {
                    mcl.insert( "{ a: " + valueInSclRange + " }" );
                    throw new ReliabilityException(
                            sclName + " not detached!" );
                } catch ( BaseException e ) {
                    // -135 SDB_CAT_NO_MATCH_CATALOG 无法找到匹配的编目信息
                    if ( e.getErrorCode() != -135 ) {
                        throw e;
                    }
                }
            }
            // check scl not detached yet
            for ( int i = detachedSclCnt + 1; i < SCLNUM; i++ ) {
                int valueInSclRange = randomNum + i * RANGE_WIDTH;
                mcl.insert( "{ a: " + valueInSclRange + " }" );
            }
        } catch ( BaseException e ) {
            Assert.fail( e.getMessage() );
        }
    }

    public static void checkConsistency( GroupMgr groupMgr )
            throws ReliabilityException {
        int checkTimes = 20;
        int checkInterval = 500; // 0.5s
        boolean success = false;
        String lastErrMsg = null;
        for ( int t = 0; t < checkTimes; ++t ) {
            success = true;
            groupMgr.refresh();
            GroupWrapper cataGroup = groupMgr.getGroupById( 1 );
            List< String > urls = cataGroup.getAllUrls();
            List< List< BSONObject > > resList = new ArrayList< List< BSONObject > >();
            // get catalog info from all catalog nodes
            for ( String url : urls ) {
                Sequoiadb cataDB = new Sequoiadb( url, "", "" );
                DBCollection cl = cataDB.getCollectionSpace( "SYSCAT" )
                        .getCollection( "SYSCOLLECTIONS" );
                DBCursor cursor = cl.query( null, null,
                        ( BSONObject ) JSON.parse( "{ _id: 1 }" ), null );
                List< BSONObject > res = new ArrayList< BSONObject >();
                while ( cursor.hasNext() ) {
                    res.add( cursor.getNext() );
                }
                cursor.close();
                cataDB.close();
                resList.add( res );
            }
            // check catalog count
            if ( resList.get( 0 ).size() != resList.get( 1 ).size()
                    || resList.get( 1 ).size() != resList.get( 2 ).size() ) {
                lastErrMsg = "";
                lastErrMsg += "catalog count is different between catalog nodes! \n";
                lastErrMsg += resList.get( 0 ).size() + " "
                        + resList.get( 1 ).size() + " "
                        + resList.get( 2 ).size() + "\n";
                lastErrMsg += "checkConsistency failed. see details on console";
                success = false;
            }
            // check catalog content
            List< BSONObject > srcList = resList.get( 0 );
            for ( int i = 1; i < resList.size(); i++ ) {
                List< BSONObject > dstList = resList.get( i );
                for ( int j = 0; j < srcList.size(); j++ ) {
                    if ( !srcList.get( j ).equals( dstList.get( j ) ) ) {
                        lastErrMsg = "";
                        lastErrMsg += "records below are different! ";
                        lastErrMsg += urls.get( 0 ) + " : " + srcList.get( j );
                        lastErrMsg += urls.get( i ) + " : " + dstList.get( j );
                        lastErrMsg += "checkConsistency failed. see details on console";
                        success = false;
                        break;
                    }
                }
                if ( !success ) {
                    break;
                }
            }

            if ( success ) {
                break;
            }

            try {
                Thread.sleep( checkInterval );
            } catch ( InterruptedException e ) {
                // ignore
            }
        }

        if ( !success ) {
            throw new ReliabilityException( lastErrMsg );
        }
    }

    public static String getKeyStack( Exception e, Object classObj ) {
        StringBuffer stackBuffer = new StringBuffer();
        StackTraceElement[] stackElements = e.getStackTrace();
        for ( int i = 0; i < stackElements.length; i++ ) {
            if ( stackElements[ i ].toString()
                    .contains( classObj.getClass().getName() ) ) {
                stackBuffer.append( stackElements[ i ].toString() )
                        .append( "\r\n" );
            }
        }
        String str = stackBuffer.toString();
        if ( str.length() >= 2 ) {
            return str.substring( 0, str.length() - 2 );
        } else {
            return str;
        }
    }
}
