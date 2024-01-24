/*
 * ExecJDBC - Command line program to process SQL DDL statements, from
 *             a text input file, to any JDBC Data Source
 *
 * Copyright (C) 2004-2016, Denis Lussier
 * Copyright (C) 2016, Jan Wieck
 *
 */

import java.io.* ;
import java.sql.* ;
import java.util.* ;

public class ExecJDBC {
    public static void main( String[] args ) {
        Connection conn = null ;
        Statement stmt = null ;
        String rLine = null ;
        StringBuffer sql = new StringBuffer() ;
        Properties ini = null ;
        try {
            ini = new Properties() ;
            ini.load( new FileInputStream( System.getProperty( "prop" ) ) ) ;
        } catch ( FileNotFoundException e ) {
            System.out.println( e.getMessage() ) ;
            e.printStackTrace() ;
            System.exit( 1 ) ; ;
        } catch ( IOException e ) {
            System.out.println( e.getMessage() ) ;
            e.printStackTrace() ;
            System.exit( 1 ) ; ;
        }

        try {
            final String keyDriver = "driver" ;
            if ( !ini.containsKey( keyDriver ) ) {
                throw new Exception(
                        String.format( "need config %s", keyDriver ) ) ;
            }
            // Register jdbcDriver
            Class.forName( ini.getProperty( keyDriver ) ) ;
        } catch ( ClassNotFoundException e ) {
            System.out.println( e.getMessage() ) ;
            e.printStackTrace() ;
            System.exit( 1 ) ; ;
        } catch ( Exception e ) {
            System.out.println( e.getMessage() ) ;
            e.printStackTrace() ;
            System.exit( 1 ) ; ;
        }

        final String keyConn = "conn" ;
        if ( !ini.containsKey( keyConn ) ) {
            System.out.println( String.format( "need config %s", keyConn ) ) ;
            return ;
        }
        String[] conns = ini.getProperty( keyConn ).split( "," ) ;
        for ( int idx = 0; idx < conns.length; ++idx ) {
            try {
                // make connection
                conn = DriverManager
                        .getConnection( conns[idx], ini.getProperty( "user" ),
                                ini.getProperty( "password" ) ) ;
                conn.setAutoCommit( true ) ;

                // Create Statement
                stmt = conn.createStatement() ;

                // Open inputFile
                BufferedReader in = new BufferedReader( new FileReader(
                        jTPCCUtil.getSysProp( "commandFile", null ) ) ) ;

                // loop thru input file and concatenate SQL statement fragments
                final String semicolon = ";" ;
                final String annotate = "--" ;
                final String NL = "\n" ;
                final String rowAnnotate = "\\;" ;
                final String rowAnnotateByEsc = "\\\\;" ;
                while ( ( rLine = in.readLine() ) != null ) {
                    String line = rLine.trim() ;
                    if ( line.length() != 0 ) {
                        if ( line.startsWith( annotate ) ) {
                            System.out.println( line ) ; // print comment line
                        } else {
                            if ( line.endsWith( rowAnnotate ) ) {
                                sql.append( line.replaceAll( rowAnnotateByEsc,
                                        semicolon ) ) ;
                                sql.append( NL ) ;
                            } else {
                                sql.append( line.replaceAll( rowAnnotateByEsc,
                                        semicolon ) ) ;
                                if ( line.endsWith( semicolon ) ) {
                                    String query = sql.toString() ;
                                    query = modifySql( query, ini ) ;
                                    execJDBC( stmt, query.substring( 0,
                                            query.length() - 1 ) ) ;
                                    sql = new StringBuffer() ;
                                } else {
                                    sql.append( NL ) ;
                                }
                            }
                        }
                    } // end if
                } // end while
                in.close() ;
            } catch ( IOException ie ) {
                System.out.println( ie.getMessage() ) ;
                System.exit( 1 ) ;
            } catch ( SQLException se ) {
                System.out.println( se.getMessage() ) ;
                System.exit( 1 ) ;
            } catch ( Exception e ) {
                e.printStackTrace() ;
                System.exit( 1 ) ;
                // exit Cleanly
            } finally {
                closeConn( conn ) ;
            } // end try
        } // end for
    } // end main

    static String modifySql( String sql, Properties ini ) throws Exception {
        final String patternStr = "coordAddrs);" ;
        final String iniKey = "sdburl" ;
        if ( sql.endsWith( patternStr ) ) {
            if ( !ini.containsKey( iniKey ) ) {
                throw new Exception( "need config coordaddress" ) ;
            }

            sql = sql.substring( 0, sql.indexOf( patternStr ) ) ;
            sql += "\'" ;
            sql += ini.getProperty( iniKey ) ;
            sql += "\'" ;
            sql += ",preferedinstance \'M\');" ;
        }

        return sql ;
    }

    static void closeConn( Connection conn ) {
        try {
            if ( conn != null )
                conn.close() ;
        } catch ( SQLException se ) {
            se.printStackTrace() ;
        } // end finally
    }

    static void execJDBC( Statement stmt, String query ) {
        System.out.println( query + ";" ) ;
        try {
            stmt.execute( query ) ;
        } catch ( SQLException se ) {
            System.out.println( query + ";" ) ;
            System.out.println( se.getMessage() ) ;
            System.exit( 1 ) ;
        } // end try
    } // end execJDBCCommand
} // end ExecJDBC Class
