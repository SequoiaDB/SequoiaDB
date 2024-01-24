/*
 * Copyright (c) 2011 IOMark.CO.CC All rights reserved.
 *
 * The contents of this file are subject to the terms of the IOMARK LICENSE.
 * You may not modify or re-distribute the contents of this file.
 *
 */
package iomark.TPCCRunner ;

import java.io.PrintStream ;
import java.io.PrintWriter ;
import java.io.StringWriter ;
import java.sql.Connection ;
import java.sql.PreparedStatement ;
import java.sql.ResultSet ;
import java.sql.SQLException ;
import java.sql.Timestamp ;
import java.util.Random ;
import java.util.Vector ;

/**
 * User thread
 * 
 * @author "Jarvis Wang"
 * @version 1.00
 */
public class User implements Config, Runnable {
    private String userName ;
    private ConnectionPool connectionPool = null ;
    private int userWarehouseID, userDistrictID ;
    private int paymentPercent, orderStatusPercent, deliveryPercent,
            stockLevelPercent ;
    private Slave parent ;
    private Random gen ;
    private int transactionCount = 1, numWarehouses ;
    private int result = 0 ;
    private boolean stopRunningSignal = false ;
    private int newOrderThinkSecond ;
    private int paymentThinkSecond ;
    private int orderStatusThinkSecond ;
    private int deliveryThinkSecond ;
    private int stockLevelThinkSecond ;
    private String databaseType ;
    private PrintStream printStreamErrors ;

    public User( String userName, int userWarehouseID, int userDistrictID,
            ConnectionPool connectionPool, int paymentPercent,
            int orderStatusPercent, int deliveryPercent, int stockLevelPercent,
            int numWarehouses, Slave parent, int newOrderThinkSecond,
            int paymentThinkSecond, int orderStatusThinkSecond,
            int deliveryThinkSecond, int stockLevelThinkSecond,
            String databaseType, PrintStream printStreamErrors )
            throws SQLException {
        this.newOrderThinkSecond = newOrderThinkSecond ;
        this.paymentThinkSecond = paymentThinkSecond ;
        this.orderStatusThinkSecond = orderStatusThinkSecond ;
        this.deliveryThinkSecond = deliveryThinkSecond ;
        this.stockLevelThinkSecond = stockLevelThinkSecond ;
        this.userName = userName ;
        this.connectionPool = connectionPool ;

        this.userWarehouseID = userWarehouseID ;
        this.userDistrictID = userDistrictID ;
        this.paymentPercent = paymentPercent ;
        this.orderStatusPercent = orderStatusPercent ;
        this.deliveryPercent = deliveryPercent ;
        this.stockLevelPercent = stockLevelPercent ;
        this.numWarehouses = numWarehouses ;
        this.parent = parent ;
        this.databaseType = databaseType ;
        this.printStreamErrors = printStreamErrors ;
    }

    public void run() {
        gen = new Random( Util.genRandomSeed( Thread.currentThread().getId() ) ) ;
        executeTransactions() ;
        parent.signalUserEnded( this ) ;
    }

    public void stopRunningWhenPossible() {
        stopRunningSignal = true ;
    }

    private void executeTransactions() {
        boolean stopRunning = false ;
        while ( !stopRunning ) {
            long transactionType = Util.randomNumber( 1, 100, gen ) ;
            String transactionTypeName = null ;
            long connectionStart = System.currentTimeMillis() ;
            Connection conn = null ;
            try {
                conn = connectionPool.getConnection() ;
            } catch ( SQLException e ) {
                logException( e ) ;
                break ;
            }
            try {
                long transactionStart = System.currentTimeMillis() ;
                if ( transactionType <= paymentPercent ) {
                    executeTransaction( PAYMENT, conn ) ;
                    transactionTypeName = "Payment" ;
                } else if ( transactionType <= paymentPercent
                        + stockLevelPercent ) {
                    executeTransaction( STOCK_LEVEL, conn ) ;
                    transactionTypeName = "Stock-Level" ;
                } else if ( transactionType <= paymentPercent
                        + stockLevelPercent + orderStatusPercent ) {
                    executeTransaction( ORDER_STATUS, conn ) ;
                    transactionTypeName = "Order-Status" ;
                } else if ( transactionType <= paymentPercent
                        + stockLevelPercent + orderStatusPercent
                        + deliveryPercent ) {
                    executeTransaction( DELIVERY, conn ) ;
                    transactionTypeName = "Delivery" ;
                } else {
                    executeTransaction( NEW_ORDER, conn ) ;
                    transactionTypeName = "New-Order" ;
                }
                long transactionEnd = System.currentTimeMillis() ;
                connectionPool.recycle( conn ) ;
                long connectionEnd = System.currentTimeMillis() ;

                parent.signalUserEndedTransaction( this.userName,
                        transactionTypeName, connectionEnd - connectionStart,
                        transactionEnd - transactionStart ) ;
            } catch ( Exception e ) {
                connectionPool.recycle( conn ) ;
            }

            if ( transactionTypeName == null ) {
                continue ;
            }
            if ( stopRunningSignal ) {
                stopRunning = true ;
            }
            try {
                if ( "Payment".equals( transactionTypeName ) ) {
                    Thread.sleep( paymentThinkSecond * 1000 ) ;
                } else if ( "Stock-Level".equals( transactionTypeName ) ) {
                    Thread.sleep( stockLevelThinkSecond * 1000 ) ;
                } else if ( "Order-Status".equals( transactionTypeName ) ) {
                    Thread.sleep( orderStatusThinkSecond * 1000 ) ;
                } else if ( "Delivery".equals( transactionTypeName ) ) {
                    Thread.sleep( deliveryThinkSecond * 1000 ) ;
                } else {
                    Thread.sleep( newOrderThinkSecond * 1000 ) ;
                }
            } catch ( Exception e ) {
                e.printStackTrace() ;
            }
        }
    }

