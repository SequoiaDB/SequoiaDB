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
import java.sql.PreparedStatement ;
import java.sql.SQLException ;
import java.sql.Timestamp ;
import java.util.Random ;
import java.util.concurrent.CountDownLatch ;

/**
 * Data loader thread
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class LoaderThread implements Runnable {
    private int threadID ;
    private String driver ;
    private String url ;
    private String username ;
    private String password ;
    private Connection conn ;
    private PreparedStatement distPrepStmt ;
    private PreparedStatement itemPrepStmt ;
    private PreparedStatement custPrepStmt ;
    private PreparedStatement histPrepStmt ;
    private PreparedStatement ordrPrepStmt ;
    private PreparedStatement orlnPrepStmt ;
    private PreparedStatement nworPrepStmt ;
    private PreparedStatement stckPrepStmt ;
    private PreparedStatement whsePrepStmt ;
    private Random gen ;
    private int configCommitCount ;
    private int numWarehouses ;
    private int warehouseBegin ;
    private int warehouseEnd ;
    private int itemBegin ;
    private int itemEnd ;
    private int configDistPerWhse ;
    private int configCustPerDist ;
    private CountDownLatch countDownLatch ;

    LoaderThread( int threadID, CountDownLatch countDownLatch, String driver,
            String url, String username, String password,
            int configCommitCount, int numWarehouses, int warehouseBegin,
            int warehouseEnd, int itemBegin, int itemEnd,
            int configDistPerWhse, int configCustPerDist, long randomseed ) {
        this.threadID = threadID ;
        this.countDownLatch = countDownLatch ;
        this.driver = driver ;
        this.url = url ;
        this.username = username ;
        this.password = password ;
        this.configCommitCount = configCommitCount ;
        this.numWarehouses = numWarehouses ;
        this.warehouseBegin = warehouseBegin ;
        this.warehouseEnd = warehouseEnd ;
        this.itemBegin = itemBegin ;
        this.itemEnd = itemEnd ;
        this.configDistPerWhse = configDistPerWhse ;
        this.configCustPerDist = configCustPerDist ;
        this.gen = new Random( randomseed ) ;
    }

    public void run() {
        long start = System.currentTimeMillis() ;
        initJDBC() ;
        long insertCounts = 0 ;
        try {
            insertCounts = loadWhse( warehouseBegin, warehouseEnd ) ;
            insertCounts += loadDist( warehouseBegin, warehouseEnd,
                    configDistPerWhse ) ;
            insertCounts += loadItem( itemBegin, itemEnd ) ;
            transCommit() ;
        } catch ( SQLException e1 ) {
            e1.printStackTrace() ;
            transRollback() ;
        }
        insertCounts += loadStock( itemBegin, itemEnd, numWarehouses ) ;
        insertCounts += loadCust( warehouseBegin, warehouseEnd,
                configDistPerWhse, configCustPerDist ) ;
        insertCounts += loadOrder( warehouseBegin, warehouseEnd,
                configDistPerWhse, configCustPerDist ) ;
        try {
            if ( conn != null ) {
                conn.close() ;
            }
        } catch ( SQLException e ) {
            e.printStackTrace() ;
        }
        long end = System.currentTimeMillis() ;
        System.out
                .printf(
                        "%-23s - thread %-2d insert end, inserted %d rows, run %d seconds\n",
                        new Timestamp( System.currentTimeMillis() ), threadID,
                        insertCounts, ( end - start ) / 1000 ) ;
        countDownLatch.countDown() ;
    }

    private void transRollback() {
        try {
            conn.rollback() ;
        } catch ( SQLException se ) {
            se.printStackTrace() ;
        }

    }

    private void transCommit() {
        try {
            conn.commit() ;
        } catch ( SQLException se ) {
            se.printStackTrace() ;
            transRollback() ;
        }
    }

    private void initJDBC() {

        try {
            Class.forName( driver ) ;
            conn = DriverManager.getConnection( url, username, password ) ;
            conn.setAutoCommit( false ) ;
            distPrepStmt = conn
                    .prepareStatement( "INSERT INTO district "
                            + " (d_id, d_w_id, d_ytd, d_tax, d_next_o_id, d_name, d_street_1, d_street_2, d_city, d_state, d_zip) "
                            + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;
            itemPrepStmt = conn.prepareStatement( "INSERT INTO item "
                    + " (i_id, i_name, i_price, i_data, i_im_id) "
                    + "VALUES (?, ?, ?, ?, ?)" ) ;

            custPrepStmt = conn
                    .prepareStatement( "INSERT INTO customer "
                            + " (c_id, c_d_id, c_w_id, "
                            + "c_discount, c_credit, c_last, c_first, c_credit_lim, "
                            + "c_balance, c_ytd_payment, c_payment_cnt, c_delivery_cnt, "
                            + "c_street_1, c_street_2, c_city, c_state, c_zip, "
                            + "c_phone, c_since, c_middle, c_data) "
                            + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;

            histPrepStmt = conn.prepareStatement( "INSERT INTO history "
                    + " (h_c_id, h_c_d_id, h_c_w_id, " + "h_d_id, h_w_id, "
                    + "h_date, h_amount, h_data) "
                    + "VALUES (?, ?, ?, ?, ?, ?, ?, ?)" ) ;

            ordrPrepStmt = conn.prepareStatement( "INSERT INTO oorder "
                    + " (o_id, o_w_id,  o_d_id, o_c_id, "
                    + "o_carrier_id, o_ol_cnt, o_all_local, o_entry_d) "
                    + "VALUES (?, ?, ?, ?, ?, ?, ?, ?)" ) ;

            orlnPrepStmt = conn.prepareStatement( "INSERT INTO order_line "
                    + " (ol_w_id, ol_d_id, ol_o_id, "
                    + "ol_number, ol_i_id, ol_delivery_d, "
                    + "ol_amount, ol_supply_w_id, ol_quantity, ol_dist_info) "
                    + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;

            nworPrepStmt = conn.prepareStatement( "INSERT INTO new_order "
                    + " (no_w_id, no_d_id, no_o_id) " + "VALUES (?, ?, ?)" ) ;

            stckPrepStmt = conn
                    .prepareStatement( "INSERT INTO stock "
                            + " (s_i_id, s_w_id, s_quantity, s_ytd, s_order_cnt, s_remote_cnt, s_data, "
                            + "s_dist_01, s_dist_02, s_dist_03, s_dist_04, s_dist_05, "
                            + "s_dist_06, s_dist_07, s_dist_08, s_dist_09, s_dist_10) "
                            + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;

            whsePrepStmt = conn
                    .prepareStatement( "INSERT INTO warehouse "
                            + " (w_id, w_ytd, w_tax, w_name, w_street_1, w_street_2, w_city, w_state, w_zip) "
                            + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;

        } catch ( SQLException se ) {
            se.printStackTrace() ;
            transRollback() ;
        } catch ( Exception e ) {
            e.printStackTrace() ;
            transRollback() ;
        }
    }

    private int loadItem( int insertBegin, int insertEnd ) throws SQLException {
        int k = 0 ;
        int randPct = 0 ;
        int len = 0 ;
        int startORIGINAL = 0 ;

        TableItem item = new TableItem() ;
        for ( int i = insertBegin; i <= insertEnd; i++ ) {

            item.i_id = i ;
            item.i_name = Util.randomStr( Util.randomNumber( 14, 24, gen ) ) ;
            item.i_price = ( float ) ( Util.randomNumber( 100, 10000, gen ) / 100.0 ) ;
            randPct = Util.randomNumber( 1, 100, gen ) ;
            len = Util.randomNumber( 26, 50, gen ) ;
            if ( randPct > 10 ) {
                /* 90% of time i_data is a random string of length [26 .. 50] */
                item.i_data = Util.randomStr( len ) ;
            } else {
                /* 10% of time i_data has "ORIGINAL" crammed somewhere in middle */
                startORIGINAL = Util.randomNumber( 2, ( len - 8 ), gen ) ;
                item.i_data = Util.randomStr( startORIGINAL - 1 ) + "ORIGINAL"
                        + Util.randomStr( len - startORIGINAL - 9 ) ;
            }

            item.i_im_id = Util.randomNumber( 1, 10000, gen ) ;

            k++ ;
            itemPrepStmt.setLong( 1, item.i_id ) ;
            itemPrepStmt.setString( 2, item.i_name ) ;
            itemPrepStmt.setDouble( 3, item.i_price ) ;
            itemPrepStmt.setString( 4, item.i_data ) ;
            itemPrepStmt.setLong( 5, item.i_im_id ) ;
            itemPrepStmt.addBatch() ;

            if ( ( k % configCommitCount ) == 0 ) {
                itemPrepStmt.executeBatch() ;
                itemPrepStmt.clearBatch() ;
                transCommit() ;
            }
        }
        itemPrepStmt.executeBatch() ;
        // itemPrepStmt.clearBatch() ;
        // transCommit() ;
        System.out.printf( "%-23s - thread %-2d end insert item_%d_%d\n",
                new Timestamp( System.currentTimeMillis() ), threadID,
                insertBegin, insertEnd ) ;
        return k ;
    }

    private int loadWhse( int insertBegin, int insertEnd ) throws SQLException {
        int k = 0 ;

        TableWarehouse warehouse = new TableWarehouse() ;
        for ( int i = insertBegin; i <= insertEnd; i++ ) {
            warehouse.w_id = i ;
            warehouse.w_ytd = 300000 ;
            // random within [0.0000 .. 0.2000]
            warehouse.w_tax = ( float ) ( ( Util.randomNumber( 0, 2000, gen ) ) / 10000.0 ) ;

            warehouse.w_name = Util.randomStr( Util.randomNumber( 6, 10, gen ) ) ;
            warehouse.w_street_1 = Util.randomStr( Util.randomNumber( 10, 20,
                    gen ) ) ;
            warehouse.w_street_2 = Util.randomStr( Util.randomNumber( 10, 20,
                    gen ) ) ;
            warehouse.w_city = Util
                    .randomStr( Util.randomNumber( 10, 20, gen ) ) ;
            warehouse.w_state = Util.randomStr( 3 ).toUpperCase() ;
            warehouse.w_zip = "123456789" ;

            whsePrepStmt.setLong( 1, warehouse.w_id ) ;
            whsePrepStmt.setDouble( 2, warehouse.w_ytd ) ;
            whsePrepStmt.setDouble( 3, warehouse.w_tax ) ;
            whsePrepStmt.setString( 4, warehouse.w_name ) ;
            whsePrepStmt.setString( 5, warehouse.w_street_1 ) ;
            whsePrepStmt.setString( 6, warehouse.w_street_2 ) ;
            whsePrepStmt.setString( 7, warehouse.w_city ) ;
            whsePrepStmt.setString( 8, warehouse.w_state ) ;
            whsePrepStmt.setString( 9, warehouse.w_zip ) ;
            whsePrepStmt.addBatch() ;
            k++ ;
            if ( ( k % configCommitCount ) == 0 ) {
                whsePrepStmt.executeBatch() ;
                whsePrepStmt.clearBatch() ;
                transCommit() ;
            }
        } // end for
        whsePrepStmt.executeBatch() ;
        //whsePrepStmt.clearBatch() ;
        //transCommit() ;
        System.out.printf( "%-23s - thread %-2d end insert warehouse_%d_%d\n",
                new Timestamp( System.currentTimeMillis() ), threadID,
                insertBegin, insertEnd ) ;
        return k ;

    } // end loadWhse()

    private int loadStock( int insertBegin, int insertEnd, int whseKount ) {

        int k = 0 ;
        int randPct = 0 ;
        int len = 0 ;
        int startORIGINAL = 0 ;

        try {

            TableStock stock = new TableStock() ;

            for ( int i = insertBegin; i <= insertEnd; i++ ) {

                for ( int w = 1; w <= whseKount; w++ ) {

                    stock.s_i_id = i ;
                    stock.s_w_id = w ;
                    stock.s_quantity = Util.randomNumber( 10, 100, gen ) ;
                    stock.s_ytd = 0 ;
                    stock.s_order_cnt = 0 ;
                    stock.s_remote_cnt = 0 ;

                    // s_data
                    randPct = Util.randomNumber( 1, 100, gen ) ;
                    len = Util.randomNumber( 26, 50, gen ) ;
                    if ( randPct > 10 ) {
                        // 90% of time i_data isa random string of length [26 ..
                        // 50]
                        stock.s_data = Util.randomStr( len ) ;
                    } else {
                        // 10% of time i_data has "ORIGINAL" crammed somewhere
                        // in middle
                        startORIGINAL = Util.randomNumber( 2, ( len - 8 ), gen ) ;
                        stock.s_data = Util.randomStr( startORIGINAL - 1 )
                                + "ORIGINAL"
                                + Util.randomStr( len - startORIGINAL - 9 ) ;
                    }

                    stock.s_dist_01 = Util.randomStr( 24 ) ;
                    stock.s_dist_02 = Util.randomStr( 24 ) ;
                    stock.s_dist_03 = Util.randomStr( 24 ) ;
                    stock.s_dist_04 = Util.randomStr( 24 ) ;
                    stock.s_dist_05 = Util.randomStr( 24 ) ;
                    stock.s_dist_06 = Util.randomStr( 24 ) ;
                    stock.s_dist_07 = Util.randomStr( 24 ) ;
                    stock.s_dist_08 = Util.randomStr( 24 ) ;
                    stock.s_dist_09 = Util.randomStr( 24 ) ;
                    stock.s_dist_10 = Util.randomStr( 24 ) ;

                    k++ ;

                    stckPrepStmt.setLong( 1, stock.s_i_id ) ;
                    stckPrepStmt.setLong( 2, stock.s_w_id ) ;
                    stckPrepStmt.setDouble( 3, stock.s_quantity ) ;
                    stckPrepStmt.setDouble( 4, stock.s_ytd ) ;
                    stckPrepStmt.setLong( 5, stock.s_order_cnt ) ;
                    stckPrepStmt.setLong( 6, stock.s_remote_cnt ) ;
                    stckPrepStmt.setString( 7, stock.s_data ) ;
                    stckPrepStmt.setString( 8, stock.s_dist_01 ) ;
                    stckPrepStmt.setString( 9, stock.s_dist_02 ) ;
                    stckPrepStmt.setString( 10, stock.s_dist_03 ) ;
                    stckPrepStmt.setString( 11, stock.s_dist_04 ) ;
                    stckPrepStmt.setString( 12, stock.s_dist_05 ) ;
                    stckPrepStmt.setString( 13, stock.s_dist_06 ) ;
                    stckPrepStmt.setString( 14, stock.s_dist_07 ) ;
                    stckPrepStmt.setString( 15, stock.s_dist_08 ) ;
                    stckPrepStmt.setString( 16, stock.s_dist_09 ) ;
                    stckPrepStmt.setString( 17, stock.s_dist_10 ) ;
                    stckPrepStmt.addBatch() ;
                    if ( ( k % configCommitCount ) == 0 ) {
                        stckPrepStmt.executeBatch() ;
                        stckPrepStmt.clearBatch() ;
                        transCommit() ;
                    }
                } // end for [w]

            } // end for [i]

            stckPrepStmt.executeBatch() ;
            //stckPrepStmt.clearBatch() ;
            transCommit() ;
            System.out.printf( "%-23s - thread %-2d end insert stock_%d_%d\n",
                    new Timestamp( System.currentTimeMillis() ), threadID,
                    insertBegin, insertEnd ) ;

        } catch ( SQLException se ) {
            se.printStackTrace() ;
            SQLException se1 = se.getNextException() ;
            System.out.println( se1.getMessage() ) ;
            transRollback() ;
        } catch ( Exception e ) {
            e.printStackTrace() ;
            transRollback() ;
        }
        return k ;
    } // end loadStock()

    private int loadDist( int insertBegin, int insertEnd, int distWhseKount )
            throws SQLException {

        int k = 0 ;

        TableDistrict district = new TableDistrict() ;

        for ( int w = insertBegin; w <= insertEnd; w++ ) {

            for ( int d = 1; d <= distWhseKount; d++ ) {

                district.d_id = d ;
                district.d_w_id = w ;
                district.d_ytd = 30000 ;

                // random within [0.0000 .. 0.2000]
                district.d_tax = ( float ) ( ( Util.randomNumber( 0, 2000, gen ) ) / 10000.0 ) ;

                district.d_next_o_id = 3001 ;
                district.d_name = Util.randomStr( Util
                        .randomNumber( 6, 10, gen ) ) ;
                district.d_street_1 = Util.randomStr( Util.randomNumber( 10,
                        20, gen ) ) ;
                district.d_street_2 = Util.randomStr( Util.randomNumber( 10,
                        20, gen ) ) ;
                district.d_city = Util.randomStr( Util.randomNumber( 10, 20,
                        gen ) ) ;
                district.d_state = Util.randomStr( 3 ).toUpperCase() ;
                district.d_zip = "123456789" ;

                k++ ;

                distPrepStmt.setLong( 1, district.d_id ) ;
                distPrepStmt.setLong( 2, district.d_w_id ) ;
                distPrepStmt.setDouble( 3, district.d_ytd ) ;
                distPrepStmt.setDouble( 4, district.d_tax ) ;
                distPrepStmt.setLong( 5, district.d_next_o_id ) ;
                distPrepStmt.setString( 6, district.d_name ) ;
                distPrepStmt.setString( 7, district.d_street_1 ) ;
                distPrepStmt.setString( 8, district.d_street_2 ) ;
                distPrepStmt.setString( 9, district.d_city ) ;
                distPrepStmt.setString( 10, district.d_state ) ;
                distPrepStmt.setString( 11, district.d_zip ) ;
                distPrepStmt.addBatch() ;
                if ( ( k % configCommitCount ) == 0 ) {
                    distPrepStmt.executeBatch() ;
                    distPrepStmt.clearBatch() ;
                    transCommit() ;
                }

            } // end for [d]

        } // end for [w]
        distPrepStmt.executeBatch() ;
        //distPrepStmt.clearBatch() ;
        transCommit() ;
        System.out.printf( "%-23s - thread %-2d end insert district_%d_%d\n",
                new Timestamp( System.currentTimeMillis() ), threadID,
                insertBegin, insertEnd ) ;

        return ( k ) ;

    } // end loadDist()

    private int loadCust( int insertBegin, int insertEnd, int distWhseKount,
            int custDistKount ) {

        int k = 0 ;
        TableCustomer customer = new TableCustomer() ;
        TableHistory history = new TableHistory() ;
        Timestamp sysdate ;

        try {

            for ( int w = insertBegin; w <= insertEnd; w++ ) {

                for ( int d = 1; d <= distWhseKount; d++ ) {

                    for ( int c = 1; c <= custDistKount; c++ ) {

                        sysdate = new java.sql.Timestamp(
                                System.currentTimeMillis() ) ;

                        customer.c_id = c ;
                        customer.c_d_id = d ;
                        customer.c_w_id = w ;

                        // discount is random between [0.0000 ... 0.5000]
                        customer.c_discount = ( float ) ( Util.randomNumber( 1,
                                5000, gen ) / 10000.0 ) ;

                        if ( Util.randomNumber( 1, 100, gen ) <= 10 ) {
                            customer.c_credit = "BC" ; // 10% Bad Credit
                        } else {
                            customer.c_credit = "GC" ; // 90% Good Credit
                        }

                        if ( customer.c_id <= 1000 ) {
                            customer.c_last = Util
                                    .getLastName( customer.c_id - 1 ) ;
                        } else {
                            customer.c_last = Util.getLastName( gen ) ;
                        }

                        customer.c_first = Util.randomStr( Util.randomNumber(
                                8, 16, gen ) ) ;
                        customer.c_credit_lim = 50000 ;

                        customer.c_balance = -10 ;
                        customer.c_ytd_payment = 10 ;
                        customer.c_payment_cnt = 1 ;
                        customer.c_delivery_cnt = 0 ;

                        customer.c_street_1 = Util.randomStr( Util
                                .randomNumber( 10, 20, gen ) ) ;
                        customer.c_street_2 = Util.randomStr( Util
                                .randomNumber( 10, 20, gen ) ) ;
                        customer.c_city = Util.randomStr( Util.randomNumber(
                                10, 20, gen ) ) ;
                        customer.c_state = Util.randomStr( 3 ).toUpperCase() ;
                        customer.c_zip = "123456789" ;

                        customer.c_phone = "(732)744-1700" ;

                        customer.c_since = sysdate.getTime() ;
                        customer.c_middle = "OE" ;
                        customer.c_data = Util.randomStr( Util.randomNumber(
                                300, 500, gen ) ) ;

                        history.h_c_id = c ;
                        history.h_c_d_id = d ;
                        history.h_c_w_id = w ;
                        history.h_d_id = d ;
                        history.h_w_id = w ;
                        history.h_date = sysdate.getTime() ;
                        history.h_amount = 10 ;
                        history.h_data = Util.randomStr( Util.randomNumber( 10,
                                24, gen ) ) ;

                        k = k + 2 ;

                        custPrepStmt.setLong( 1, customer.c_id ) ;
                        custPrepStmt.setLong( 2, customer.c_d_id ) ;
                        custPrepStmt.setLong( 3, customer.c_w_id ) ;
                        custPrepStmt.setDouble( 4, customer.c_discount ) ;
                        custPrepStmt.setString( 5, customer.c_credit ) ;
                        custPrepStmt.setString( 6, customer.c_last ) ;
                        custPrepStmt.setString( 7, customer.c_first ) ;
                        custPrepStmt.setDouble( 8, customer.c_credit_lim ) ;
                        custPrepStmt.setDouble( 9, customer.c_balance ) ;
                        custPrepStmt.setDouble( 10, customer.c_ytd_payment ) ;
                        custPrepStmt.setDouble( 11, customer.c_payment_cnt ) ;
                        custPrepStmt.setDouble( 12, customer.c_delivery_cnt ) ;
                        custPrepStmt.setString( 13, customer.c_street_1 ) ;
                        custPrepStmt.setString( 14, customer.c_street_2 ) ;
                        custPrepStmt.setString( 15, customer.c_city ) ;
                        custPrepStmt.setString( 16, customer.c_state ) ;
                        custPrepStmt.setString( 17, customer.c_zip ) ;
                        custPrepStmt.setString( 18, customer.c_phone ) ;

                        Timestamp since = new Timestamp( customer.c_since ) ;
                        custPrepStmt.setTimestamp( 19, since ) ;
                        custPrepStmt.setString( 20, customer.c_middle ) ;
                        custPrepStmt.setString( 21, customer.c_data ) ;

                        custPrepStmt.addBatch() ;

                        histPrepStmt.setInt( 1, history.h_c_id ) ;
                        histPrepStmt.setInt( 2, history.h_c_d_id ) ;
                        histPrepStmt.setInt( 3, history.h_c_w_id ) ;

                        histPrepStmt.setInt( 4, history.h_d_id ) ;
                        histPrepStmt.setInt( 5, history.h_w_id ) ;
                        Timestamp hdate = new Timestamp( history.h_date ) ;
                        histPrepStmt.setTimestamp( 6, hdate ) ;
                        histPrepStmt.setDouble( 7, history.h_amount ) ;
                        histPrepStmt.setString( 8, history.h_data ) ;

                        histPrepStmt.addBatch() ;

                        if ( ( k % configCommitCount ) == 0 ) {

                            custPrepStmt.executeBatch() ;
                            histPrepStmt.executeBatch() ;
                            custPrepStmt.clearBatch() ;
                            histPrepStmt.clearBatch() ;
                            transCommit() ;
                        }

                    } // end for [c]

                } // end for [d]

            } // end for [w]

            custPrepStmt.executeBatch() ;
            histPrepStmt.executeBatch() ;
            //custPrepStmt.clearBatch() ;
            //histPrepStmt.clearBatch() ;
            transCommit() ;
            System.out.printf(
                    "%-23s - thread %-2d end insert customer_%d_%d\n",
                    new Timestamp( System.currentTimeMillis() ), threadID,
                    insertBegin, insertEnd ) ;
            System.out.printf(
                    "%-23s - thread %-2d end insert history_%d_%d\n",
                    new Timestamp( System.currentTimeMillis() ), threadID,
                    insertBegin, insertEnd ) ;
        } catch ( SQLException se ) {
            se.printStackTrace() ;
            transRollback() ;
        } catch ( Exception e ) {
            e.printStackTrace() ;
            transRollback() ;
        }

        return k ;

    }

    private int loadOrder( int insertBegin, int insertEnd, int distWhseKount,
            int custDistKount ) {

        int k = 0 ;
        try {

            TableOorder oorder = new TableOorder() ;
            TableNewOrder new_order = new TableNewOrder() ;
            TableOrderLine order_line = new TableOrderLine() ;
            OrderLoader orderLoader = new OrderLoader() ;

            for ( int w = insertBegin; w <= insertEnd; w++ ) {

                for ( int d = 1; d <= distWhseKount; d++ ) {

                    for ( int c = 1; c <= custDistKount; c++ ) {

                        oorder.o_id = c ;
                        oorder.o_w_id = w ;
                        oorder.o_d_id = d ;
                        oorder.o_c_id = Util.randomNumber( 1, custDistKount,
                                gen ) ;
                        oorder.o_carrier_id = Util.randomNumber( 1, 10, gen ) ;
                        oorder.o_ol_cnt = Util.randomNumber( 5, 15, gen ) ;
                        oorder.o_all_local = 1 ;
                        oorder.o_entry_d = System.currentTimeMillis() ;
                        k++ ;
                        orderLoader.insertOrder( ordrPrepStmt, oorder ) ;
                        // 900 rows in the NEW-ORDER table corresponding to the
                        // last
                        // 900 rows in the ORDER table for that district (i.e.,
                        // with
                        // NO_O_ID between 2,101 and 3,000)

                        if ( c > 2100 ) {
                            new_order.no_w_id = w ;
                            new_order.no_d_id = d ;
                            new_order.no_o_id = c ;
                            k++ ;
                            orderLoader
                                    .insertNewOrder( nworPrepStmt, new_order ) ;
                        } // end new order

                        for ( int l = 1; l <= oorder.o_ol_cnt; l++ ) {

                            order_line.ol_w_id = w ;
                            order_line.ol_d_id = d ;
                            order_line.ol_o_id = c ;
                            order_line.ol_number = l ; // ol_number
                            order_line.ol_i_id = Util.randomNumber( 1, 100000,
                                    gen ) ;
                            order_line.ol_delivery_d = oorder.o_entry_d ;

                            if ( order_line.ol_o_id < 2101 ) {
                                order_line.ol_amount = 0 ;
                            } else {
                                // random within [0.01 .. 9,999.99]
                                order_line.ol_amount = ( float ) ( Util
                                        .randomNumber( 1, 999999, gen ) / 100.0 ) ;
                            }

                            order_line.ol_supply_w_id = Util.randomNumber( 1,
                                    numWarehouses, gen ) ;
                            order_line.ol_quantity = 5 ;
                            order_line.ol_dist_info = Util.randomStr( 24 ) ;

                            k++ ;

                            orderLoader.insertOrderLine( orlnPrepStmt,
                                    order_line ) ;

                            if ( ( k % configCommitCount ) == 0 ) {
                                ordrPrepStmt.executeBatch() ;
                                nworPrepStmt.executeBatch() ;
                                orlnPrepStmt.executeBatch() ;
                                ordrPrepStmt.clearBatch() ;
                                nworPrepStmt.clearBatch() ;
                                orlnPrepStmt.clearBatch() ;
                                transCommit() ;
                            }

                        } // end for [l]

                    } // end for [c]

                } // end for [d]

            } // end for [w]

            ordrPrepStmt.executeBatch() ;
            nworPrepStmt.executeBatch() ;
            orlnPrepStmt.executeBatch() ;
            //ordrPrepStmt.clearBatch() ;
            //nworPrepStmt.clearBatch() ;
            //orlnPrepStmt.clearBatch() ;
            transCommit() ;
            System.out.printf( "%-23s - thread %-2d end insert oorder_%d_%d\n",
                    new Timestamp( System.currentTimeMillis() ), threadID,
                    insertBegin, insertEnd ) ;
            System.out.printf(
                    "%-23s - thread %-2d end insert new_order_%d_%d\n",
                    new Timestamp( System.currentTimeMillis() ), threadID,
                    insertBegin, insertEnd ) ;
            System.out.printf(
                    "%-23s - thread %-2d end insert order_status_%d_%d\n",
                    new Timestamp( System.currentTimeMillis() ), threadID,
                    insertBegin, insertEnd ) ;
        } catch ( SQLException se ) {
            se.printStackTrace() ;
            SQLException se1 = se.getNextException() ;
            System.out.println( se1.getMessage() ) ;
            transRollback() ;
        } catch ( Exception e ) {
            e.printStackTrace() ;
            transRollback() ;

        }
        return k ;
    }
}

