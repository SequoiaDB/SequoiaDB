/*
 * Copyright (c) 2011 IOMark.CO.CC All rights reserved.
 *
 * The contents of this file are subject to the terms of the IOMARK LICENSE.
 * You may not modify or re-distribute the contents of this file.
 *
 */
package iomark.TPCCRunner ;

import java.io.* ;
import java.sql.* ;
import java.util.* ;

/**
 * SQL Runner
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class RunSQL {
    public static void main( String[] args ) {
        Connection conn = null ;
        Statement stmt = null ;
        String rLine = null ;
        StringBuffer sql = new StringBuffer() ;
        java.util.Date now = null ;

        String propertiesFile = null ;
        String sqlFile = null ;

        if ( args.length != 2 ) {
            System.err
                    .println( "USAGE: java TPCCRunner.RunSQL [properties file] [SQL file]" ) ;
            System.exit( -1 ) ;
        } else {
            propertiesFile = args[0] ;
            sqlFile = args[1] ;
        }

        now = new java.util.Date() ;
        System.out.println( "------------- ExecJDBC Start Date = " + now
                + "-------------" ) ;

        try {
            Properties ini = new Properties() ;
            ini.load( new FileInputStream( propertiesFile ) ) ;
            System.out.println( "driver=" + ini.getProperty( "driver" ) ) ;
            System.out.println( "url=" + ini.getProperty( "url" ) ) ;
            System.out.println( "user=" + ini.getProperty( "user" ) ) ;
            System.out.println( "password=******" ) ;
            Class.forName( ini.getProperty( "driver" ) ) ;
            conn = DriverManager.getConnection( ini.getProperty( "url" ),
                    ini.getProperty( "user" ), ini.getProperty( "password" ) ) ;
            conn.setAutoCommit( true ) ;
            stmt = conn.createStatement() ;
            BufferedReader in = new BufferedReader( new FileReader( sqlFile ) ) ;
            System.out
                    .println( "-------------------------------------------------\n" ) ;

            while ( ( rLine = in.readLine() ) != null ) {
                String line = rLine.trim() ;
                if ( line.length() == 0 ) {
                    System.out.println( "" ) ;
                } else {
                    if ( line.startsWith( "--" ) ) {
                        System.out.println( line ) ;
                    } else {
                        sql.append( line ) ;
                        if ( line.endsWith( ";" ) ) {
                            execJDBC( stmt, sql ) ;
                            sql = new StringBuffer() ;
                        } else {
                            sql.append( "\n" ) ;
                        }
                    }
                }
            }
            in.close() ;
        } catch ( IOException ie ) {
            System.out.println( ie.getMessage() ) ;
        } catch ( SQLException se ) {
            System.out.println( se.getMessage() ) ;

        } catch ( Exception e ) {
            e.printStackTrace() ;
        } finally {
            try {
                if ( conn != null )
                    conn.rollback() ;
                conn.close() ;
            } catch ( SQLException se ) {
                se.printStackTrace() ;
            }
        }
        now = new java.util.Date() ;
        System.out.println( "" ) ;
        System.out.println( "------------- ExecJDBC End Date = " + now
                + "-------------" ) ;
    }

    private static void execJDBC( Statement stmt, StringBuffer sql ) {
        System.out.println( "\n" + sql ) ;
        try {

            long startTimeMS = new java.util.Date().getTime() ;
            stmt.execute( sql.toString().replace( ';', ' ' ) ) ;
            long runTimeMS = ( new java.util.Date().getTime() ) + 1
                    - startTimeMS ;
            System.out.println( "-- SQL Success: Runtime = " + runTimeMS
                    + " ms --" ) ;
        } catch ( SQLException se ) {
            String msg = null ;
            msg = se.getMessage() ;
            System.out
                    .println( "-- SQL Runtime Exception -----------------------------------------" ) ;
            System.out.println( "DBMS SqlCode=" + se.getErrorCode()
                    + "  DBMS Msg=" ) ;
            System.out
                    .println( "  "
                            + msg
                            + "\n"
                            + "------------------------------------------------------------------\n" ) ;
        }
    }
}