    private int executeTransaction( int transaction, Connection conn )
            throws Exception {
        int result = 0 ;
        switch ( transaction ) {
        case NEW_ORDER:
            int districtID = Util.randomNumber( 1, configDistPerWhse, gen ) ;
            int customerID = Util.getCustomerID( gen ) ;

            int numItems = ( int ) Util.randomNumber( 5, 15, gen ) ;
            int[] itemIDs = new int[ numItems ] ;
            int[] supplierWarehouseIDs = new int[ numItems ] ;
            int[] orderQuantities = new int[ numItems ] ;
            int allLocal = 1 ;
            for ( int i = 0; i < numItems; i++ ) {
                itemIDs[i] = Util.getItemID( gen ) ;
                if ( Util.randomNumber( 1, 100, gen ) > 1 ) {
                    supplierWarehouseIDs[i] = userWarehouseID ;
                } else {
                    do {
                        supplierWarehouseIDs[i] = Util.randomNumber( 1,
                                numWarehouses, gen ) ;
                    } while ( supplierWarehouseIDs[i] == userWarehouseID
                            && numWarehouses > 1 ) ;
                    allLocal = 0 ;
                }
                orderQuantities[i] = Util.randomNumber( 1, 10, gen ) ;
            }

            // we need to cause 1% of the new orders to be rolled back.
            if ( Util.randomNumber( 1, 100, gen ) == 1 )
                itemIDs[numItems - 1] = -12345 ;
            newOrderTransaction( userWarehouseID, districtID, customerID,
                    numItems, allLocal, itemIDs, supplierWarehouseIDs,
                    orderQuantities, conn ) ;
            break ;

        case PAYMENT:
            districtID = Util.randomNumber( 1, 10, gen ) ;

            int x = Util.randomNumber( 1, 100, gen ) ;
            int customerDistrictID ;
            int customerWarehouseID ;
            if ( x <= 85 ) {
                customerDistrictID = districtID ;
                customerWarehouseID = userWarehouseID ;
            } else {
                customerDistrictID = Util.randomNumber( 1, 10, gen ) ;
                do {
                    customerWarehouseID = Util.randomNumber( 1, numWarehouses,
                            gen ) ;
                } while ( customerWarehouseID == userWarehouseID
                        && numWarehouses > 1 ) ;
            }

            long y = Util.randomNumber( 1, 100, gen ) ;
            boolean customerByName ;
            String customerLastName = null ;
            customerID = -1 ;
            customerByName = false ;
            customerID = Util.getCustomerID( gen ) ;

            float paymentAmount = ( float ) ( Util.randomNumber( 100, 500000,
                    gen ) / 100.0 ) ;
            paymentTransaction( userWarehouseID, customerWarehouseID,
                    paymentAmount, districtID, customerDistrictID, customerID,
                    customerLastName, customerByName, conn ) ;
            break ;

        case STOCK_LEVEL:
            int threshold = Util.randomNumber( 10, 20, gen ) ;
            stockLevelTransaction( userWarehouseID, userDistrictID, threshold,
                    conn ) ;
            break ;

        case ORDER_STATUS:
            districtID = Util.randomNumber( 1, 10, gen ) ;

            y = Util.randomNumber( 1, 100, gen ) ;
            customerLastName = null ;
            customerID = -1 ;
            if ( y <= 60 ) {
                customerByName = true ;
                customerLastName = Util.getLastName( gen ) ;
            } else {
                customerByName = false ;
                customerID = Util.getCustomerID( gen ) ;
            }
            orderStatusTransaction( userWarehouseID, districtID, customerID,
                    customerLastName, customerByName, conn ) ;
            break ;
        case DELIVERY:
            int orderCarrierID = Util.randomNumber( 1, 10, gen ) ;
            result = deliveryTransaction( userWarehouseID, orderCarrierID, conn ) ;
            break ;
        default:
            error( "EMPTY-TYPE" ) ;
            break ;
        }
        transactionCount++ ;

        return result ;
    }

    private int deliveryTransaction( int w_id, int o_carrier_id, Connection conn )
            throws Exception {
        int d_id, no_o_id, c_id ;
        float ol_total ;
        int[] orderIDs ;
        int skippedDeliveries = 0 ;
        boolean newOrderRemoved ;

        TableNewOrder new_order = new TableNewOrder() ;
        new_order.no_w_id = w_id ;
        PreparedStatement delivGetOrderId = null ;
        PreparedStatement delivDeleteNewOrder = null ;
        PreparedStatement delivGetCustId = null ;
        PreparedStatement delivUpdateCarrierId = null ;
        PreparedStatement delivUpdateDeliveryDate = null ;
        PreparedStatement delivSumOrderAmount = null ;
        PreparedStatement delivUpdateCustBalDelivCnt = null ;
        try {
            conn.setAutoCommit( false ) ;
            orderIDs = new int[ 10 ] ;
            for ( d_id = 1; d_id <= 10; d_id++ ) {
                new_order.no_d_id = d_id ;

                do {
                    no_o_id = -1 ;
                    if ( delivGetOrderId == null ) {
                        delivGetOrderId = conn
                                .prepareStatement( "SELECT no_o_id FROM new_order WHERE no_d_id = ?"
                                        + " AND no_w_id = ?"
                                        + " ORDER BY no_o_id ASC" ) ;
                    }
                    delivGetOrderId.setInt( 1, d_id ) ;
                    delivGetOrderId.setInt( 2, w_id ) ;
                    ResultSet rs = delivGetOrderId.executeQuery() ;
                    if ( rs.next() )
                        no_o_id = rs.getInt( "no_o_id" ) ;
                    orderIDs[( int ) d_id - 1] = no_o_id ;
                    rs.close() ;

                    newOrderRemoved = false ;
                    if ( no_o_id != -1 ) {
                        new_order.no_o_id = no_o_id ;
                        if ( delivDeleteNewOrder == null ) {
                            delivDeleteNewOrder = conn
                                    .prepareStatement( "DELETE FROM new_order"
                                            + " WHERE no_d_id = ?"
                                            + " AND no_w_id = ?"
                                            + " AND no_o_id = ?" ) ;
                        }
                        delivDeleteNewOrder.setInt( 1, d_id ) ;
                        delivDeleteNewOrder.setInt( 2, w_id ) ;
                        delivDeleteNewOrder.setInt( 3, no_o_id ) ;
                        result = delivDeleteNewOrder.executeUpdate() ;
                        if ( result > 0 )
                            newOrderRemoved = true ;
                    }
                } while ( no_o_id != -1 && !newOrderRemoved ) ;

                if ( no_o_id != -1 ) {
                    if ( delivGetCustId == null ) {
                        delivGetCustId = conn.prepareStatement( "SELECT o_c_id"
                                + " FROM oorder" + " WHERE o_id = ?"
                                + " AND o_d_id = ?" + " AND o_w_id = ?" ) ;
                    }
                    delivGetCustId.setInt( 1, no_o_id ) ;
                    delivGetCustId.setInt( 2, d_id ) ;
                    delivGetCustId.setInt( 3, w_id ) ;
                    ResultSet rs = delivGetCustId.executeQuery() ;

                    if ( !rs.next() )
                        throw new Exception( "O_ID=" + no_o_id + " O_D_ID="
                                + d_id + " O_W_ID=" + w_id + " not found!" ) ;
                    c_id = rs.getInt( "o_c_id" ) ;
                    rs.close() ;
                    if ( delivUpdateCarrierId == null ) {
                        delivUpdateCarrierId = conn
                                .prepareStatement( "UPDATE oorder SET o_carrier_id = ?"
                                        + " WHERE o_id = ?"
                                        + " AND o_d_id = ?"
                                        + " AND o_w_id = ?" ) ;
                    }
                    delivUpdateCarrierId.setInt( 1, o_carrier_id ) ;
                    delivUpdateCarrierId.setInt( 2, no_o_id ) ;
                    delivUpdateCarrierId.setInt( 3, d_id ) ;
                    delivUpdateCarrierId.setInt( 4, w_id ) ;
                    result = delivUpdateCarrierId.executeUpdate() ;

                    if ( result != 1 )
                        throw new Exception( "O_ID=" + no_o_id + " O_D_ID="
                                + d_id + " O_W_ID=" + w_id + " not found!" ) ;
                    if ( delivUpdateDeliveryDate == null ) {
                        delivUpdateDeliveryDate = conn
                                .prepareStatement( "UPDATE order_line SET ol_delivery_d = ?"
                                        + " WHERE ol_o_id = ?"
                                        + " AND ol_d_id = ?"
                                        + " AND ol_w_id = ?" ) ;
                    }
                    delivUpdateDeliveryDate.setTimestamp( 1, new Timestamp(
                            System.currentTimeMillis() ) ) ;
                    delivUpdateDeliveryDate.setInt( 2, no_o_id ) ;
                    delivUpdateDeliveryDate.setInt( 3, d_id ) ;
                    delivUpdateDeliveryDate.setInt( 4, w_id ) ;
                    result = delivUpdateDeliveryDate.executeUpdate() ;
                    if ( result == 0 )
                        throw new Exception( "OL_O_ID=" + no_o_id + " OL_D_ID="
                                + d_id + " OL_W_ID=" + w_id + " not found!" ) ;
                    if ( delivSumOrderAmount == null ) {
                        delivSumOrderAmount = conn
                                .prepareStatement( "SELECT SUM(ol_amount) AS ol_total"
                                        + " FROM order_line"
                                        + " WHERE ol_o_id = ?"
                                        + " AND ol_d_id = ?"
                                        + " AND ol_w_id = ?" ) ;
                    }
                    delivSumOrderAmount.setInt( 1, no_o_id ) ;
                    delivSumOrderAmount.setInt( 2, d_id ) ;
                    delivSumOrderAmount.setInt( 3, w_id ) ;
                    ResultSet rs1 = delivSumOrderAmount.executeQuery() ;

                    if ( !rs1.next() )
                        throw new Exception( "OL_O_ID=" + no_o_id + " OL_D_ID="
                                + d_id + " OL_W_ID=" + w_id + " not found!" ) ;
                    ol_total = rs1.getFloat( "ol_total" ) ;
                    rs1.close() ;
                    if ( delivUpdateCustBalDelivCnt == null ) {
                        delivUpdateCustBalDelivCnt = conn
                                .prepareStatement( "UPDATE customer SET c_balance = c_balance + ?"
                                        + ", c_delivery_cnt = c_delivery_cnt + 1"
                                        + " WHERE c_id = ?"
                                        + " AND c_d_id = ?"
                                        + " AND c_w_id = ?" ) ;
                    }
                    delivUpdateCustBalDelivCnt.setFloat( 1, ol_total ) ;
                    delivUpdateCustBalDelivCnt.setInt( 2, c_id ) ;
                    delivUpdateCustBalDelivCnt.setInt( 3, d_id ) ;
                    delivUpdateCustBalDelivCnt.setInt( 4, w_id ) ;
                    result = delivUpdateCustBalDelivCnt.executeUpdate() ;
                    if ( result == 0 )
                        throw new Exception( "C_ID=" + c_id + " C_W_ID=" + w_id
                                + " C_D_ID=" + d_id + " not found!" ) ;
                }
            }
            conn.commit() ;
        } catch ( Exception e ) {
            error( "DELIVERY" ) ;
            logException( e ) ;
            try {
                conn.rollback() ;
            } catch ( Exception e1 ) {
                error( "DELIVERY-ROLLBACK" ) ;
                logException( e1 ) ;
            }
            throw e ;
        } finally {
            try {
                if ( delivGetOrderId != null ) {
                    delivGetOrderId.close() ;
                }
                if ( delivDeleteNewOrder != null ) {
                    delivDeleteNewOrder.close() ;
                }
                if ( delivGetCustId != null ) {
                    delivGetCustId.close() ;
                }
                if ( delivUpdateCarrierId != null ) {
                    delivUpdateCarrierId.close() ;
                }
                if ( delivUpdateDeliveryDate != null ) {
                    delivUpdateDeliveryDate.close() ;
                }
                if ( delivSumOrderAmount != null ) {
                    delivSumOrderAmount.close() ;
                }
                if ( delivUpdateCustBalDelivCnt != null ) {
                    delivUpdateCustBalDelivCnt.close() ;
                }
            } catch ( SQLException e ) {
                e.printStackTrace() ;
            }
        }

        return skippedDeliveries ;
    }

