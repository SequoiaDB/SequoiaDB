/*
 * Copyright (c) 2011 IOMark.CO.CC All rights reserved.
 *
 * The contents of this file are subject to the terms of the IOMARK LICENSE.
 * You may not modify or re-distribute the contents of this file.
 *
 */
package iomark.TPCCRunner ;

import java.io.FileInputStream ;
import java.io.FileNotFoundException ;
import java.io.IOException ;
import java.sql.Connection ;
import java.sql.DatabaseMetaData ;
import java.sql.DriverManager ;
import java.sql.SQLException ;
import java.sql.Statement ;
import java.util.Date ;
import java.util.Properties ;
import java.util.concurrent.CountDownLatch ;

/**
 * Data loader
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class Loader implements Config {
    private static Connection conn = null ;
    private static Statement stmt = null ;
    private static Date startDate ;
    private static Date endDate ;
    private static String driver ;
    private static String url ;
    private static String username ;
    private static String password ;
    private static int threadCount = 8 ;

    public static void main( String[] args ) {
        if ( args.length != 1 ) {
            System.err
                    .println( "USAGE: java TPCCRunner.LoadData [properties file]" ) ;
            System.exit( -1 ) ;
        }
        Properties ini = new Properties() ;
        try {
            ini.load( new FileInputStream( args[0] ) ) ;
        } catch ( FileNotFoundException e ) {
            e.printStackTrace() ;
        } catch ( IOException e ) {
            e.printStackTrace() ;
        }
        driver = ini.getProperty( "driver" ) ;
        url = ini.getProperty( "url" ) ;
        username = ini.getProperty( "user" ) ;
        password = ini.getProperty( "password" ) ;
        threadCount = Integer.parseInt( ini.getProperty( "threads" ) ) ;
        int numWarehouses = Integer.parseInt( ini.getProperty( "warehouses" ) ) ;
        initJDBC() ;
        truncateTable( "item" ) ;
        truncateTable( "warehouse" ) ;
        truncateTable( "stock" ) ;
        truncateTable( "district" ) ;
        truncateTable( "customer" ) ;
        truncateTable( "history" ) ;
        truncateTable( "oorder" ) ;
        truncateTable( "order_line" ) ;
        truncateTable( "new_order" ) ;
        startDate = new java.util.Date() ;
        System.out.println( "------------- LoadData Start Date = " + startDate
                + "-------------" ) ;
        long startTimeMS = System.currentTimeMillis() ;
        CountDownLatch countDownLatch = new CountDownLatch( threadCount ) ;
        int numWarehousePerThread = ( numWarehouses - ( numWarehouses % threadCount ) )
                / threadCount ;
        int numItemPerThread = ( configItemCount - ( configItemCount % threadCount ) )
                / threadCount ;
        for ( int i = 0; i < threadCount; i++ ) {
            if ( i != threadCount - 1 ) {
                new Thread( new LoaderThread( i, countDownLatch, driver, url,
                        username, password, configCommitCount, numWarehouses, i
                                * numWarehousePerThread + 1,
                        i * numWarehousePerThread + numWarehousePerThread, i
                                * numItemPerThread + 1, i * numItemPerThread
                                + numItemPerThread, configDistPerWhse,
                        configCustPerDist, Util.genRandomSeed( Thread
                                .currentThread().getId() ) ) ).start() ;
            } else {
                new Thread( new LoaderThread( i, countDownLatch, driver, url,
                        username, password, configCommitCount, numWarehouses, i
                                * numWarehousePerThread + 1, i
                                * numWarehousePerThread + numWarehousePerThread
                                + numWarehouses % threadCount, i
                                * numItemPerThread + 1, i * numItemPerThread
                                + numItemPerThread + configItemCount
                                % threadCount, configDistPerWhse,
                        configCustPerDist, Util.genRandomSeed( Thread
                                .currentThread().getId() ) ) ).start() ;
            }
            try {
                Thread.sleep( 1000 ) ;
            } catch ( InterruptedException e ) {
                e.printStackTrace() ;
            }
        }
        try {
            countDownLatch.await() ;
        } catch ( InterruptedException e1 ) {
            e1.printStackTrace() ;
        }
        long endTimeMS = System.currentTimeMillis() ;
        endDate = new java.util.Date() ;
        System.out.println( "" ) ;
        System.out
                .println( "------------- LoadJDBC Statistics --------------------" ) ;
        System.out.println( "     Start Time = " + startDate ) ;
        System.out.println( "       End Time = " + endDate ) ;
        System.out.println( "       Run Time = " + ( endTimeMS - startTimeMS )
                + " Milliseconds" ) ;
        System.out
                .println( "------------------------------------------------------" ) ;

        try {
            if ( conn != null ) {
                conn.close() ;
            }
        } catch ( SQLException e ) {
            e.printStackTrace() ;
        }
    }

    static void truncateTable( String strTable ) {
        String DBMS = "" ;
        try {
            DatabaseMetaData metaData = conn.getMetaData() ;
            DBMS = metaData.getDatabaseProductName().toLowerCase() ;
        } catch ( SQLException e ) {
            System.out.println( "Problem determining database product name: "
                    + e ) ;
        }
        System.out.println( "Truncating '" + strTable + "' ..." ) ;
        try {
            if ( DBMS.startsWith( "db2" ) ) {
                stmt.execute( "DELETE FROM " + strTable ) ;
            } else {
                stmt.execute( "TRUNCATE TABLE " + strTable ) ;
            }
            conn.commit() ;
        } catch ( SQLException se ) {
            System.out.println( se.getMessage() ) ;
            try {
                conn.rollback() ;
            } catch ( SQLException e ) {
                e.printStackTrace() ;
            }
        }

    }

    static void initJDBC() {
        try {
            Class.forName( driver ) ;
            conn = DriverManager.getConnection( url, username, password ) ;
            conn.setAutoCommit( false ) ;
            stmt = conn.createStatement() ;
        } catch ( SQLException se ) {
            System.out.println( se.getMessage() ) ;
            transRollback() ;
        } catch ( Exception e ) {
            e.printStackTrace() ;
            transRollback() ;
        }
    }

    static void transRollback() {
        try {
            conn.rollback() ;
        } catch ( SQLException se ) {
            System.out.println( se.getMessage() ) ;
        }

    }
}
