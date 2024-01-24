package com.sequoiadb.transaction.jdbc2mysql.common;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.logging.Logger;

import com.sequoiadb.transaction.common.TransUtil;

public class TransferLargeJDBCTh extends TransferJDBCTh {

    private static final Logger log = Logger
            .getLogger( TransferLargeJDBCTh.class.getName() );

    public TransferLargeJDBCTh( String clName ) {
        super( clName );
    }

    public static void main( String[] args ) throws Exception {
        TransferJDBCTh.initTrans( "newCL", 10000 );
        TransferLargeJDBCTh th = new TransferLargeJDBCTh( "newCL" );
        th.exec();
    }

    @Override
    public void exec() throws Exception {
        // 模拟转账操作：开启事务，新增和删除1w条记录，更新1w条记录
        Connection conn = null;
        PreparedStatement ps1 = null;
        PreparedStatement ps2 = null;
        PreparedStatement ps3 = null;
        PreparedStatement ps4 = null;

        List< Integer > accountListA = new ArrayList<>();
        List< Integer > accountListB = new ArrayList<>();
        for ( int i = 0; i < getInsertNum(); i++ ) {
            accountListA.add( i );
            accountListB.add( i );
        }

        try {
            int count = 0;
            conn = TransDBCPUtils.getConnection();
            // Class.forName("com.mysql.jdbc.Driver");
            // conn = DriverManager.getConnection(
            // "jdbc:mysql://192.168.31.4:3306/bank?useSSL=true&useServerPrepStmts=true&cachePrepStmts=true&rewriteBatchedStatements=true",
            // "root", "");
            while ( count++ < 10 ) {
                try {
                    if ( !TransUtil.runFlag ) {
                        log.info( "transfer thread " + getName()
                                + " stop in 10s" );
                        break;
                    }

                    Collections.shuffle( accountListA );
                    Collections.shuffle( accountListB );
                    conn.setAutoCommit( false );

                    String sql1 = "update " + getClName()
                            + " set balance=balance-?, th_hashcode=? where account=?";
                    String sql2 = "update " + getClName()
                            + " set balance=balance+?, th_hashcode=? where account=?";
                    String sql3 = "insert into " + getClName()
                            + " values(NULL, ?, ?)";
                    String sql4 = "delete from " + getClName()
                            + " where account=?";
                    ps1 = conn.prepareStatement( sql1 );
                    ps2 = conn.prepareStatement( sql2 );
                    ps3 = conn.prepareStatement( sql3 );
                    ps4 = conn.prepareStatement( sql4 );

                    ps1.clearBatch();
                    ps2.clearBatch();

                    for ( int i = 0; i < 5000; i++ ) {
                        int accountA = accountListA.get( i );
                        int accountB = accountListB.get( i );
                        int transAmount = ( int ) ( Math.random() * 200 );

                        ps2.setInt( 1, transAmount );
                        ps2.setString( 2, hashCode() + "|"
                                + new Date( System.currentTimeMillis() )
                                        .toString()
                                + "|" + conn.hashCode() + "|"
                                + ps2.hashCode() );
                        ps2.setInt( 3, accountB );
                        ps2.addBatch();

                        ps1.setInt( 1, transAmount );
                        ps1.setString( 2, String.valueOf( hashCode() ) );
                        ps1.setInt( 3, accountA );
                        ps1.addBatch();

                        if ( i % 1000 == 0 && i != 0 ) {
                            int ret2[] = ps2.executeBatch();
                            int ret1[] = ps1.executeBatch();
                            ps2.clearBatch();
                            ps1.clearBatch();
                            System.out.println( "ret1: " + ret1.length );
                            System.out.println( "ret2: " + ret2.length );
                            System.out.println( getName() + hashCode()
                                    + " executeBatch update " + i );
                        }
                    }
                    int ret1[] = ps1.executeBatch();
                    int ret2[] = ps2.executeBatch();
                    System.out.println( "ret1: " + ret1.length );
                    System.out.println( "ret2: " + ret2.length );

                    for ( int i = getInsertNum(); i < getInsertNum()
                            + 10000; i++ ) {
                        ps3.setInt( 1, 10000 );
                        ps3.setInt( 2, i );
                        ps3.addBatch();
                        if ( i != 0 && 0 == i % 1000 ) {
                            ps3.executeBatch();
                            ps3.clearBatch();
                            System.out.println( getName() + hashCode()
                                    + " executeBatch insert " + i );
                        }
                    }
                    ps3.executeBatch();

                    for ( int i = getInsertNum(); i < getInsertNum()
                            + 10000; i++ ) {
                        ps4.setInt( 1, i );
                        ps4.addBatch();
                        if ( i != 0 && 0 == i % 1000 ) {
                            ps4.executeBatch();
                            ps4.clearBatch();
                            System.out.println( getName() + hashCode()
                                    + " executeBatch delete " + i );
                        }
                    }
                    ps4.executeBatch();
                    conn.commit();
                } finally {
                    TransDBCPUtils.release( null, ps1, ps2, ps3, ps4 );
                }
            }
        } catch ( SQLException e ) {
            conn.rollback();
            e.printStackTrace();
        } finally {
            TransDBCPUtils.release( conn, ps1, ps2 );
        }
    }
}
