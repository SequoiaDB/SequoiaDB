import java.io.IOException ;
import java.sql.SQLException ;
import java.util.Formatter ;
import java.util.LinkedHashMap ;
import java.util.Map ;
import java.util.Random ;
import java.util.concurrent.atomic.AtomicInteger ;

import com.sequoiadb.exception.BaseException ;

public class LoadDataWorker implements Runnable {
    private int worker ;
    private jTPCCRandom rnd ;
    private int randomCID[] = null ;

    protected StringBuffer sb ;
    protected Formatter fmt ;

    protected static AtomicInteger thrdCnt = new AtomicInteger( 0 ) ;

    public LoadDataWorker( int worker, jTPCCRandom rnd ) {
        this.worker = worker ;
        this.rnd = rnd ;
        this.sb = new StringBuffer() ;
        this.fmt = new Formatter( sb ) ;
    }

    protected Map< String, String > getConfig() {
        Map< String, String > configs = new LinkedHashMap< String, String >() ;
        configs.put( "warehouses", "" + LoadData.getNumWarehouses() ) ;
        configs.put( "nURandCLast", "" + rnd.getNURandCLast() ) ;
        configs.put( "nURandCC_ID", "" + rnd.getNURandCC_ID() ) ;
        configs.put( "nURandCI_ID", "" + rnd.getNURandCI_ID() ) ;
        return configs ;
    }

    public Map< String, Object > getItem( int i_id ) {
        Map< String, Object > item = new LinkedHashMap< String, Object >() ;
        String iData ;

        // Clause 4.3.3.1 for ITEM
        if ( rnd.nextInt( 1, 100 ) <= 10 ) {
            int len = rnd.nextInt( 26, 50 ) ;
            int off = rnd.nextInt( 0, len - 8 ) ;
            iData = rnd.getAString( off, off ) + "ORIGINAL"
                    + rnd.getAString( len - off - 8, len - off - 8 ) ;
        } else {
            iData = rnd.getAString( 26, 50 ) ;
        }

        item.put( "i_id", i_id ) ;
        item.put( "i_im_id", rnd.nextInt( 1, 10000 ) ) ;
        item.put( "i_name", rnd.getAString( 14, 24 ) ) ;
        item.put( "i_price", ( ( double ) rnd.nextLong( 100, 10000 ) ) / 100.0 ) ;
        item.put( "i_data", iData ) ;

        return item ;
    }

    public Map< String, Object > getWarehouse( int w_id ) {
        Map< String, Object > wareHouse = new LinkedHashMap< String, Object >() ;
        wareHouse.put( "w_id", w_id ) ;
        wareHouse.put( "w_name", rnd.getAString( 6, 10 ) ) ;
        wareHouse.put( "w_street_1", rnd.getAString( 10, 20 ) ) ;
        wareHouse.put( "w_street_2", rnd.getAString( 10, 20 ) ) ;
        wareHouse.put( "w_city", rnd.getAString( 10, 20 ) ) ;
        wareHouse.put( "w_state", rnd.getState() ) ;
        wareHouse.put( "w_zip", rnd.getNString( 4, 4 ) + "11111" ) ;
        wareHouse.put( "w_tax",
                ( ( double ) rnd.nextLong( 0, 2000 ) ) / 10000.0 ) ;
        wareHouse.put( "w_ytd", 300000.0 ) ;

        return wareHouse ;
    }

