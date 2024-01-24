/*
 * LoadDataWorker - Class to load one Warehouse (or in a special case
 * the ITEM table).
 *
 * Copyright (C) 2016, Denis Lussier
 * Copyright (C) 2016, Jan Wieck
 *
 */
import java.sql.SQLException ;
import java.util.* ;
import java.io.* ;

import com.sequoiadb.exception.BaseException ;

public class CsvLoadDataWorker extends LoadDataWorker {
    private String csvNull = null ;

    private StringBuffer sbConfig = null ;
    private Formatter fmtConfig = null ;
    private StringBuffer sbItem = null ;
    private Formatter fmtItem = null ;
    private StringBuffer sbWarehouse = null ;
    private Formatter fmtWarehouse = null ;
    private StringBuffer sbDistrict = null ;
    private Formatter fmtDistrict = null ;
    private StringBuffer sbStock = null ;
    private Formatter fmtStock = null ;
    private StringBuffer sbCustomer = null ;
    private Formatter fmtCustomer = null ;
    private StringBuffer sbHistory = null ;
    private Formatter fmtHistory = null ;
    private StringBuffer sbOrder = null ;
    private Formatter fmtOrder = null ;
    private StringBuffer sbOrderLine = null ;
    private Formatter fmtOrderLine = null ;
    private StringBuffer sbNewOrder = null ;
    private Formatter fmtNewOrder = null ;

