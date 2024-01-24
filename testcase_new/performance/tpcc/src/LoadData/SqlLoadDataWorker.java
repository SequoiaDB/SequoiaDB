/*
 * LoadDataWorker - Class to load one Warehouse (or in a special case
 * the ITEM table).
 *
 * Copyright (C) 2016, Denis Lussier
 * Copyright (C) 2016, Jan Wieck
 *
 */

import java.sql.* ;
import java.util.* ;
import java.io.* ;

import com.sequoiadb.exception.BaseException ;

public class SqlLoadDataWorker extends LoadDataWorker {
    private Connection dbConn ;

    private PreparedStatement stmtConfig = null ;
    private PreparedStatement stmtItem = null ;
    private PreparedStatement stmtWarehouse = null ;
    private PreparedStatement stmtDistrict = null ;
    private PreparedStatement stmtStock = null ;
    private PreparedStatement stmtCustomer = null ;
    private PreparedStatement stmtHistory = null ;
    private PreparedStatement stmtOrder = null ;
    private PreparedStatement stmtOrderLine = null ;
    private PreparedStatement stmtNewOrder = null ;

    public SqlLoadDataWorker( int worker, Connection dbConn, jTPCCRandom rnd )
            throws SQLException {
        super( worker, rnd ) ;
        this.dbConn = dbConn ;

        stmtConfig = dbConn.prepareStatement( "INSERT INTO bmsql_config ("
                + "  cfg_name, cfg_value) " + "VALUES (?, ?)" ) ;
        stmtItem = dbConn.prepareStatement( "INSERT INTO bmsql_item ("
                + "  i_id, i_im_id, i_name, i_price, i_data) "
                + "VALUES (?, ?, ?, ?, ?)" ) ;
        stmtWarehouse = dbConn
                .prepareStatement( "INSERT INTO bmsql_warehouse ("
                        + "  w_id, w_name, w_street_1, w_street_2, w_city, "
                        + "  w_state, w_zip, w_tax, w_ytd) "
                        + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;
        stmtStock = dbConn.prepareStatement( "INSERT INTO bmsql_stock ("
                + "  s_i_id, s_w_id, s_quantity, s_dist_01, s_dist_02, "
                + "  s_dist_03, s_dist_04, s_dist_05, s_dist_06, "
                + "  s_dist_07, s_dist_08, s_dist_09, s_dist_10, "
                + "  s_ytd, s_order_cnt, s_remote_cnt, s_data) "
                + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;
        stmtDistrict = dbConn.prepareStatement( "INSERT INTO bmsql_district ("
                + "  d_id, d_w_id, d_name, d_street_1, d_street_2, "
                + "  d_city, d_state, d_zip, d_tax, d_ytd, d_next_o_id) "
                + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;
        stmtCustomer = dbConn.prepareStatement( "INSERT INTO bmsql_customer ("
                + "  c_id, c_d_id, c_w_id, c_first, c_middle, c_last, "
                + "  c_street_1, c_street_2, c_city, c_state, c_zip, "
                + "  c_phone, c_since, c_credit, c_credit_lim, c_discount, "
                + "  c_balance, c_ytd_payment, c_payment_cnt, "
                + "  c_delivery_cnt, c_data) "
                + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                + "        ?, ?, ?, ?, ?, ?)" ) ;
        stmtHistory = dbConn.prepareStatement( "INSERT INTO bmsql_history ("
                + "  hist_id, h_c_id, h_c_d_id, h_c_w_id, h_d_id, h_w_id, "
                + "  h_date, h_amount, h_data) "
                + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;
        stmtOrder = dbConn.prepareStatement( "INSERT INTO bmsql_oorder ("
                + "  o_id, o_d_id, o_w_id, o_c_id, o_entry_d, "
                + "  o_carrier_id, o_ol_cnt, o_all_local) "
                + "VALUES (?, ?, ?, ?, ?, ?, ?, ?)" ) ;
        stmtOrderLine = dbConn
                .prepareStatement( "INSERT INTO bmsql_order_line ("
                        + "  ol_o_id, ol_d_id, ol_w_id, ol_number, ol_i_id, "
                        + "  ol_supply_w_id, ol_delivery_d, ol_quantity, "
                        + "  ol_amount, ol_dist_info) "
                        + "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)" ) ;
        stmtNewOrder = dbConn.prepareStatement( "INSERT INTO bmsql_new_order ("
                + "  no_o_id, no_d_id, no_w_id) " + "VALUES (?, ?, ?)" ) ;
    }

    protected void loadConfig() throws SQLException, BaseException, IOException {
        Map< String, String > configs = getConfig() ;

        /*
         * Saving CONFIG information in DB mode.
         */
        Iterator< Map.Entry< String, String >> it = configs.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, String > entry = it.next() ;
            stmtConfig.setString( 1, entry.getKey() ) ;
            stmtConfig.setString( 2, entry.getValue() ) ;
            stmtConfig.execute() ;
        }
    }

    protected void buildItemRecord( int i_id ) throws SQLException,
            BaseException, IOException {
        Map< String, Object > item = getItem( i_id ) ;
        stmtItem.setInt( 1, ( int ) item.get( "i_id" ) ) ;

        stmtItem.setInt( 2, ( int ) item.get( "i_im_id" ) ) ;
        stmtItem.setString( 3, ( String ) item.get( "i_name" ) ) ;
        stmtItem.setDouble( 4, ( double ) item.get( "i_price" ) ) ;
        stmtItem.setString( 5, ( String ) item.get( "i_data" ) ) ;
        stmtItem.addBatch() ;
    }

    protected void loadWarehouseRecord( int w_id ) throws SQLException,
            BaseException, IOException {
        /*
         * Load the WAREHOUSE row.
         */
        Map< String, Object > record = getWarehouse( w_id ) ;
        stmtWarehouse.setInt( 1, ( int ) record.get( "w_id" ) ) ;
        stmtWarehouse.setString( 2, ( String ) record.get( "w_name" ) ) ;
        stmtWarehouse.setString( 3, ( String ) record.get( "w_street_1" ) ) ;
        stmtWarehouse.setString( 4, ( String ) record.get( "w_street_2" ) ) ;
        stmtWarehouse.setString( 5, ( String ) record.get( "w_city" ) ) ;
        stmtWarehouse.setString( 6, ( String ) record.get( "w_state" ) ) ;
        stmtWarehouse.setString( 7, ( String ) record.get( "w_zip" ) ) ;
        stmtWarehouse.setDouble( 8, ( double ) record.get( "w_tax" ) ) ;
        stmtWarehouse.setDouble( 9, ( double ) record.get( "w_ytd" ) ) ;

        stmtWarehouse.execute() ;
    }

    protected void buildStockRecord( int s_i_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getStock( s_i_id, w_id ) ;
        stmtStock.setInt( 1, s_i_id ) ;
        stmtStock.setInt( 2, w_id ) ;
        stmtStock.setInt( 3, ( int ) record.get( "s_quantity" ) ) ;
        stmtStock.setString( 4, ( String ) record.get( "s_dist_01" ) ) ;
        stmtStock.setString( 5, ( String ) record.get( "s_dist_02" ) ) ;
        stmtStock.setString( 6, ( String ) record.get( "s_dist_03" ) ) ;
        stmtStock.setString( 7, ( String ) record.get( "s_dist_04" ) ) ;
        stmtStock.setString( 8, ( String ) record.get( "s_dist_05" ) ) ;
        stmtStock.setString( 9, ( String ) record.get( "s_dist_06" ) ) ;
        stmtStock.setString( 10, ( String ) record.get( "s_dist_07" ) ) ;
        stmtStock.setString( 11, ( String ) record.get( "s_dist_08" ) ) ;
        stmtStock.setString( 12, ( String ) record.get( "s_dist_09" ) ) ;
        stmtStock.setString( 13, ( String ) record.get( "s_dist_10" ) ) ;
        stmtStock.setInt( 14, ( int ) record.get( "s_ytd" ) ) ;
        stmtStock.setInt( 15, ( int ) record.get( "s_order_cnt" ) ) ;
        stmtStock.setInt( 16, ( int ) record.get( "s_remote_cnt" ) ) ;
        stmtStock.setString( 17, ( String ) record.get( "s_data" ) ) ;

        stmtStock.addBatch() ;
    }

    protected void loadDistrictRecord( int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getDistrict( d_id, w_id ) ;
        stmtDistrict.setInt( 1, d_id ) ;
        stmtDistrict.setInt( 2, w_id ) ;
        stmtDistrict.setString( 3, ( String ) record.get( "d_name" ) ) ;
        stmtDistrict.setString( 4, ( String ) record.get( "d_street_1" ) ) ;
        stmtDistrict.setString( 5, ( String ) record.get( "d_street_2" ) ) ;
        stmtDistrict.setString( 6, ( String ) record.get( "d_city" ) ) ;
        stmtDistrict.setString( 7, ( String ) record.get( "d_state" ) ) ;
        stmtDistrict.setString( 8, ( String ) record.get( "d_zip" ) ) ;
        stmtDistrict.setDouble( 9, ( double ) record.get( "d_tax" ) ) ;
        stmtDistrict.setDouble( 10, ( double ) record.get( "d_ytd" ) ) ;
        stmtDistrict.setInt( 11, ( int ) record.get( "d_next_o_id" ) ) ;

        stmtDistrict.execute() ;
    }

    protected void buildCustomerRecord( int c_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getCustomer( c_id, d_id, w_id ) ;
        stmtCustomer.setInt( 1, ( int ) record.get( "c_id" ) ) ;
        stmtCustomer.setInt( 2, ( int ) record.get( "c_d_id" ) ) ;
        stmtCustomer.setInt( 3, ( int ) record.get( "c_w_id" ) ) ;
        stmtCustomer.setString( 4, ( String ) record.get( "c_first" ) ) ;
        stmtCustomer.setString( 5, ( String ) record.get( "c_middle" ) ) ;
        stmtCustomer.setString( 6, ( String ) record.get( "c_last" ) ) ;
        stmtCustomer.setString( 7, ( String ) record.get( "c_street_1" ) ) ;
        stmtCustomer.setString( 8, ( String ) record.get( "c_street_2" ) ) ;
        stmtCustomer.setString( 9, ( String ) record.get( "c_city" ) ) ;
        stmtCustomer.setString( 10, ( String ) record.get( "c_state" ) ) ;
        stmtCustomer.setString( 11, ( String ) record.get( "c_zip" ) ) ;
        stmtCustomer.setString( 12, ( String ) record.get( "c_phone" ) ) ;
        stmtCustomer.setTimestamp( 13,
                new java.sql.Timestamp((long)record.get( "c_since" )) ) ;
        stmtCustomer.setString( 14, ( String ) record.get( "c_credit" ) ) ;
        stmtCustomer.setDouble( 15, ( double ) record.get( "c_credit_lim" ) ) ;
        stmtCustomer.setDouble( 16, ( double ) record.get( "c_discount" ) ) ;
        stmtCustomer.setDouble( 17, ( double ) record.get( "c_balance" ) ) ;
        stmtCustomer.setDouble( 18, ( double ) record.get( "c_ytd_payment" ) ) ;
        stmtCustomer.setInt( 19, ( int ) record.get( "c_payment_cnt" ) ) ;
        stmtCustomer.setInt( 20, ( int ) record.get( "c_delivery_cnt" ) ) ;
        stmtCustomer.setString( 21, ( String ) record.get( "c_data" ) ) ;

        stmtCustomer.addBatch() ;
    }

    protected void buildHistoryRecord( int c_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > records = getHistory( w_id, c_id, d_id ) ;
        stmtHistory.setInt( 1, ( int ) records.get( "hist_id" ) ) ;
        stmtHistory.setInt( 2, ( int ) records.get( "h_c_id" ) ) ;
        stmtHistory.setInt( 3, ( int ) records.get( "h_c_d_id" ) ) ;
        stmtHistory.setInt( 4, ( int ) records.get( "h_c_w_id" ) ) ;
        stmtHistory.setInt( 5, ( int ) records.get( "h_d_id" ) ) ;
        stmtHistory.setInt( 6, ( int ) records.get( "h_w_id" ) ) ;
        stmtHistory.setTimestamp( 7,
                new java.sql.Timestamp((long) records.get( "h_date" ) ) ) ;
        stmtHistory.setDouble( 8, ( double ) records.get( "h_amount" ) ) ;
        stmtHistory.setString( 9, ( String ) records.get( "h_data" ) ) ;

        stmtHistory.addBatch() ;
    }

    protected void buildOrderRecord( int o_id, int w_id, int d_id, int o_ol_cnt )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getOrder( o_id, w_id, d_id, o_ol_cnt ) ;
        stmtOrder.setInt( 1, ( int ) record.get( "o_id" ) ) ;
        stmtOrder.setInt( 2, ( int ) record.get( "o_d_id" ) ) ;
        stmtOrder.setInt( 3, ( int ) record.get( "o_w_id" ) ) ;
        stmtOrder.setInt( 4, ( int ) record.get( "o_c_id" ) ) ;
        stmtOrder.setTimestamp( 5,
                new java.sql.Timestamp( (long)record.get( "o_entry_d" ) ) ) ;
        if ( (int) record.get( "o_carrier_id" ) == java.sql.Types.INTEGER ){
           stmtOrder.setNull( 6, java.sql.Types.INTEGER );
        }else{
           stmtOrder.setInt( 6, ( int ) record.get( "o_carrier_id" ) ) ;
        }
        stmtOrder.setInt( 7, ( int ) record.get( "o_ol_cnt" ) ) ;
        stmtOrder.setInt( 8, ( int ) record.get( "o_all_local" ) ) ;

        stmtOrder.addBatch() ;
    }

    protected void buildOrderLineRecord( int w_id, int d_id, int o_id,
            int ol_number ) throws SQLException, BaseException, IOException {
        Map< String, Object > record = getOrderLine( o_id, d_id, w_id,
                ol_number ) ;
        stmtOrderLine.setInt( 1, ( int ) record.get( "ol_o_id" ) ) ;
        stmtOrderLine.setInt( 2, ( int ) record.get( "ol_d_id" ) ) ;
        stmtOrderLine.setInt( 3, ( int ) record.get( "ol_w_id" ) ) ;
        stmtOrderLine.setInt( 4, ( int ) record.get( "ol_number" ) ) ;
        stmtOrderLine.setInt( 5, ( int ) record.get( "ol_i_id" ) ) ;
        stmtOrderLine.setInt( 6, ( int ) record.get( "ol_supply_w_id" ) ) ;
        if ( record.get( "ol_delivery_d" ) == java.sql.Types.TIMESTAMP ){
           stmtOrderLine.setNull( 7, java.sql.Types.TIMESTAMP ) ;
        }else{
           stmtOrderLine.setTimestamp(7, new java.sql.Timestamp((long)record.get( "ol_delivery_d" )) );
        }
        stmtOrderLine.setInt( 8, ( int ) record.get( "ol_quantity" ) ) ;
        stmtOrderLine.setDouble( 9, ( double ) record.get( "ol_amount" ) ) ;
        stmtOrderLine.setString( 10, ( String ) record.get( "ol_dist_info" ) ) ;

        stmtOrderLine.addBatch() ;
    }

    protected void buildNewOrder( int o_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getNewOrder( o_id, d_id, w_id ) ;
        stmtNewOrder.setInt( 1, ( int ) record.get( "no_o_id" ) ) ;
        stmtNewOrder.setInt( 2, ( int ) record.get( "no_d_id" ) ) ;
        stmtNewOrder.setInt( 3, ( int ) record.get( "no_w_id" ) ) ;

        stmtNewOrder.addBatch() ;
    }

    protected void batchCommit( String tableName ) throws SQLException,
            BaseException, IOException {
        if ( tableName.equals( "item" ) ) {
            stmtItem.executeBatch() ;
            stmtItem.clearBatch() ;
        } else if ( tableName.equals( "stock" ) ) {
            stmtStock.executeBatch() ;
            stmtStock.clearBatch() ;
        } else if ( tableName.equals( "customer" ) ) {
            stmtCustomer.executeBatch() ;
            stmtCustomer.clearBatch() ;
        } else if ( tableName.equals( "history" ) ) {
            stmtHistory.executeBatch() ;
            stmtHistory.clearBatch() ;
        } else if ( tableName.equals( "order" ) ) {
            stmtOrder.executeBatch() ;
            stmtOrder.clearBatch() ;
        } else if ( tableName.equals( "orderline" ) ) {
            stmtOrderLine.executeBatch() ;
            stmtOrderLine.clearBatch() ;
        } else if ( tableName.equals( "neworder" ) ) {
            stmtNewOrder.executeBatch() ;
            stmtNewOrder.clearBatch() ;
        }
    }

    protected void Commit( String tableName ) throws SQLException,
            BaseException, IOException {
        batchCommit( tableName ) ;
        if ( tableName.equals( "item" ) ) {
            stmtItem.close() ;
            dbConn.commit() ;
        } else {
            dbConn.commit() ;
        }
    }

    protected void Done() throws SQLException, BaseException, IOException {
        /*
         * Close the DB connection if in direct DB mode.
         */
        dbConn.close() ;
    }
}