    public Map< String, Object > getStock( int s_i_id, int w_id ) {
        Map< String, Object > stock = new LinkedHashMap< String, Object >() ;
        String sData ;

        // Clause 4.3.3.1 for STOCK
        if ( rnd.nextInt( 1, 100 ) <= 10 ) {
            int len = rnd.nextInt( 26, 50 ) ;
            int off = rnd.nextInt( 0, len - 8 ) ;

            sData = rnd.getAString( off, off ) + "ORIGINAL"
                    + rnd.getAString( len - off - 8, len - off - 8 ) ;
        } else {
            sData = rnd.getAString( 26, 50 ) ;
        }

        stock.put( "s_i_id", s_i_id ) ;
        stock.put( "s_w_id", w_id ) ;
        stock.put( "s_quantity", rnd.nextInt( 10, 100 ) ) ;
        stock.put( "s_dist_01", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_dist_02", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_dist_03", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_dist_04", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_dist_05", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_dist_06", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_dist_07", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_dist_08", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_dist_09", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_dist_10", rnd.getAString( 24, 24 ) ) ;
        stock.put( "s_ytd", 0 ) ;
        stock.put( "s_order_cnt", 0 ) ;
        stock.put( "s_remote_cnt", 0 ) ;
        stock.put( "s_data", sData ) ;

        return stock ;
    }

    public Map< String, Object > getDistrict( int d_id, int w_id ) {
        Map< String, Object > district = new LinkedHashMap< String, Object >() ;

        district.put( "d_id", d_id ) ;
        district.put( "d_w_id", w_id ) ;
        district.put( "d_name", rnd.getAString( 6, 10 ) ) ;
        district.put( "d_street_1", rnd.getAString( 10, 20 ) ) ;
        district.put( "d_street_2", rnd.getAString( 10, 20 ) ) ;
        district.put( "d_city", rnd.getAString( 10, 20 ) ) ;
        district.put( "d_state", rnd.getState() ) ;
        district.put( "d_zip", rnd.getNString( 4, 4 ) + "11111" ) ;
        district.put( "d_tax", ( ( double ) rnd.nextLong( 0, 2000 ) ) / 10000.0 ) ;
        district.put( "d_ytd", 30000.0 ) ;
        district.put( "d_next_o_id", 3001 ) ;

        return district ;
    }

    public Map< String, Object > getCustomer( int c_id, int d_id, int w_id ) {
        Map< String, Object > customer = new LinkedHashMap< String, Object >() ;

        customer.put( "c_id", c_id ) ;
        customer.put( "c_d_id", d_id ) ;
        customer.put( "c_w_id", w_id ) ;
        customer.put( "c_first", rnd.getAString( 8, 16 ) ) ;
        customer.put( "c_middle", "OE" ) ;
        if ( c_id <= 1000 ) {
            customer.put( "c_last", rnd.getCLast( c_id - 1 ) ) ;
        } else {
            customer.put( "c_last", rnd.getCLast() ) ;
        }

        customer.put( "c_street_1", rnd.getAString( 10, 20 ) ) ;
        customer.put( "c_street_2", rnd.getAString( 10, 20 ) ) ;
        customer.put( "c_city", rnd.getAString( 10, 20 ) ) ;
        customer.put( "c_state", rnd.getState() ) ;
        customer.put( "c_zip", rnd.getNString( 4, 4 ) + "11111" ) ;
        customer.put( "c_phone", rnd.getNString( 16, 16 ) ) ;
        customer.put( "c_since",
                             System.currentTimeMillis() ) ;
        if ( rnd.nextInt( 1, 100 ) <= 90 ) {
            customer.put( "c_credit", "GC" ) ;
        } else {
            customer.put( "c_credit", "BC" ) ;
        }

        customer.put( "c_credit_lim", 50000.00 ) ;
        customer.put( "c_discount",
                ( ( double ) rnd.nextLong( 0, 5000 ) ) / 10000.0 ) ;
        customer.put( "c_balance", -10.00 ) ;
        customer.put( "c_ytd_payment", 10.00 ) ;
        customer.put( "c_payment_cnt", 1 ) ;
        customer.put( "c_delivery_cnt", 1 ) ;
        customer.put( "c_data", rnd.getAString( 300, 500 ) ) ;

        return customer ;
    }

    public Map< String, Object > getHistory( int w_id, int c_id, int d_id ) {
        Map< String, Object > history = new LinkedHashMap< String, Object >() ;
        history.put( "hist_id", ( w_id - 1 ) * 30000 + ( d_id - 1 ) * 3000
                + c_id ) ;
        history.put( "h_c_id", c_id ) ;
        history.put( "h_c_d_id", d_id ) ;
        history.put( "h_c_w_id", w_id ) ;
        history.put( "h_d_id", d_id ) ;
        history.put( "h_w_id", w_id ) ;
        history.put( "h_date",
                 System.currentTimeMillis() ) ;
        history.put( "h_amount", 10.00 ) ;
        history.put( "h_data", rnd.getAString( 12, 24 ) ) ;
        return history ;
    }

    public Map< String, Object > getOrder( int o_id, int d_id, int w_id,
            int o_ol_cnt ) {

        /*
         * For the ORDER rows the TPC-C specification demands that they are
         * generated using a random permutation of all 3,000 customers. To do
         * that we set up an array with all C_IDs and then randomly shuffle it.
         */
        if ( randomCID == null ) {
            randomCID = new int[ 3000 ] ;
            for ( int i = 0; i < 3000; i++ ) {
                randomCID[i] = i + 1 ;
            }

            for ( int i = 0; i < 3000; i++ ) {
                int x = rnd.nextInt( 0, 2999 ) ;
                int y = rnd.nextInt( 0, 2999 ) ;
                int tmp = randomCID[x] ;
                randomCID[x] = randomCID[y] ;
                randomCID[y] = tmp ;
            }
        }

        Map< String, Object > order = new LinkedHashMap< String, Object >() ;
        order.put( "o_id", o_id ) ;
        order.put( "o_d_id", d_id ) ;
        order.put( "o_w_id", w_id ) ;
        order.put( "o_c_id", randomCID[o_id - 1] ) ;
        order.put( "o_entry_d",
                 System.currentTimeMillis() ) ;
        if ( o_id < 2101 ) {
            order.put( "o_carrier_id", rnd.nextInt( 1, 10 ) ) ;
        } else {
            order.put( "o_carrier_id", java.sql.Types.INTEGER ) ;
        }
        order.put( "o_ol_cnt", o_ol_cnt ) ;
        order.put( "o_all_local", 1 ) ;

        return order ;
    }

    public Map< String, Object > getOrderLine( int o_id, int d_id, int w_id,
            int ol_number ) {
        Map< String, Object > orderLine = new LinkedHashMap< String, Object >() ;
        orderLine.put( "ol_o_id", o_id ) ;
        orderLine.put( "ol_d_id", d_id ) ;
        orderLine.put( "ol_w_id", w_id ) ;
        orderLine.put( "ol_number", ol_number ) ;
        orderLine.put( "ol_i_id", rnd.nextInt( 1, 100000 ) ) ;
        orderLine.put( "ol_supply_w_id", w_id ) ;
        if ( o_id < 2101 )
            orderLine.put( "ol_delivery_d",
                     System.currentTimeMillis()) ;
        else
            orderLine.put( "ol_delivery_d", java.sql.Types.TIMESTAMP ) ;
        orderLine.put( "ol_quantity", 5 ) ;
        if ( o_id < 2101 )
            orderLine.put( "ol_amount", 0.00 ) ;
        else
            orderLine.put( "ol_amount",
                    ( ( double ) rnd.nextLong( 1, 999999 ) ) / 100.0 ) ;

        orderLine.put( "ol_dist_info", rnd.getAString( 24, 24 ) ) ;
        return orderLine ;

    }

    public Map< String, Object > getNewOrder( int o_id, int d_id, int w_id ) {
        Map< String, Object > newOrder = new LinkedHashMap< String, Object >() ;
        newOrder.put( "no_o_id", o_id ) ;
        newOrder.put( "no_d_id", d_id ) ;
        newOrder.put( "no_w_id", w_id ) ;

        return newOrder ;
    }

    protected void Done() throws SQLException, BaseException, IOException {

    }

    protected void loadConfig() throws SQLException, BaseException, IOException {

    }

    protected void buildItemRecord( int i_id ) throws SQLException,
            BaseException, IOException {

    }

    protected void loadWarehouseRecord( int w_id ) throws SQLException,
            BaseException, IOException {

    }

    protected void buildStockRecord( int s_i_id, int w_id )
            throws SQLException, BaseException, IOException {

    }

    protected void loadDistrictRecord( int d_id, int w_id )
            throws SQLException, BaseException, IOException {

    }

    protected void buildCustomerRecord( int c_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {

    }

    protected void buildHistoryRecord( int c_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {

    }

    protected void buildOrderRecord( int o_id, int w_id, int d_id, int o_ol_cnt )
            throws SQLException, BaseException, IOException {

    }

    protected void buildOrderLineRecord( int w_id, int d_id, int o_id,
            int ol_number ) throws SQLException, BaseException, IOException {

    }

    protected void buildNewOrder( int o_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {

    }

    protected void batchCommit( String tableName ) throws SQLException,
            BaseException, IOException {

    }

    protected void Commit( String tableName ) throws SQLException,
            BaseException, IOException {

    }

    /*
     * ---- loadItem()
     * 
     * Load the content of the ITEM table. ----
     */
    protected void loadItem() throws SQLException, BaseException, IOException {
        loadConfig() ;
        int i_id ;
        for ( i_id = 1; i_id <= 100000; i_id++ ) {
            if ( i_id != 1 && ( i_id - 1 ) % 10000 == 0 ) {
                batchCommit( "item" ) ;
            }
            buildItemRecord( i_id ) ;
        }

        Commit( "item" ) ;
    }

    protected void loadWarehouse( int w_id ) throws SQLException, IOException {

        loadWarehouseRecord( w_id ) ;
        /*
         * For each WAREHOUSE there are 100,000 STOCK rows.
         */
        for ( int s_i_id = 1; s_i_id <= 100000; s_i_id++ ) {
            /*
             * Load the data in batches of 10,000 rows.
             */
            if ( s_i_id != 1 && ( s_i_id - 1 ) % 10000 == 0 ) {
                batchCommit( "stock" ) ;
            }
            buildStockRecord( s_i_id, w_id ) ;
        }
        Commit( "stock" ) ;

        /*
         * For each WAREHOUSE there are 10 DISTRICT rows.
         */
        for ( int d_id = 1; d_id <= 10; d_id++ ) {
            loadDistrictRecord( d_id, w_id ) ;
            /*
             * Within each DISTRICT there are 3,000 CUSTOMERs.
             */
            for ( int c_id = 1; c_id <= 3000; c_id++ ) {
                buildCustomerRecord( c_id, d_id, w_id ) ;
                buildHistoryRecord( c_id, d_id, w_id ) ;
            }
            batchCommit( "customer" ) ;
            batchCommit( "history" ) ;
            for ( int o_id = 1; o_id <= 3000; o_id++ ) {
                int o_ol_cnt = rnd.nextInt( 5, 15 ) ;
                buildOrderRecord( o_id, d_id, w_id, o_ol_cnt ) ;

                /*
                 * Create the ORDER_LINE rows for this ORDER.
                 */
                for ( int ol_number = 1; ol_number <= o_ol_cnt; ol_number++ ) {
                    buildOrderLineRecord( w_id, d_id, o_id, ol_number ) ;
                }

                /*
                 * The last 900 ORDERs are not yet delieverd and have a row in
                 * NEW_ORDER.
                 */
                if ( o_id >= 2101 ) {
                    buildNewOrder( o_id, d_id, w_id ) ;
                }
            }
            batchCommit( "order" ) ;
            batchCommit( "orderline" ) ;
            batchCommit( "neworder" ) ;
        }
        Commit( "other" ) ;
    }

    public void load() throws SQLException, BaseException, IOException {
        int job ;
        while ( ( job = LoadData.getNextJob() ) >= 0 ) {
            if ( job == 0 ) {
                fmt.format( "Worker %03d: Loading ITEM", worker ) ;
                System.out.println( sb.toString() ) ;
                sb.setLength( 0 ) ;

                loadItem() ;

                fmt.format( "Worker %03d: Loading ITEM done", worker ) ;
                System.out.println( sb.toString() ) ;
                sb.setLength( 0 ) ;
            } else {
                fmt.format( "Worker %03d: Loading Warehouse %6d", worker, job ) ;
                System.out.println( sb.toString() ) ;
                sb.setLength( 0 ) ;

                loadWarehouse( job ) ;

                fmt.format( "Worker %03d: Loading Warehouse %6d done", worker,
                        job ) ;
                System.out.println( sb.toString() ) ;
                sb.setLength( 0 ) ;
            }
        }

        /*
         * Close the DB connection if in direct DB mode.
         */
        Done() ;
    }

    protected int fini() {
        return thrdCnt.decrementAndGet() ;
    }

    /*
     * run()
     */
    public void run() {
        try {
            thrdCnt.incrementAndGet() ;
            load() ;
        } catch ( SQLException se ) {
            se.printStackTrace() ;
            while ( se != null ) {
                fmt.format( "Worker %03d: ERROR: %s", worker, se.getMessage() ) ;
                System.err.println( sb.toString() ) ;
                sb.setLength( 0 ) ;
                se = se.getNextException() ;
            }
            System.exit( 2 ) ;
        } catch ( BaseException e ) {
            fmt.format( "Worker %03d: ERROR: %d", worker, e.getErrorCode() ) ;
            System.err.println( sb.toString() ) ;
            sb.setLength( 0 ) ;
            e.printStackTrace() ;
            System.exit( 2 ) ;
        } catch ( IOException e ) {
            fmt.format( "Worker %03d: ERROR: %s", worker, e.getMessage() ) ;
            System.err.println( sb.toString() ) ;
            sb.setLength( 0 ) ;
            e.printStackTrace() ;
			System.exit( 2 ) ;
        }
        fini() ;
        return ;
    }
}
