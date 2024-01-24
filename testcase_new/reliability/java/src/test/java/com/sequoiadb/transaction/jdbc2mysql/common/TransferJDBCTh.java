package com.sequoiadb.transaction.jdbc2mysql.common;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.logging.Logger;

import org.testng.Assert;

import com.sequoiadb.task.OperateTask;
import com.sequoiadb.transaction.common.TransUtil;

public class TransferJDBCTh extends OperateTask {
    private String clName;
    private static int insertNum;
    private static final Logger log = Logger
            .getLogger( TransferJDBCTh.class.getName() );

    public TransferJDBCTh( String clName ) {
        this.clName = clName;
    }

    protected String getClName() {
        return clName;
    }

    protected int getInsertNum() {
        return insertNum;
    }

    public static void initTrans( String clName, int insertNum )
            throws Exception {
        Connection conn = null;
        PreparedStatement ps = null;
        TransferJDBCTh.insertNum = insertNum;
        try {
            conn = TransDBCPUtils.getConnection();
            if ( isCLExist( clName ) ) {
                ps = conn.prepareStatement( "drop table " + clName );
                ps.execute();
                ps.close();
            }

            ps = conn.prepareStatement( "create table " + clName
                    + "(th_hashcode varchar(100), balance int, account int primary key)" );
            ps.execute();

            insertData( clName, insertNum );
        } finally {
            TransDBCPUtils.release( conn, ps );
        }
    }

    public static void finiTrans( String clName ) throws SQLException {
        Connection conn = null;
        PreparedStatement ps = null;
        try {
            conn = TransDBCPUtils.getConnection();
            if ( isCLExist( clName ) ) {
                ps = conn.prepareStatement( "drop table " + clName );
                ps.execute();
            }
        } finally {
            TransDBCPUtils.release( conn, ps );
        }
    }

    public static boolean isCLExist( String clName ) throws SQLException {
        Connection conn = null;
        PreparedStatement ps = null;
        try {
            conn = TransDBCPUtils.getConnection();
            ps = conn.prepareStatement(
                    "select count(*) as count from information_schema.tables where table_name=? and table_schema='bank'" );
            ps.setString( 1, clName );
            ResultSet set = ps.executeQuery();

            if ( set.next() ) {
                int count = set.getInt( "count" );
                if ( count == 1 ) {
                    return true;
                } else {
                    return false;
                }
            }
        } finally {
            TransDBCPUtils.release( conn, ps );
        }
        return false;
    }

    private static void insertData( String clName, int insertNum )
            throws Exception {
        Connection conn = null;
        PreparedStatement ps = null;
        try {
            conn = TransDBCPUtils.getConnection();
            conn.setAutoCommit( false );
            ps = conn.prepareStatement(
                    "insert into " + clName + " values(NULL, ?, ?)" );
            for ( int i = 0; i < insertNum; i++ ) {
                ps.setInt( 1, 10000 );
                ps.setInt( 2, i );
                ps.addBatch();
                if ( i != 0 && 0 == i % 1000 ) {
                    ps.executeBatch();
                    ps.clearBatch();
                }
            }
            ps.executeBatch();
            conn.commit();

            ResultSet set = ps.executeQuery( "select * from " + clName );
            set.last();
            if ( insertNum != set.getRow() ) {
                throw new Exception( "Insert " + insertNum
                        + " records but found [" + set.getRow() + "]" );
            }
        } finally {
            TransDBCPUtils.release( conn, ps );
        }
    }

    public static void main( String[] args ) throws Exception {
        TransferJDBCTh.initTrans( "test", 10000 );
        TransferJDBCTh th = new TransferJDBCTh( "test" );
        th.exec();
        TransferJDBCTh.finiTrans( "test" );
    }

    public static void checkTransResult( String clName, int expSum )
            throws SQLException {
        Connection conn = null;
        PreparedStatement ps = null;
        try {
            conn = TransDBCPUtils.getConnection();
            ps = conn.prepareStatement(
                    "select sum(balance) as bals from " + clName );
            ResultSet set = ps.executeQuery();
            while ( set.next() ) {
                int sum = set.getInt( "bals" );
                log.info( "checkTransResult balance general ledger [" + sum
                        + "]" );
                Assert.assertEquals( sum, expSum );
            }
        } finally {
            TransDBCPUtils.release( conn, ps );
        }
    }

    @Override
    public void exec() throws Exception {
        // 模拟转账操作：开启事务，随机取一个账户转出value；随机取另一个账户转入value
        Connection conn = null;
        PreparedStatement ps1 = null;
        PreparedStatement ps2 = null;

        try {
            int count = 0;
            conn = TransDBCPUtils.getConnection();
            conn.setAutoCommit( false );
            String sql1 = "update " + clName
                    + " set balance=balance-? where account=?";
            String sql2 = "update " + clName
                    + " set balance=balance+? where account=?";
            ps1 = conn.prepareStatement( sql1 );
            ps2 = conn.prepareStatement( sql2 );
            while ( count++ < 1800 ) {
                try {
                    if ( !TransUtil.runFlag ) {
                        break;
                    }

                    int accountA = ( int ) ( Math.random() * 10000 );
                    int accountB = ( int ) ( Math.random() * 10000 );
                    int transAmount = ( int ) ( Math.random() * 200 );

                    ps1.setInt( 1, transAmount );
                    ps1.setInt( 2, accountA );
                    ps1.executeUpdate();

                    ps2.setInt( 1, transAmount );
                    ps2.setInt( 2, accountB );
                    ps2.executeUpdate();

                    conn.commit();
                } finally {
                    TransDBCPUtils.release( null, ps1, ps2 );
                }
            }
        } catch ( SQLException e ) {
        } finally {
            TransDBCPUtils.release( conn, ps1, ps2 );
        }
    }
}