class OrderLoader {
    public void insertOrder( PreparedStatement ordrPrepStmt, TableOorder oorder ) {
        try {
            ordrPrepStmt.setInt( 1, oorder.o_id ) ;
            ordrPrepStmt.setInt( 2, oorder.o_w_id ) ;
            ordrPrepStmt.setInt( 3, oorder.o_d_id ) ;
            ordrPrepStmt.setInt( 4, oorder.o_c_id ) ;
            ordrPrepStmt.setInt( 5, oorder.o_carrier_id ) ;
            ordrPrepStmt.setInt( 6, oorder.o_ol_cnt ) ;
            ordrPrepStmt.setInt( 7, oorder.o_all_local ) ;
            Timestamp entry_d = new java.sql.Timestamp( oorder.o_entry_d ) ;
            ordrPrepStmt.setTimestamp( 8, entry_d ) ;
            ordrPrepStmt.addBatch() ;

        } catch ( SQLException se ) {
            se.printStackTrace() ;
        } catch ( Exception e ) {
            e.printStackTrace() ;
        }
    }

    public void insertNewOrder( PreparedStatement nworPrepStmt,
            TableNewOrder new_order ) {
        try {
            nworPrepStmt.setInt( 1, new_order.no_w_id ) ;
            nworPrepStmt.setInt( 2, new_order.no_d_id ) ;
            nworPrepStmt.setInt( 3, new_order.no_o_id ) ;
            nworPrepStmt.addBatch() ;
        } catch ( SQLException se ) {
            se.printStackTrace() ;
        } catch ( Exception e ) {
            e.printStackTrace() ;
        }
    }

    public void insertOrderLine( PreparedStatement orlnPrepStmt,
            TableOrderLine order_line ) {

        try {
            orlnPrepStmt.setInt( 1, order_line.ol_w_id ) ;
            orlnPrepStmt.setInt( 2, order_line.ol_d_id ) ;
            orlnPrepStmt.setInt( 3, order_line.ol_o_id ) ;
            orlnPrepStmt.setInt( 4, order_line.ol_number ) ;
            orlnPrepStmt.setLong( 5, order_line.ol_i_id ) ;
            Timestamp delivery_d = new Timestamp( order_line.ol_delivery_d ) ;
            orlnPrepStmt.setTimestamp( 6, delivery_d ) ;
            orlnPrepStmt.setDouble( 7, order_line.ol_amount ) ;
            orlnPrepStmt.setLong( 8, order_line.ol_supply_w_id ) ;
            orlnPrepStmt.setDouble( 9, order_line.ol_quantity ) ;
            orlnPrepStmt.setString( 10, order_line.ol_dist_info ) ;
            orlnPrepStmt.addBatch() ;
        } catch ( SQLException se ) {
            se.printStackTrace() ;
        } catch ( Exception e ) {
            e.printStackTrace() ;
        }
    }
}
