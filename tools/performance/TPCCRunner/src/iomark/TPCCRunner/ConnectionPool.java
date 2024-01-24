/*
 * Copyright (c) 2011 IOMark.CO.CC All rights reserved.
 *
 * The contents of this file are subject to the terms of the IOMARK LICENSE.
 * You may not modify or re-distribute the contents of this file.
 *
 */
package iomark.TPCCRunner ;

import java.sql.Connection ;
import java.sql.DriverManager ;
import java.sql.SQLException ;
import java.sql.Statement ;
import java.util.LinkedList ;
import java.util.NoSuchElementException ;

/**
 * Database Connection Pool <BR>
 * 
 * NOTE: invoke destroy() before program termination
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 * 
 */
public class ConnectionPool {
    private String className ;
    private String url ;
    private String userName ;
    private String password ;
    private int maxConnections ;
    private int currentConnections = 0 ;

    /** store the connections recycled */
    private LinkedList< Connection > pool = new LinkedList< Connection >() ;

    /** store the connections coming out of this pool */
    private LinkedList< Connection > out = new LinkedList< Connection >() ;

    public ConnectionPool( String className, String url, String userName,
            String password, int maxConns ) throws ClassNotFoundException {
        Class.forName( className ) ;
        this.className = className ;
        this.url = url ;
        this.userName = userName ;
        this.password = password ;
        this.maxConnections = maxConns ;
    }

    public Connection getConnection() throws SQLException {
        Connection conn = null ;
        synchronized ( pool ) {
            if ( pool.size() > 0 ) {
                conn = pool.remove() ;
            } else {
                if ( this.currentConnections < this.maxConnections ) {
                    try {
                        conn = DriverManager.getConnection( url, userName,
                                password ) ;
                        if ( className.endsWith( "IfxDriver" ) ) {
                            Statement stmt = conn.createStatement() ;
                            stmt.execute( "SET ISOLATION TO COMMITTED READ" ) ;
                            stmt.execute( "SET LOCK MODE TO WAIT 15" ) ;
                            stmt.close() ;
                        }
                        conn.setAutoCommit( false ) ;
                        this.currentConnections++ ;
                    } catch ( SQLException e ) {
                        System.out.println( e ) ;
                        throw e ;
                    }
                } else {
                    try {
                        while ( pool.size() <= 0 ) {
                            pool.wait() ;
                        }
                        conn = pool.remove() ;
                    } catch ( InterruptedException e ) {
                        System.err.println( e ) ;
                    } catch ( NoSuchElementException e1 ) {
                        System.err.println( Thread.currentThread().getName()
                                + " - Pool Size: " + pool.size() ) ;
                    }
                }
            }
            out.add( conn ) ;
            return conn ;
        }

    }

    public boolean recycle( Connection conn ) {
        synchronized ( pool ) {
            if ( out.remove( conn ) == true ) {
                pool.add( conn ) ;
                pool.notify() ;
                return true ;
            } else {
                return false ;
            }
        }
    }

    public synchronized void destroy() {
        try {
            while ( this.currentConnections > 0 ) {
                if ( pool.size() > 0 ) {
                    pool.remove().close() ;
                    this.currentConnections-- ;
                } else {
                    this.wait() ;
                }
            }
        } catch ( Exception e ) {
            System.err.println( e ) ;
        }
    }
}