    private void orderStatusTransaction( int w_id, int d_id, int c_id,
            String c_last, boolean c_by_name, Connection conn )
            throws Exception {
        int namecnt, o_id = -1, o_carrier_id = -1 ;
        float c_balance ;
        String c_first, c_middle ;
        java.sql.Date entdate = null ;
        Vector< String > orderLines = new Vector< String >() ;
        PreparedStatement ordStatCountCust = null ;
        PreparedStatement ordStatGetCust = null ;
        PreparedStatement ordStatGetCustBal = null ;
        PreparedStatement ordStatGetNewestOrd = null ;
        PreparedStatement ordStatGetOrder = null ;
        PreparedStatement ordStatGetOrderLines = null ;
        try {
            conn.setAutoCommit( false ) ;
            if ( c_by_name ) {
                if ( ordStatCountCust == null ) {
                    ordStatCountCust = conn
                            .prepareStatement( "SELECT count(*) AS namecnt FROM customer"
                                    + " WHERE c_last = ?"
                                    + " AND c_d_id = ?"
                                    + " AND c_w_id = ?" ) ;
                }
                ordStatCountCust.setString( 1, c_last ) ;
                ordStatCountCust.setInt( 2, d_id ) ;
                ordStatCountCust.setInt( 3, w_id ) ;
                ResultSet rs = ordStatCountCust.executeQuery() ;

                if ( !rs.next() )
                    throw new Exception( "C_LAST=" + c_last + " C_D_ID=" + d_id
                            + " C_W_ID=" + w_id + " not found!" ) ;
                namecnt = rs.getInt( "namecnt" ) ;
                rs.close() ;

                // pick the middle customer from the list of customers
                if ( ordStatGetCust == null ) {
                    ordStatGetCust = conn
                            .prepareStatement( "SELECT c_balance, c_first, c_middle, c_id FROM customer"
                                    + " WHERE c_last = ?"
                                    + " AND c_d_id = ?"
                                    + " AND c_w_id = ?"
                                    + " ORDER BY c_w_id, c_d_id, c_last, c_first" ) ;
                }
                ordStatGetCust.setString( 1, c_last ) ;
                ordStatGetCust.setInt( 2, d_id ) ;
                ordStatGetCust.setInt( 3, w_id ) ;
                ResultSet rs1 = ordStatGetCust.executeQuery() ;

                if ( !rs1.next() )
                    throw new Exception( "C_LAST=" + c_last + " C_D_ID=" + d_id
                            + " C_W_ID=" + w_id + " not found!" ) ;
                if ( namecnt % 2 == 1 )
                    namecnt++ ;
                for ( int i = 1; i < namecnt / 2; i++ )
                    rs1.next() ;
                c_id = rs1.getInt( "c_id" ) ;
                c_first = rs1.getString( "c_first" ) ;
                c_middle = rs1.getString( "c_middle" ) ;
                c_balance = rs1.getFloat( "c_balance" ) ;
                rs1.close() ;
            } else {
                if ( ordStatGetCustBal == null ) {
                    ordStatGetCustBal = conn
                            .prepareStatement( "SELECT c_balance, c_first, c_middle, c_last"
                                    + " FROM customer"
                                    + " WHERE c_id = ?"
                                    + " AND c_d_id = ?" + " AND c_w_id = ?" ) ;
                }
                ordStatGetCustBal.setInt( 1, c_id ) ;
                ordStatGetCustBal.setInt( 2, d_id ) ;
                ordStatGetCustBal.setInt( 3, w_id ) ;
                ResultSet rs = ordStatGetCustBal.executeQuery() ;

                if ( !rs.next() )
                    throw new Exception( "C_ID=" + c_id + " C_D_ID=" + d_id
                            + " C_W_ID=" + w_id + " not found!" ) ;
                c_last = rs.getString( "c_last" ) ;
                c_first = rs.getString( "c_first" ) ;
                c_middle = rs.getString( "c_middle" ) ;
                c_balance = rs.getFloat( "c_balance" ) ;
                rs.close() ;
            }

            // find the newest order for the customer
            if ( ordStatGetNewestOrd == null ) {
                ordStatGetNewestOrd = conn
                        .prepareStatement( "SELECT MAX(o_id) AS maxorderid FROM oorder"
                                + " WHERE o_w_id = ?"
                                + " AND o_d_id = ?"
                                + " AND o_c_id = ?" ) ;
            }
            ordStatGetNewestOrd.setInt( 1, w_id ) ;
            ordStatGetNewestOrd.setInt( 2, d_id ) ;
            ordStatGetNewestOrd.setInt( 3, c_id ) ;
            ResultSet rs = ordStatGetNewestOrd.executeQuery() ;

            if ( rs.next() ) {
                o_id = rs.getInt( "maxorderid" ) ;

                // retrieve the carrier & order date for the most recent order.
                if ( ordStatGetOrder == null ) {
                    ordStatGetOrder = conn
                            .prepareStatement( "SELECT o_carrier_id, o_entry_d"
                                    + " FROM oorder" + " WHERE o_w_id = ?"
                                    + " AND o_d_id = ?" + " AND o_c_id = ?"
                                    + " AND o_id = ?" ) ;
                }
                ordStatGetOrder.setInt( 1, w_id ) ;
                ordStatGetOrder.setInt( 2, d_id ) ;
                ordStatGetOrder.setInt( 3, c_id ) ;
                ordStatGetOrder.setInt( 4, o_id ) ;
                ResultSet rs1 = ordStatGetOrder.executeQuery() ;

                if ( rs1.next() ) {
                    o_carrier_id = rs1.getInt( "o_carrier_id" ) ;
                    entdate = rs1.getDate( "o_entry_d" ) ;
                }
                rs1.close() ;
            }
            rs.close() ;
            // retrieve the order lines for the most recent order
            if ( ordStatGetOrderLines == null ) {
                ordStatGetOrderLines = conn
                        .prepareStatement( "SELECT ol_i_id, ol_supply_w_id, ol_quantity,"
                                + " ol_amount, ol_delivery_d"
                                + " FROM order_line"
                                + " WHERE ol_o_id = ?"
                                + " AND ol_d_id =?" + " AND ol_w_id = ?" ) ;
            }
            ordStatGetOrderLines.setInt( 1, o_id ) ;
            ordStatGetOrderLines.setInt( 2, d_id ) ;
            ordStatGetOrderLines.setInt( 3, w_id ) ;
            rs = ordStatGetOrderLines.executeQuery() ;

            while ( rs.next() ) {
                StringBuffer orderLine = new StringBuffer() ;
                orderLine.append( "[" ) ;
                orderLine.append( rs.getLong( "ol_supply_w_id" ) ) ;
                orderLine.append( " - " ) ;
                orderLine.append( rs.getLong( "ol_i_id" ) ) ;
                orderLine.append( " - " ) ;
                orderLine.append( rs.getLong( "ol_quantity" ) ) ;
                orderLine.append( " - " ) ;
                orderLine.append( Util.formattedDouble( rs
                        .getDouble( "ol_amount" ) ) ) ;
                orderLine.append( " - " ) ;
                if ( rs.getDate( "ol_delivery_d" ) != null )
                    orderLine.append( rs.getDate( "ol_delivery_d" ) ) ;
                else
                    orderLine.append( "99-99-9999" ) ;
                orderLine.append( "]" ) ;
                orderLines.add( orderLine.toString() ) ;
            }
            rs.close() ;
            conn.commit() ;
        } catch ( Exception e ) {
            error( "ORDER-STATUS" ) ;
            logException( e ) ;
            throw e ;
        } finally {
            try {
                if ( ordStatCountCust != null ) {
                    ordStatCountCust.close() ;
                }
                if ( ordStatGetCust != null ) {
                    ordStatGetCust.close() ;
                }
                if ( ordStatGetCustBal != null ) {
                    ordStatGetCustBal.close() ;
                }
                if ( ordStatGetNewestOrd != null ) {
                    ordStatGetNewestOrd.close() ;
                }
                if ( ordStatGetOrder != null ) {
                    ordStatGetOrder.close() ;
                }
                if ( ordStatGetOrderLines != null ) {
                    ordStatGetOrderLines.close() ;
                }
            } catch ( SQLException e ) {
                e.printStackTrace() ;
            }
        }
    }