    public CsvLoadDataWorker( int worker, String csvNull, jTPCCRandom rnd ) {
        super( worker, rnd ) ;
        this.csvNull = csvNull ;
        // TODO Auto-generated constructor stub
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
            fmtConfig.format( "%s,%d\n", entry.getKey(),
                    Integer.parseInt( entry.getValue() ) ) ;
        }
        LoadData.configAppend( sbConfig ) ;
    }

    protected void buildItemRecord( int i_id ) throws SQLException,
            BaseException, IOException {
        Map< String, Object > item = getItem( i_id ) ;
        fmtItem.format( "%d,%s,%.2f,%s,%d\n", ( int ) item.get( "i_id" ),
                ( String ) item.get( "i_name" ),
                ( double ) item.get( "i_price" ),
                ( String ) item.get( "i_data" ), ( int ) item.get( "i_im_id" ) ) ;
    }

    protected void loadWarehouseRecord( int w_id ) throws SQLException,
            BaseException, IOException {
        /*
         * Load the WAREHOUSE row.
         */
        Map< String, Object > record = getWarehouse( w_id ) ;
        fmtWarehouse.format( "%d,%.2f,%.4f,%s,%s,%s,%s,%s,%s\n", w_id,
                ( double ) record.get( "w_ytd" ),
                ( double ) record.get( "w_tax" ),
                ( String ) record.get( "w_name" ),
                ( String ) record.get( "w_street_1" ),
                ( String ) record.get( "w_street_2" ),
                ( String ) record.get( "w_city" ),
                ( String ) record.get( "w_state" ),
                ( String ) record.get( "w_zip" ) ) ;
        LoadData.warehouseAppend( sbWarehouse ) ;
    }

    protected void buildStockRecord( int s_i_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getStock( s_i_id, w_id ) ;
        fmtStock.format( "%d,%d,%d,%d,%d,%d,%s,"
                + "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", s_i_id, w_id,
                ( int ) record.get( "s_quantity" ),
                ( int ) record.get( "s_ytd" ),
                ( int ) record.get( "s_order_cnt" ),
                ( int ) record.get( "s_remote_cnt" ),
                ( String ) record.get( "s_data" ),
                ( String ) record.get( "s_dist_01" ),
                ( String ) record.get( "s_dist_02" ),
                ( String ) record.get( "s_dist_03" ),
                ( String ) record.get( "s_dist_04" ),
                ( String ) record.get( "s_dist_05" ),
                ( String ) record.get( "s_dist_06" ),
                ( String ) record.get( "s_dist_07" ),
                ( String ) record.get( "s_dist_08" ),
                ( String ) record.get( "s_dist_09" ),
                ( String ) record.get( "s_dist_10" ) ) ;
    }

    protected void loadDistrictRecord( int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getDistrict( d_id, w_id ) ;
        fmtDistrict.format( "%d,%d,%.2f,%.4f,%d,%s,%s,%s,%s,%s,%s\n", d_id,
                w_id, ( double ) record.get( "d_ytd" ),
                ( double ) record.get( "d_tax" ),
                ( int ) record.get( "d_next_o_id" ),
                ( String ) record.get( "d_name" ),
                ( String ) record.get( "d_street_1" ),
                ( String ) record.get( "d_street_2" ),
                ( String ) record.get( "d_city" ),
                ( String ) record.get( "d_state" ),
                ( String ) record.get( "d_zip" ) ) ;
        LoadData.districtAppend( sbDistrict ) ;
    }

    protected void buildCustomerRecord( int c_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getCustomer( c_id, d_id, w_id ) ;
        fmtCustomer.format( "%d,%d,%d,%.4f,%s,%s,%s," + "%.2f,%.2f,%.2f,%d,%d,"
                + "%s,%s,%s,%s,%s,%s,%s,%s,%s\n", ( int ) record.get( "c_id" ),
                ( int ) record.get( "c_d_id" ), ( int ) record.get( "c_w_id" ),
                ( double ) record.get( "c_discount" ),
                ( String ) record.get( "c_credit" ),
                ( String ) record.get( "c_last" ),
                ( String ) record.get( "c_first" ),
                ( double ) record.get( "c_credit_lim" ),
                ( double ) record.get( "c_balance" ),
                ( double ) record.get( "c_ytd_payment" ),
                ( int ) record.get( "c_payment_cnt" ),
                ( int ) record.get( "c_delivery_cnt" ),
                ( String ) record.get( "c_street_1" ),
                ( String ) record.get( "c_street_2" ),
                ( String ) record.get( "c_city" ),
                ( String ) record.get( "c_state" ),
                ( String ) record.get( "c_zip" ),
                ( String ) record.get( "c_phone" ),
                ( new java.sql.Timestamp ((long) record.get( "c_since" ) )).toString(),
                ( String ) record.get( "c_middle" ),
                ( String ) record.get( "c_data" ) ) ;
    }

    protected void buildHistoryRecord( int c_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > records = getHistory( w_id, c_id, d_id ) ;

        fmtHistory.format( "%d,%d,%d,%d,%d,%d,%s,%.2f,%s\n",
                ( int ) records.get( "hist_id" ),
                ( int ) records.get( "h_c_id" ),
                ( int ) records.get( "h_c_d_id" ),
                ( int ) records.get( "h_c_w_id" ),
                ( int ) records.get( "h_d_id" ),
                ( int ) records.get( "h_w_id" ),
                ( new java.sql.Timestamp ((long) records.get( "h_date" ) )).toString(),
                ( double ) records.get( "h_amount" ),
                ( String ) records.get( "h_data" ) ) ;
    }

    protected void buildOrderRecord( int o_id, int w_id, int d_id, int o_ol_cnt )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getOrder( o_id, w_id, d_id, o_ol_cnt ) ;

        fmtOrder.format( "%d,%d,%d,%d,%s,%d,%d,%s\n", ( int ) record
                .get( "o_id" ), ( int ) record.get( "o_w_id" ), ( int ) record
                .get( "o_d_id" ), ( int ) record.get( "o_c_id" ),
                ( o_id < 2101 ) ? ( int ) record.get( "o_carrier_id" )
                        : csvNull, ( int ) record.get( "o_ol_cnt" ),
                ( int ) record.get( "o_all_local" ),
                ( new java.sql.Timestamp((long) record.get( "o_entry_d" ) )).toString() ) ;
    }

    protected void buildOrderLineRecord( int w_id, int d_id, int o_id,
            int ol_number ) throws SQLException, BaseException, IOException {
        Map< String, Object > record = getOrderLine( o_id, d_id, w_id,
                ol_number ) ;
        fmtOrderLine.format(
                "%d,%d,%d,%d,%d,%s,%.2f,%d,%d,%s\n",
                ( int ) record.get( "ol_w_id" ),
                ( int ) record.get( "ol_d_id" ),
                ( int ) record.get( "ol_o_id" ),
                ( int ) record.get( "ol_number" ),
                ( int ) record.get( "ol_i_id" ),
                ( o_id < 2101 ) ? ( new java.sql.Timestamp ((long) record
                        .get( "ol_delivery_d" ) ) ).toString() : csvNull,
                ( double ) record.get( "ol_amount" ), ( int ) record
                        .get( "ol_supply_w_id" ), ( int ) record
                        .get( "ol_quantity" ), ( String ) record
                        .get( "ol_dist_info" ) ) ;
    }

    protected void buildNewOrder( int o_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getNewOrder( o_id, d_id, w_id ) ;
        fmtNewOrder
                .format( "%d,%d,%d\n", ( int ) record.get( "no_w_id" ),
                        ( int ) record.get( "no_d_id" ),
                        ( int ) record.get( "no_o_id" ) ) ;
    }

    protected void batchCommit( String tableName ) throws SQLException,
            BaseException, IOException {
        if ( tableName.equals( "item" ) ) {
            LoadData.itemAppend( sbItem ) ;
        } else if ( tableName.equals( "stock" ) ) {
            LoadData.stockAppend( sbStock ) ;
        } else if ( tableName.equals( "customer" ) ) {
            LoadData.customerAppend( sbCustomer ) ;
        } else if ( tableName.equals( "history" ) ) {
            LoadData.historyAppend( sbHistory ) ;
        } else if ( tableName.equals( "order" ) ) {
            LoadData.orderAppend( sbOrder ) ;
        } else if ( tableName.equals( "orderline" ) ) {
            LoadData.orderLineAppend( sbOrderLine ) ;
        } else if ( tableName.equals( "neworder" ) ) {
            LoadData.newOrderAppend( sbNewOrder ) ;
        }
    }

    protected void Commit( String tableName ) throws SQLException,
            BaseException, IOException {
        batchCommit( tableName ) ;
    }
}
