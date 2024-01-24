package com.sequoiadb.basicoperation;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class CappedCLUtils {

    /**
     * @param sdb
     * @param csName
     * @param clName
     * @throws BaseException
     */
    public static DBCollection createCL( Sequoiadb sdb, String csName,
            String clName, boolean isCapped ) throws BaseException {
        try {
            BSONObject options_cs = new BasicBSONObject();
            options_cs.put( "Capped", true );

            if ( isCapped == true ) {
                sdb.createCollectionSpace( csName, options_cs );// create
                                                                // cappedCS
            } else {
                sdb.createCollectionSpace( csName );// create commonCS
            }

        } catch ( BaseException e ) {
            if ( -33 != e.getErrorCode() ) {
                throw e;
            }
        }
        DBCollection cl = null;
        BSONObject options_cl = new BasicBSONObject();
        options_cl.put( "Capped", true );
        options_cl.put( "Size", 8192 );
        options_cl.put( "AutoIndexId", false );
        try {
            CollectionSpace cs = sdb.getCollectionSpace( csName );
            if ( cs.isCollectionExist( clName ) ) {
                cs.dropCollection( clName );
            }
            if ( isCapped == true ) {
                cl = cs.createCollection( clName, options_cl );// create
                                                               // cappedCL
            } else {
                cl = cs.createCollection( clName,
                        ( BSONObject ) JSON.parse( "{AutoIndexId:false}" ) );// create
                                                                             // commonCL
            }

        } catch ( BaseException e ) {
            if ( -22 != e.getErrorCode() ) {
                throw e;
            }
        }
        return cl;
    }

    /**
     * @param sdb
     * @param csName
     * @param clName
     * @throws BaseException
     */
    public static List< DBCollection > createMoreCappedCL( Sequoiadb sdb,
            String csName, String clName, int csNum, int clNum )
            throws BaseException {
        List< DBCollection > dbCollections = new ArrayList< DBCollection >();

        for ( int csNo = 1; csNo <= csNum; csNo++ ) {
            try {
                BSONObject options_cs = new BasicBSONObject();
                options_cs.put( "Capped", true );
                sdb.createCollectionSpace( csName + csNo, options_cs );
            } catch ( BaseException e ) {
                if ( -33 != e.getErrorCode() ) {
                    throw e;
                }
            }

            for ( int clNo = 1; clNo <= clNum; clNo++ ) {
                try {
                    CollectionSpace cs = sdb
                            .getCollectionSpace( csName + csNo );
                    BSONObject options_cl = new BasicBSONObject();
                    options_cl.put( "Capped", true );
                    options_cl.put( "Size", 8192 );
                    options_cl.put( "AutoIndexId", false );
                    DBCollection cl = cs.createCollection( clName + clNo,
                            options_cl );
                    dbCollections.add( cl );
                } catch ( BaseException e ) {
                    if ( -22 != e.getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
        return dbCollections;
    }

    /**
     * @param cl
     * @param strBuffer
     * @param obj
     * @throws BaseException
     */
    public static void insertRecords( DBCollection cl, BSONObject obj )
            throws BaseException {

        // has modified one case has 20 threads
        final int each_thread_recordNums = 100;
        try {
            for ( int i = 0; i < each_thread_recordNums; i++ ) {
                cl.insert( obj );// each cappedCL insert the same record
            }
        } catch ( BaseException e ) {
            throw e;
        }
    }

    /**
     * @param cl
     * @param stringLength
     * @param recordNums
     * @throws BaseException
     */
    public static void insertRecords( DBCollection cl, int stringLength,
            int recordNums ) throws BaseException {
        StringBuffer strBuffer = new StringBuffer();
        BSONObject obj = new BasicBSONObject();
        for ( int len = 0; len < stringLength; len++ ) {
            strBuffer.append( "a" );
        }
        obj.put( "a", strBuffer.toString() );

        try {
            for ( int i = 0; i < recordNums; i++ ) {
                cl.insert( obj );// each cappedCL insert the same record
            }

        } catch ( BaseException e ) {
            throw e;
        }
    }

    /**
     * get random length for records
     */
    public static int getRandomStringLength() {
        int minLength = 100 * 1024; // 100k
        int range = 100 * 1024;
        int stringLength = ( int ) ( minLength + Math.random() * range );// all
                                                                         // records
                                                                         // length
                                                                         // range
                                                                         // [100k,200k]
        return stringLength;
    }

    /**
     * check whether LogicalID is correctly
     * 
     * @param sdb
     * @param cl
     * @param stringLength
     * @throws BaseException
     */
    public static boolean checkLogicalID( DBCollection cl, int stringLength,
            String className ) throws BaseException {
        System.out.println(
                "--------" + className + " begin to check logicalId---------" );
        DBCursor queryCursor = null;

        try {
            int recordNo = 0;
            final int each_add_55 = 55; // add head length
            final int full_byte_4 = 4; // 4 bytes
            int blockCounts = 1; // init block
            final int block_max_32 = 33554396; // each block is up to 32m
            long expectId = 0;
            BSONObject orderBy = new BasicBSONObject();
            BSONObject selector = new BasicBSONObject();
            orderBy.put( "_id", 1 );
            selector.put( "_id", 1 );
            queryCursor = cl.query( null, selector, orderBy, null );
            while ( queryCursor.hasNext() ) {
                recordNo++;

                long actId = ( long ) queryCursor.getNext().get( "_id" );
                int recordLength = stringLength + each_add_55;

                recordLength = ( 0 == recordLength % full_byte_4 )
                        ? recordLength
                        : ( recordLength - recordLength % full_byte_4
                                + full_byte_4 );

                long nextRecordId = expectId + recordLength;
                if ( nextRecordId > ( blockCounts * block_max_32 ) ) { // if the
                                                                       // next
                                                                       // record
                                                                       // length
                                                                       // is up
                                                                       // to
                                                                       // current
                                                                       // block
                                                                       // size,it
                                                                       // will
                                                                       // be put
                                                                       // to the
                                                                       // next
                                                                       // block
                    expectId = blockCounts * block_max_32;
                    ++blockCounts;
                }

                if ( expectId != actId ) {
                    System.out.println( className + ": stringLength: "
                            + stringLength + " recordNo: " + recordNo
                            + " blockCounts: " + blockCounts + " expectId: "
                            + expectId + "  actId: " + actId );
                    return false;
                }
                expectId = actId + recordLength;// This expectId belongs to the
                                                // next record
            }
            return true;
        } catch ( BaseException e ) {
            e.printStackTrace();
            return false;
        } finally {
            System.out.println( "--------" + className
                    + " end to check logicalId---------" );
            queryCursor.close();
        }
    }

}