    private void newOrderTransaction( int w_id, int d_id, int c_id,
            int o_ol_cnt, int o_all_local, int[] itemIDs,
            int[] supplierWarehouseIDs, int[] orderQuantities, Connection conn )
            throws Exception {
        float c_discount, w_tax, d_tax = 0, i_price ;
        int d_next_o_id, o_id = -1, s_quantity ;
        String c_last = null, c_credit = null, i_name, i_data, s_data ;
        String s_dist_01, s_dist_02, s_dist_03, s_dist_04, s_dist_05 ;
        String s_dist_06, s_dist_07, s_dist_08, s_dist_09, s_dist_10, ol_dist_info = null ;
        float[] itemPrices = new float[ o_ol_cnt ] ;
        float[] orderLineAmounts = new float[ o_ol_cnt ] ;
        String[] itemNames = new String[ o_ol_cnt ] ;
        int[] stockQuantities = new int[ o_ol_cnt ] ;
        char[] brandGeneric = new char[ o_ol_cnt ] ;
        int ol_supply_w_id, ol_i_id, ol_quantity ;
        int s_remote_cnt_increment ;
        float ol_amount, total_amount = 0 ;
        boolean newOrderRowInserted ;

        TableWarehouse whse = new TableWarehouse() ;
        TableCustomer cust = new TableCustomer() ;
        TableDistrict dist = new TableDistrict() ;
        TableNewOrder nwor = new TableNewOrder() ;
        TableOorder ordr = new TableOorder() ;
        TableOrderLine orln = new TableOrderLine() ;
        TableStock stck = new TableStock() ;
        TableItem item = new TableItem() ;
        PreparedStatement stmtInsertOrderLine = null ;
        PreparedStatement stmtUpdateStock = null ;
        PreparedStatement stmtGetCustWhse = null ;
        PreparedStatement stmtGetDist = null ;
        PreparedStatement stmtInsertNewOrder = null ;
        PreparedStatement stmtUpdateDist = null ;
        PreparedStatement stmtInsertOOrder = null ;
        PreparedStatement stmtGetItem = null ;
        PreparedStatement stmtGetStock = null ;
        try {
            conn.setAutoCommit( false ) ;
            if ( stmtGetCustWhse == null ) {
                stmtGetCustWhse = conn
                        .prepareStatement( "SELECT c_discount, c_last, c_credit, w_tax"
                                + "  FROM customer, warehouse"
                                + " WHERE w_id = ? AND w_id = c_w_id"
                                + " AND c_d_id = ? AND c_id = ?" ) ;
            }
            stmtGetCustWhse.setInt( 1, w_id ) ;
            stmtGetCustWhse.setInt( 2, d_id ) ;
            stmtGetCustWhse.setInt( 3, c_id ) ;
            ResultSet rs = stmtGetCustWhse.executeQuery() ;
            if ( !rs.next() )
                throw new Exception( "W_ID=" + w_id + " C_D_ID=" + d_id
                        + " C_ID=" + c_id + " not found!" ) ;
            c_discount = rs.getFloat( "c_discount" ) ;
            c_last = rs.getString( "c_last" ) ;
            c_credit = rs.getString( "c_credit" ) ;
            w_tax = rs.getFloat( "w_tax" ) ;
            rs.close() ;
            newOrderRowInserted = false ;
            while ( !newOrderRowInserted ) {

                if ( stmtGetDist == null ) {
                    if ( "SQLServer".equals( databaseType ) ) {
                        stmtGetDist = conn
                                .prepareStatement( "SELECT d_next_o_id, d_tax FROM district"
                                        + " WHERE d_id = ? AND d_w_id = ? " ) ;
                    } else {
                        stmtGetDist = conn
                                .prepareStatement( "SELECT d_next_o_id, d_tax FROM district"
                                        + " WHERE d_id = ? AND d_w_id = ? FOR UPDATE" ) ;
                    }
                }

                stmtGetDist.setInt( 1, d_id ) ;
                stmtGetDist.setInt( 2, w_id ) ;
                ResultSet rs1 = stmtGetDist.executeQuery() ;
                if ( !rs1.next() )
                    throw new Exception( "D_ID=" + d_id + " D_W_ID=" + w_id
                            + " not found!" ) ;
                d_next_o_id = rs1.getInt( "d_next_o_id" ) ;
                d_tax = rs1.getFloat( "d_tax" ) ;
                rs1.close() ;
                o_id = d_next_o_id ;

                try {
                    if ( stmtInsertNewOrder == null ) {
                        stmtInsertNewOrder = conn
                                .prepareStatement( "INSERT INTO new_order (no_o_id, no_d_id, no_w_id) "
                                        + "VALUES ( ?, ?, ?)" ) ;
                    }
                    stmtInsertNewOrder.setInt( 1, o_id ) ;
                    stmtInsertNewOrder.setInt( 2, d_id ) ;
                    stmtInsertNewOrder.setInt( 3, w_id ) ;
                    stmtInsertNewOrder.executeUpdate() ;
                    newOrderRowInserted = true ;
                } catch ( SQLException e2 ) {
                }
            }
            if ( stmtUpdateDist == null ) {
                stmtUpdateDist = conn
                        .prepareStatement( "UPDATE district SET d_next_o_id = d_next_o_id + 1 "
                                + " WHERE d_id = ? AND d_w_id = ?" ) ;
            }
            stmtUpdateDist.setInt( 1, d_id ) ;
            stmtUpdateDist.setInt( 2, w_id ) ;
            result = stmtUpdateDist.executeUpdate() ;
            if ( result == 0 )
                throw new Exception(
                        "Error!! Cannot update next_order_id on DISTRICT for D_ID="
                                + d_id + " D_W_ID=" + w_id ) ;
            if ( stmtInsertOOrder == null ) {
                stmtInsertOOrder = conn
                        .prepareStatement( "INSERT INTO oorder "
                                + " (o_id, o_d_id, o_w_id, o_c_id, o_entry_d, o_ol_cnt, o_all_local)"
                                + " VALUES (?, ?, ?, ?, ?, ?, ?)" ) ;
            }
            stmtInsertOOrder.setInt( 1, o_id ) ;
            stmtInsertOOrder.setInt( 2, d_id ) ;
            stmtInsertOOrder.setInt( 3, w_id ) ;
            stmtInsertOOrder.setInt( 4, c_id ) ;
            stmtInsertOOrder.setTimestamp( 5,
                    new Timestamp( System.currentTimeMillis() ) ) ;
            stmtInsertOOrder.setInt( 6, o_ol_cnt ) ;
            stmtInsertOOrder.setInt( 7, o_all_local ) ;
            stmtInsertOOrder.executeUpdate() ;
            for ( int ol_number = 1; ol_number <= o_ol_cnt; ol_number++ ) {
                ol_supply_w_id = supplierWarehouseIDs[ol_number - 1] ;
                ol_i_id = itemIDs[ol_number - 1] ;
                ol_quantity = orderQuantities[ol_number - 1] ;

                if ( item.i_id == -12345 ) {
                    // an expected condition generated 1% of the time in the
                    // test data...
                    // we throw an illegal access exception and the transaction
                    // gets rolled back later on
                    throw new IllegalAccessException(
                            "Expected NEW-ORDER error condition excersing rollback functionality" ) ;
                }
                if ( stmtGetItem == null ) {
                    stmtGetItem = conn
                            .prepareStatement( "SELECT i_price, i_name , i_data FROM item WHERE i_id = ?" ) ;
                }
                stmtGetItem.setInt( 1, ol_i_id ) ;
                rs = stmtGetItem.executeQuery() ;
                if ( !rs.next() )
                    throw new IllegalAccessException( "I_ID=" + ol_i_id
                            + " not found!" ) ;
                i_price = rs.getFloat( "i_price" ) ;
                i_name = rs.getString( "i_name" ) ;
                i_data = rs.getString( "i_data" ) ;
                rs.close() ;
                rs = null ;

                itemPrices[ol_number - 1] = i_price ;
                itemNames[ol_number - 1] = i_name ;

                if ( stmtGetStock == null ) {
                    if ( "SQLServer".equals( databaseType ) ) {
                        stmtGetStock = conn
                                .prepareStatement( "SELECT s_quantity, s_data, s_dist_01, s_dist_02, s_dist_03, s_dist_04, s_dist_05, "
                                        + "       s_dist_06, s_dist_07, s_dist_08, s_dist_09, s_dist_10"
                                        + " FROM stock WHERE s_i_id = ? AND s_w_id = ? " ) ;
                    } else {
                        stmtGetStock = conn
                                .prepareStatement( "SELECT s_quantity, s_data, s_dist_01, s_dist_02, s_dist_03, s_dist_04, s_dist_05, "
                                        + "       s_dist_06, s_dist_07, s_dist_08, s_dist_09, s_dist_10"
                                        + " FROM stock WHERE s_i_id = ? AND s_w_id = ? FOR UPDATE" ) ;
                    }
                }
                stmtGetStock.setInt( 1, ol_i_id ) ;
                stmtGetStock.setInt( 2, ol_supply_w_id ) ;
                rs = stmtGetStock.executeQuery() ;
                if ( !rs.next() )
                    throw new Exception( "I_ID=" + ol_i_id + " not found!" ) ;
                s_quantity = rs.getInt( "s_quantity" ) ;
                s_data = rs.getString( "s_data" ) ;
                s_dist_01 = rs.getString( "s_dist_01" ) ;
                s_dist_02 = rs.getString( "s_dist_02" ) ;
                s_dist_03 = rs.getString( "s_dist_03" ) ;
                s_dist_04 = rs.getString( "s_dist_04" ) ;
                s_dist_05 = rs.getString( "s_dist_05" ) ;
                s_dist_06 = rs.getString( "s_dist_06" ) ;
                s_dist_07 = rs.getString( "s_dist_07" ) ;
                s_dist_08 = rs.getString( "s_dist_08" ) ;
                s_dist_09 = rs.getString( "s_dist_09" ) ;
                s_dist_10 = rs.getString( "s_dist_10" ) ;
                rs.close() ;
                rs = null ;

                stockQuantities[ol_number - 1] = s_quantity ;

                if ( s_quantity - ol_quantity >= 10 ) {
                    s_quantity -= ol_quantity ;
                } else {
                    s_quantity += -ol_quantity + 91 ;
                }
                if ( ol_supply_w_id == w_id ) {
                    s_remote_cnt_increment = 0 ;
                } else {
                    s_remote_cnt_increment = 1 ;
                }
                if ( stmtUpdateStock == null ) {
                    stmtUpdateStock = conn
                            .prepareStatement( "UPDATE stock SET s_quantity = ? , s_ytd = s_ytd + ?, s_remote_cnt = s_remote_cnt + ? "
                                    + " WHERE s_i_id = ? AND s_w_id = ?" ) ;
                }
                stmtUpdateStock.setInt( 1, s_quantity ) ;
                stmtUpdateStock.setInt( 2, ol_quantity ) ;
                stmtUpdateStock.setInt( 3, s_remote_cnt_increment ) ;
                stmtUpdateStock.setInt( 4, ol_i_id ) ;
                stmtUpdateStock.setInt( 5, ol_supply_w_id ) ;
                stmtUpdateStock.addBatch() ;

                ol_amount = ol_quantity * i_price ;
                orderLineAmounts[ol_number - 1] = ol_amount ;
                total_amount += ol_amount ;

                if ( i_data.indexOf( "GENERIC" ) != -1
                        && s_data.indexOf( "GENERIC" ) != -1 ) {
                    brandGeneric[ol_number - 1] = 'B' ;
                } else {
                    brandGeneric[ol_number - 1] = 'G' ;
                }

                switch ( ( int ) d_id ) {
                case 1:
                    ol_dist_info = s_dist_01 ;
                    break ;
                case 2:
                    ol_dist_info = s_dist_02 ;
                    break ;
                case 3:
                    ol_dist_info = s_dist_03 ;
                    break ;
                case 4:
                    ol_dist_info = s_dist_04 ;
                    break ;
                case 5:
                    ol_dist_info = s_dist_05 ;
                    break ;
                case 6:
                    ol_dist_info = s_dist_06 ;
                    break ;
                case 7:
                    ol_dist_info = s_dist_07 ;
                    break ;
                case 8:
                    ol_dist_info = s_dist_08 ;
                    break ;
                case 9:
                    ol_dist_info = s_dist_09 ;
                    break ;
                case 10:
                    ol_dist_info = s_dist_10 ;
                    break ;
                }
                if ( stmtInsertOrderLine == null ) {
                    stmtInsertOrderLine = conn
                            .prepareStatement( "INSERT INTO order_line (ol_o_id, ol_d_id, ol_w_id, ol_number, ol_i_id, ol_supply_w_id,"
                                    + "  ol_quantity, ol_amount, ol_dist_info) VALUES (?,?,?,?,?,?,?,?,?)" ) ;
                }
                stmtInsertOrderLine.setInt( 1, o_id ) ;
                stmtInsertOrderLine.setInt( 2, d_id ) ;
                stmtInsertOrderLine.setInt( 3, w_id ) ;
                stmtInsertOrderLine.setInt( 4, ol_number ) ;
                stmtInsertOrderLine.setInt( 5, ol_i_id ) ;
                stmtInsertOrderLine.setInt( 6, ol_supply_w_id ) ;
                stmtInsertOrderLine.setInt( 7, ol_quantity ) ;
                stmtInsertOrderLine.setFloat( 8, ol_amount ) ;
                stmtInsertOrderLine.setString( 9, ol_dist_info ) ;
                stmtInsertOrderLine.addBatch() ;

            } // end-for

            stmtInsertOrderLine.executeBatch() ;
            stmtUpdateStock.executeBatch() ;
            conn.commit() ;
            stmtInsertOrderLine.clearBatch() ;
            stmtUpdateStock.clearBatch() ;
            total_amount *= ( 1 + w_tax + d_tax ) * ( 1 - c_discount ) ;
        } catch ( SQLException ex ) {
            error( "\n--- Unexpected SQLException caught in NEW-ORDER Txn ---\n" ) ;
            while ( ex != null ) {
                error( "Message:   " + ex.getMessage() ) ;
                error( "SQLState:  " + ex.getSQLState() ) ;
                error( "ErrorCode: " + ex.getErrorCode() ) ;
                ex = ex.getNextException() ;
                error( "" ) ;
            }

        } catch ( Exception e ) {
            logException( e ) ;
            throw e ;
        } finally {
            try {
                conn.rollback() ;
            } catch ( Exception e1 ) {
                error( "NEW-ORDER-ROLLBACK" ) ;
                logException( e1 ) ;
            }
            if ( stmtInsertOrderLine != null ) {
                try {
                    stmtInsertOrderLine.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
            if ( stmtUpdateStock != null ) {
                try {
                    stmtUpdateStock.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
            if ( stmtGetCustWhse != null ) {
                try {
                    stmtGetCustWhse.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
            if ( stmtGetDist != null ) {
                try {
                    stmtGetDist.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
            if ( stmtInsertNewOrder != null ) {
                try {
                    stmtInsertNewOrder.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
            if ( stmtUpdateDist != null ) {
                try {
                    stmtUpdateDist.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
            if ( stmtInsertOOrder != null ) {
                try {
                    stmtInsertOOrder.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
            if ( stmtGetItem != null ) {
                try {
                    stmtGetItem.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
            if ( stmtGetStock != null ) {
                try {
                    stmtGetStock.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
        }
    }

    private void stockLevelTransaction( int w_id, int d_id, int threshold,
            Connection conn ) throws Exception {
        int o_id = 0 ;
        int i_id = 0 ;
        int stock_count = 0 ;
        TableDistrict dist = new TableDistrict() ;
        TableOrderLine orln = new TableOrderLine() ;
        TableStock stck = new TableStock() ;
        PreparedStatement stockGetDistOrderId = null ;
        PreparedStatement stockGetCountStock = null ;
        try {
            conn.setAutoCommit( false ) ;
            if ( stockGetDistOrderId == null ) {
                stockGetDistOrderId = conn
                        .prepareStatement( "SELECT d_next_o_id"
                                + " FROM district" + " WHERE d_w_id = ?"
                                + " AND d_id = ?" ) ;
            }
            stockGetDistOrderId.setInt( 1, w_id ) ;
            stockGetDistOrderId.setInt( 2, d_id ) ;
            ResultSet rs = stockGetDistOrderId.executeQuery() ;

            if ( !rs.next() )
                throw new Exception( "D_W_ID=" + w_id + " D_ID=" + d_id
                        + " not found!" ) ;
            o_id = rs.getInt( "d_next_o_id" ) ;
            rs.close() ;
            rs = null ;
            if ( stockGetCountStock == null ) {
                stockGetCountStock = conn
                        .prepareStatement( "SELECT COUNT(DISTINCT (s_i_id)) AS stock_count"
                                + " FROM order_line, stock"
                                + " WHERE ol_w_id = ?"
                                + " AND ol_d_id = ?"
                                + " AND ol_o_id < ?"
                                + " AND ol_o_id >= ? - 20"
                                + " AND s_w_id = ?"
                                + " AND s_i_id = ol_i_id"
                                + " AND s_quantity < ?" ) ;
            }
            stockGetCountStock.setInt( 1, w_id ) ;
            stockGetCountStock.setInt( 2, d_id ) ;
            stockGetCountStock.setInt( 3, o_id ) ;
            stockGetCountStock.setInt( 4, o_id ) ;
            stockGetCountStock.setInt( 5, w_id ) ;
            stockGetCountStock.setInt( 6, threshold ) ;
            rs = stockGetCountStock.executeQuery() ;

            if ( !rs.next() )
                throw new Exception( "OL_W_ID=" + w_id + " OL_D_ID=" + d_id
                        + " OL_O_ID=" + o_id + " (...) not found!" ) ;
            stock_count = rs.getInt( "stock_count" ) ;
            rs.close() ;
            rs = null ;
            conn.commit() ;
        } catch ( Exception e ) {
            error( "STOCK-LEVEL" ) ;
            logException( e ) ;
            throw e ;
        } finally {
            if ( stockGetDistOrderId != null ) {
                try {
                    stockGetDistOrderId.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
            if ( stockGetCountStock != null ) {
                try {
                    stockGetCountStock.close() ;
                } catch ( SQLException e ) {
                    e.printStackTrace() ;
                }
            }
        }
    }

    private void paymentTransaction( int w_id, int c_w_id, float h_amount,
            int d_id, int c_d_id, int c_id, String c_last, boolean c_by_name,
            Connection conn ) throws Exception {
        String w_street_1, w_street_2, w_city, w_state, w_zip, w_name ;
        String d_street_1, d_street_2, d_city, d_state, d_zip, d_name ;
        int namecnt ;
        String c_first, c_middle, c_street_1, c_street_2, c_city, c_state, c_zip ;
        String c_phone, c_credit = null, c_data = null, c_new_data, h_data ;
        float c_credit_lim, c_discount, c_balance = 0 ;
        java.sql.Date c_since ;

        TableWarehouse whse = new TableWarehouse() ;
        TableCustomer cust = new TableCustomer() ;
        TableDistrict dist = new TableDistrict() ;
        TableHistory hist = new TableHistory() ;
        PreparedStatement payUpdateWhse = null ;
        PreparedStatement payGetWhse = null ;
        PreparedStatement payUpdateDist = null ;
        PreparedStatement payGetDist = null ;
        PreparedStatement payCountCust = null ;
        PreparedStatement payCursorCustByName = null ;
        PreparedStatement payGetCust = null ;
        PreparedStatement payGetCustCdata = null ;
        PreparedStatement payUpdateCustBalCdata = null ;
        PreparedStatement payUpdateCustBal = null ;
        PreparedStatement payInsertHist = null ;
        try {
            if ( payUpdateWhse == null ) {
                payUpdateWhse = conn
                        .prepareStatement( "UPDATE warehouse SET w_ytd = w_ytd + ?  WHERE w_id = ? " ) ;
            }
            payUpdateWhse.setFloat( 1, h_amount ) ;
            payUpdateWhse.setInt( 2, w_id ) ;
            result = payUpdateWhse.executeUpdate() ;
            if ( result == 0 )
                throw new Exception( "W_ID=" + w_id + " not found!" ) ;
            if ( payGetWhse == null ) {
                payGetWhse = conn
                        .prepareStatement( "SELECT w_street_1, w_street_2, w_city, w_state, w_zip, w_name"
                                + " FROM warehouse WHERE w_id = ?" ) ;
            }
            payGetWhse.setInt( 1, w_id ) ;
            ResultSet rs = payGetWhse.executeQuery() ;
            if ( !rs.next() )
                throw new Exception( "W_ID=" + w_id + " not found!" ) ;
            w_street_1 = rs.getString( "w_street_1" ) ;
            w_street_2 = rs.getString( "w_street_2" ) ;
            w_city = rs.getString( "w_city" ) ;
            w_state = rs.getString( "w_state" ) ;
            w_zip = rs.getString( "w_zip" ) ;
            w_name = rs.getString( "w_name" ) ;
            rs.close() ;
            rs = null ;
            if ( payUpdateDist == null ) {
                payUpdateDist = conn
                        .prepareStatement( "UPDATE district SET d_ytd = d_ytd + ? WHERE d_w_id = ? AND d_id = ?" ) ;
            }
            payUpdateDist.setFloat( 1, h_amount ) ;
            payUpdateDist.setInt( 2, w_id ) ;
            payUpdateDist.setInt( 3, d_id ) ;
            result = payUpdateDist.executeUpdate() ;
            if ( result == 0 )
                throw new Exception( "D_ID=" + d_id + " D_W_ID=" + w_id
                        + " not found!" ) ;
            if ( payGetDist == null ) {
                payGetDist = conn
                        .prepareStatement( "SELECT d_street_1, d_street_2, d_city, d_state, d_zip, d_name"
                                + " FROM district WHERE d_w_id = ? AND d_id = ?" ) ;
            }
            payGetDist.setInt( 1, w_id ) ;
            payGetDist.setInt( 2, d_id ) ;
            rs = payGetDist.executeQuery() ;
            if ( !rs.next() )
                throw new Exception( "D_ID=" + d_id + " D_W_ID=" + w_id
                        + " not found!" ) ;
            d_street_1 = rs.getString( "d_street_1" ) ;
            d_street_2 = rs.getString( "d_street_2" ) ;
            d_city = rs.getString( "d_city" ) ;
            d_state = rs.getString( "d_state" ) ;
            d_zip = rs.getString( "d_zip" ) ;
            d_name = rs.getString( "d_name" ) ;
            rs.close() ;
            rs = null ;

            if ( c_by_name ) {
                // payment is by customer name
                if ( payCountCust == null ) {
                    payCountCust = conn
                            .prepareStatement( "SELECT count(c_id) AS namecnt FROM customer "
                                    + " WHERE c_last = ?  AND c_d_id = ? AND c_w_id = ?" ) ;
                }
                payCountCust.setString( 1, c_last ) ;
                payCountCust.setInt( 2, c_d_id ) ;
                payCountCust.setInt( 3, c_w_id ) ;
                rs = payCountCust.executeQuery() ;
                if ( !rs.next() )
                    throw new Exception( "C_LAST=" + c_last + " C_D_ID="
                            + c_d_id + " C_W_ID=" + c_w_id + " not found!" ) ;
                namecnt = rs.getInt( "namecnt" ) ;
                rs.close() ;
                rs = null ;
                if ( payCursorCustByName == null ) {
                    payCursorCustByName = conn
                            .prepareStatement( "SELECT c_first, c_middle, c_id, c_street_1, c_street_2, c_city, c_state, c_zip,"
                                    + "       c_phone, c_credit, c_credit_lim, c_discount, c_balance, c_since "
                                    + "  FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_last = ? "
                                    + "ORDER BY c_w_id, c_d_id, c_last, c_first " ) ;
                }
                payCursorCustByName.setInt( 1, c_w_id ) ;
                payCursorCustByName.setInt( 2, c_d_id ) ;
                payCursorCustByName.setString( 3, c_last ) ;
                rs = payCursorCustByName.executeQuery() ;
                if ( !rs.next() )
                    throw new Exception( "C_LAST=" + c_last + " C_D_ID="
                            + c_d_id + " C_W_ID=" + c_w_id + " not found!" ) ;
                if ( namecnt % 2 == 1 )
                    namecnt++ ;
                for ( int i = 1; i < namecnt / 2; i++ )
                    rs.next() ;
                c_id = rs.getInt( "c_id" ) ;
                c_first = rs.getString( "c_first" ) ;
                c_middle = rs.getString( "c_middle" ) ;
                c_street_1 = rs.getString( "c_street_1" ) ;
                c_street_2 = rs.getString( "c_street_2" ) ;
                c_city = rs.getString( "c_city" ) ;
                c_state = rs.getString( "c_state" ) ;
                c_zip = rs.getString( "c_zip" ) ;
                c_phone = rs.getString( "c_phone" ) ;
                c_credit = rs.getString( "c_credit" ) ;
                c_credit_lim = rs.getFloat( "c_credit_lim" ) ;
                c_discount = rs.getFloat( "c_discount" ) ;
                c_balance = rs.getFloat( "c_balance" ) ;
                c_since = rs.getDate( "c_since" ) ;
                rs.close() ;
                rs = null ;
            } else {
                // payment is by customer ID
                if ( payGetCust == null ) {
                    payGetCust = conn
                            .prepareStatement( "SELECT c_first, c_middle, c_last, c_street_1, c_street_2, c_city, c_state, c_zip,"
                                    + "       c_phone, c_credit, c_credit_lim, c_discount, c_balance, c_since "
                                    + "  FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_id = ?" ) ;
                }
                payGetCust.setInt( 1, c_w_id ) ;
                payGetCust.setInt( 2, c_d_id ) ;
                payGetCust.setInt( 3, c_id ) ;
                rs = payGetCust.executeQuery() ;
                if ( !rs.next() )
                    throw new Exception( "C_ID=" + c_id + " C_D_ID=" + c_d_id
                            + " C_W_ID=" + c_w_id + " not found!" ) ;
                c_last = rs.getString( "c_last" ) ;
                c_first = rs.getString( "c_first" ) ;
                c_middle = rs.getString( "c_middle" ) ;
                c_street_1 = rs.getString( "c_street_1" ) ;
                c_street_2 = rs.getString( "c_street_2" ) ;
                c_city = rs.getString( "c_city" ) ;
                c_state = rs.getString( "c_state" ) ;
                c_zip = rs.getString( "c_zip" ) ;
                c_phone = rs.getString( "c_phone" ) ;
                c_credit = rs.getString( "c_credit" ) ;
                c_credit_lim = rs.getFloat( "c_credit_lim" ) ;
                c_discount = rs.getFloat( "c_discount" ) ;
                c_balance = rs.getFloat( "c_balance" ) ;
                c_since = rs.getDate( "c_since" ) ;
                rs.close() ;
                rs = null ;
            }

            c_balance += h_amount ;
            if ( c_credit.equals( "BC" ) ) { // bad credit
                if ( payGetCustCdata == null ) {
                    payGetCustCdata = conn
                            .prepareStatement( "SELECT c_data FROM customer WHERE c_w_id = ? AND c_d_id = ? AND c_id = ?" ) ;
                }
                payGetCustCdata.setInt( 1, c_w_id ) ;
                payGetCustCdata.setInt( 2, c_d_id ) ;
                payGetCustCdata.setInt( 3, c_id ) ;
                rs = payGetCustCdata.executeQuery() ;
                if ( !rs.next() )
                    throw new Exception( "C_ID=" + c_id + " C_W_ID=" + c_w_id
                            + " C_D_ID=" + c_d_id + " not found!" ) ;
                c_data = rs.getString( "c_data" ) ;
                rs.close() ;
                rs = null ;

                c_new_data = c_id + " " + c_d_id + " " + c_w_id + " " + d_id
                        + " " + w_id + " " + h_amount + " |" ;
                if ( c_data.length() > c_new_data.length() ) {
                    c_new_data += c_data.substring( 0, c_data.length()
                            - c_new_data.length() ) ;
                } else {
                    c_new_data += c_data ;
                }
                if ( c_new_data.length() > 500 )
                    c_new_data = c_new_data.substring( 0, 500 ) ;
                if ( payUpdateCustBalCdata == null ) {
                    payUpdateCustBalCdata = conn
                            .prepareStatement( "UPDATE customer SET c_balance = ?, c_data = ? "
                                    + " WHERE c_w_id = ? AND c_d_id = ? AND c_id = ?" ) ;
                }
                payUpdateCustBalCdata.setFloat( 1, c_balance ) ;
                payUpdateCustBalCdata.setString( 2, c_new_data ) ;
                payUpdateCustBalCdata.setInt( 3, c_w_id ) ;
                payUpdateCustBalCdata.setInt( 4, c_d_id ) ;
                payUpdateCustBalCdata.setInt( 5, c_id ) ;
                result = payUpdateCustBalCdata.executeUpdate() ;
                if ( result == 0 )
                    throw new Exception(
                            "Error in PYMNT Txn updating Customer C_ID=" + c_id
                                    + " C_W_ID=" + c_w_id + " C_D_ID=" + c_d_id ) ;

            } else { // GoodCredit
                if ( payUpdateCustBal == null ) {
                    payUpdateCustBal = conn
                            .prepareStatement( "UPDATE customer SET c_balance = ? WHERE c_w_id = ? AND c_d_id = ? AND c_id = ?" ) ;
                }
                payUpdateCustBal.setFloat( 1, c_balance ) ;
                payUpdateCustBal.setInt( 2, c_w_id ) ;
                payUpdateCustBal.setInt( 3, c_d_id ) ;
                payUpdateCustBal.setInt( 4, c_id ) ;
                result = payUpdateCustBal.executeUpdate() ;
                if ( result == 0 )
                    throw new Exception( "C_ID=" + c_id + " C_W_ID=" + c_w_id
                            + " C_D_ID=" + c_d_id + " not found!" ) ;

            }

            if ( w_name.length() > 10 )
                w_name = w_name.substring( 0, 10 ) ;
            if ( d_name.length() > 10 )
                d_name = d_name.substring( 0, 10 ) ;
            h_data = w_name + "    " + d_name ;
            if ( payInsertHist == null ) {
                payInsertHist = conn
                        .prepareStatement( "INSERT INTO history (h_c_d_id, h_c_w_id, h_c_id, h_d_id, h_w_id, h_date, h_amount, h_data) "
                                + " VALUES (?,?,?,?,?,?,?,?)" ) ;
            }
            payInsertHist.setInt( 1, c_d_id ) ;
            payInsertHist.setInt( 2, c_w_id ) ;
            payInsertHist.setInt( 3, c_id ) ;
            payInsertHist.setInt( 4, d_id ) ;
            payInsertHist.setInt( 5, w_id ) ;
            payInsertHist.setTimestamp( 6,
                    new Timestamp( System.currentTimeMillis() ) ) ;
            payInsertHist.setFloat( 7, h_amount ) ;
            payInsertHist.setString( 8, h_data ) ;
            payInsertHist.executeUpdate() ;
            conn.commit() ;
        } catch ( Exception e ) {
            error( "PAYMENT" ) ;
            logException( e ) ;
            try {
                conn.rollback() ;
            } catch ( Exception e1 ) {
                error( "PAYMENT-ROLLBACK" ) ;
                logException( e1 ) ;
            }
            throw e ;
        } finally {
            try {
                if ( payUpdateWhse != null ) {
                    payUpdateWhse.close() ;
                }
                if ( payGetWhse != null ) {
                    payGetWhse.close() ;
                }
                if ( payUpdateDist != null ) {
                    payUpdateDist.close() ;
                }
                if ( payGetDist != null ) {
                    payGetDist.close() ;
                }
                if ( payCountCust != null ) {
                    payCountCust.close() ;
                }
                if ( payCursorCustByName != null ) {
                    payCursorCustByName.close() ;
                }
                if ( payGetCust != null ) {
                    payGetCust.close() ;
                }
                if ( payGetCustCdata != null ) {
                    payGetCustCdata.close() ;
                }
                if ( payUpdateCustBalCdata != null ) {
                    payUpdateCustBalCdata.close() ;
                }
                if ( payUpdateCustBal != null ) {
                    payUpdateCustBal.close() ;
                }
                if ( payInsertHist != null ) {
                    payInsertHist.close() ;
                }
            } catch ( SQLException e ) {
                e.printStackTrace() ;
            }
        }
    }

    private void error( String type ) {
        synchronized ( printStreamErrors ) {
            this.printStreamErrors.println( "[ERROR] USER=" + userName
                    + "  TYPE=" + type + "  COUNT=" + transactionCount ) ;
        }
    }

    private void logException( Exception e ) {
        // StringWriter stringWriter = new StringWriter();
        // PrintWriter printWriter = new PrintWriter(stringWriter);
        e.printStackTrace( this.printStreamErrors ) ;
        // printWriter.close();
    }
}
