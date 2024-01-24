package com.sequoiadb.transaction.jdbc2mysql.common;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.Properties;

import javax.sql.DataSource;

import org.apache.commons.dbcp2.BasicDataSource;

public class TransDBCPUtils {
    private static Properties connConf = new Properties();
    private static String driver = "driver";
    private static String url = "url";
    private static String userName = "username";
    private static String password = "password";
    private static String initialSize = "initialSize";
    private static BasicDataSource dataSource;

    private static void setProperties() {
        connConf.setProperty( driver, "com.mysql.jdbc.Driver" );
        connConf.setProperty( url,
                "jdbc:mysql://192.168.31.4:3306/bank?useSSL=true&useServerPrepStmts=true&cachePrepStmts=true&rewriteBatchedStatements=true" );
        connConf.setProperty( userName, "root" );
        connConf.setProperty( password, "" );
        connConf.setProperty( initialSize, "10" );
    }

    private static void init() {
        setProperties();
        dataSource = new BasicDataSource();
        dataSource.setDriverClassName( connConf.getProperty( driver ) );
        dataSource.setUrl( connConf.getProperty( url ) );
        dataSource.setUsername( connConf.getProperty( userName ) );
        dataSource.setPassword( connConf.getProperty( password ) );
        dataSource.setInitialSize(
                Integer.parseInt( connConf.getProperty( initialSize ) ) );
    }

    static {
        init();
    }

    public static DataSource getDataSource() {
        return dataSource;
    }

    public static Connection getConnection() throws SQLException {
        return dataSource.getConnection();
    }

    public static void release( Connection conn, PreparedStatement... ps )
            throws SQLException {
        for ( PreparedStatement statement : ps ) {
            if ( statement != null ) {
                statement.close();
            }
        }
        if ( conn != null ) {
            conn.close();
        }
    }
}
