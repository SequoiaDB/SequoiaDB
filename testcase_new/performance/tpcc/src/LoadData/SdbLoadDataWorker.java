import java.io.IOException ;
import java.sql.SQLException ;
import java.util.ArrayList ;
import java.util.Formatter ;
import java.util.Iterator ;
import java.util.List ;
import java.util.Map ;

import org.bson.BasicBSONObject ;
import org.bson.BSONObject ;

import com.sequoiadb.base.CollectionSpace ;
import com.sequoiadb.base.DBCollection ;
import com.sequoiadb.base.Sequoiadb ;
import com.sequoiadb.base.SequoiadbDatasource ;
import com.sequoiadb.exception.BaseException ;
import com.sequoiadb.datasource.DatasourceOptions ;

public class SdbLoadDataWorker extends LoadDataWorker {

    List< BSONObject > configs = new ArrayList< BSONObject >() ;
    private DBCollection clConfig = null ;
    List< BSONObject > items = new ArrayList< BSONObject >() ;
    private DBCollection clItem = null ;
    List< BSONObject > warehouses = new ArrayList< BSONObject >() ;
    private DBCollection clWarehouse = null ;
    List< BSONObject > districts = new ArrayList< BSONObject >() ;
    private DBCollection clDistrict = null ;
    List< BSONObject > stocks = new ArrayList< BSONObject >() ;
    private DBCollection clStock = null ;
    List< BSONObject > customers = new ArrayList< BSONObject >() ;
    private DBCollection clCustomer = null ;
    List< BSONObject > historys = new ArrayList< BSONObject >() ;
    private DBCollection clHistory = null ;
    List< BSONObject > orders = new ArrayList< BSONObject >() ;
    private DBCollection clOrder = null ;
    List< BSONObject > orderlines = new ArrayList< BSONObject >() ;
    private DBCollection clOrderLine = null ;
    List< BSONObject > neworders = new ArrayList< BSONObject >() ;
    private DBCollection clNewOrder = null ;
    private Sequoiadb sdb = null ;

    private static SequoiadbDatasource ds = null ;

    public SdbLoadDataWorker( int worker, String url, jTPCCRandom rnd )
            throws BaseException {
        super( worker, rnd ) ;
        if ( ds == null ) {
            synchronized ( SdbLoadDataWorker.class ) {
                if ( ds == null ) {
                    String[] urls = url.split( "," ) ;
                    List< String > coordUrls = new ArrayList< String >() ;
                    for ( int i = 0; i < urls.length; ++i ) {
                        String[] pair = urls[i].split( ":" ) ;
                        if ( pair.length == 2 ) {
                            coordUrls.add( urls[i] ) ;
                        } else {
                            coordUrls.add( urls[i] + ":" + 11810 ) ;
                        }
                    }
                    DatasourceOptions opts = null ;
                    ds = new SequoiadbDatasource( coordUrls, "", "", null, opts ) ;
                }
            }
        }
        // TODO Auto-generated constructor stub
        try {
            sdb = ds.getConnection() ;
            CollectionSpace cs = sdb.getCollectionSpace( "tpc_c" ) ;

            clConfig = cs.getCollection( "config" ) ;
            clItem = cs.getCollection( "item" ) ;
            clWarehouse = cs.getCollection( "warehouse" ) ;
            clDistrict = cs.getCollection( "district" ) ;
            clStock = cs.getCollection( "stock" ) ;
            clCustomer = cs.getCollection( "customer" ) ;
            clHistory = cs.getCollection( "history" ) ;
            clOrder = cs.getCollection( "oorder" ) ;
            clOrderLine = cs.getCollection( "order_line" ) ;
            clNewOrder = cs.getCollection( "new_order" ) ;
        } catch ( InterruptedException e ) {
            e.printStackTrace() ;
        } catch ( BaseException e ) {
            e.printStackTrace() ;
        }

    }

    protected void loadConfig() throws SQLException, BaseException, IOException {
        Map< String, String > records = getConfig() ;

        /*
         * Saving CONFIG information in DB mode.
         */
        Iterator< Map.Entry< String, String >> it = records.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            BasicBSONObject doc = new BasicBSONObject() ;
            Map.Entry< String, String > entry = it.next() ;
            doc.put( "cfg_name", entry.getKey() );
            doc.put( "cfg_value", entry.getValue() );
            configs.add( doc ) ;
        }
        clConfig.bulkInsert( configs, 0 ) ;
    }

    protected void buildItemRecord( int i_id ) throws SQLException,
            BaseException, IOException {
        Map< String, Object > record = getItem( i_id ) ;

        BasicBSONObject doc = new BasicBSONObject() ;
        Iterator< Map.Entry< String, Object >> it = record.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, Object > entry = it.next() ;
            doc.put( entry.getKey(), entry.getValue() ) ;
        }
        items.add( doc ) ;
    }

    protected void loadWarehouseRecord( int w_id ) throws SQLException,
            BaseException, IOException {
        /*
         * Load the WAREHOUSE row.
         */
        Map< String, Object > record = getWarehouse( w_id ) ;
        BasicBSONObject doc = new BasicBSONObject() ;
        Iterator< Map.Entry< String, Object >> it = record.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, Object > entry = it.next() ;
            doc.put( entry.getKey(), entry.getValue() ) ;

        }
        clWarehouse.insert( doc ) ;
    }

    protected void buildStockRecord( int s_i_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getStock( s_i_id, w_id ) ;
        BasicBSONObject doc = new BasicBSONObject() ;
        Iterator< Map.Entry< String, Object >> it = record.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, Object > entry = it.next() ;
            doc.put( entry.getKey(), entry.getValue() ) ;
        }
        stocks.add( doc ) ;
    }

    protected void loadDistrictRecord( int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getDistrict( d_id, w_id ) ;
        BasicBSONObject doc = new BasicBSONObject() ;
        Iterator< Map.Entry< String, Object >> it = record.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, Object > entry = it.next() ;
            doc.put( entry.getKey(), entry.getValue() ) ;
        }
        clDistrict.insert( doc ) ;
    }

    protected void buildCustomerRecord( int c_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getCustomer( c_id, d_id, w_id ) ;
        BasicBSONObject doc = new BasicBSONObject() ;
        Iterator< Map.Entry< String, Object >> it = record.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, Object > entry = it.next() ;
            if (entry.getKey().equals("c_since")) {
               doc.put( entry.getKey(), new java.sql.Timestamp((long) entry.getValue()));
            }else{
               doc.put( entry.getKey(), entry.getValue() ) ;
            }

        }
        customers.add( doc ) ;
    }

    protected void buildHistoryRecord( int c_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getHistory( w_id, c_id, d_id ) ;

        BasicBSONObject doc = new BasicBSONObject() ;
        Iterator< Map.Entry< String, Object >> it = record.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, Object > entry = it.next() ;
            if (entry.getKey().equals("h_date")) {
               doc.put( entry.getKey(), new java.sql.Timestamp((long) entry.getValue() ) );
            }else{
               doc.put( entry.getKey(), entry.getValue() ) ;
            }

        }
        historys.add( doc ) ;
    }

    protected void buildOrderRecord( int o_id, int w_id, int d_id, int o_ol_cnt )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getOrder( o_id, w_id, d_id, o_ol_cnt ) ;

        BasicBSONObject doc = new BasicBSONObject() ;
        Iterator< Map.Entry< String, Object >> it = record.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, Object > entry = it.next() ;
            if ( entry.getKey().equals("o_entry_d") ){
               doc.put( entry.getKey(), new java.sql.Timestamp((long)entry.getValue()) ) ;
            }else if ( entry.getKey().equals("o_carrier_id") && entry.getValue() == java.sql.Types.INTEGER ){
               continue ;
            }else{
               doc.put( entry.getKey(), entry.getValue() ) ;
            }
        }
        orders.add( doc ) ;
    }

    protected void buildOrderLineRecord( int w_id, int d_id, int o_id,
            int ol_number ) throws SQLException, BaseException, IOException {
        Map< String, Object > record = getOrderLine( o_id, d_id, w_id,
                ol_number ) ;
        BasicBSONObject doc = new BasicBSONObject() ;
        Iterator< Map.Entry< String, Object >> it = record.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, Object > entry = it.next() ;
            if ( entry.getKey().equals("ol_delivery_d") && entry.getValue() == java.sql.Types.TIMESTAMP ) {
               continue ;
            }else if ( entry.getKey().equals("ol_delivery_d") ) {
               doc.put( entry.getKey(), new java.sql.Timestamp((long)entry.getValue()) ) ;
            }else{
               doc.put( entry.getKey(), entry.getValue() ) ;
            }
        }
        orderlines.add( doc ) ;
    }

    protected void buildNewOrder( int o_id, int d_id, int w_id )
            throws SQLException, BaseException, IOException {
        Map< String, Object > record = getNewOrder( o_id, d_id, w_id ) ;
        BasicBSONObject doc = new BasicBSONObject() ;
        Iterator< Map.Entry< String, Object >> it = record.entrySet()
                .iterator() ;
        while ( it.hasNext() ) {
            Map.Entry< String, Object > entry = it.next() ;
            doc.put( entry.getKey(), entry.getValue() ) ;
        }
        neworders.add( doc ) ;
    }

    protected void batchCommit( String tableName ) throws SQLException,
            BaseException, IOException {
        if ( tableName.equals( "item" ) ) {
            clItem.bulkInsert( items, 0 ) ;
            items.clear() ;
        } else if ( tableName.equals( "stock" ) ) {
            clStock.bulkInsert( stocks, 0 ) ;
            stocks.clear() ;
        } else if ( tableName.equals( "customer" ) ) {
            clCustomer.bulkInsert( customers, 0 ) ;
            customers.clear() ;
        } else if ( tableName.equals( "history" ) ) {
            clHistory.bulkInsert( historys, 0 ) ;
            historys.clear() ;
        } else if ( tableName.equals( "order" ) ) {
            clOrder.bulkInsert( orders, 0 ) ;
            orders.clear() ;
        } else if ( tableName.equals( "orderline" ) ) {
            clOrderLine.bulkInsert( orderlines, 0 ) ;
            orderlines.clear() ;
        } else if ( tableName.equals( "neworder" ) ) {
            clNewOrder.bulkInsert( neworders, 0 ) ;
            neworders.clear() ;
        }
    }

    protected void Commit( String tableName ) throws SQLException,
            BaseException, IOException {
        batchCommit( tableName ) ;
    }

    protected void Done() throws SQLException, BaseException, IOException {
        ds.releaseConnection( sdb ) ;
    }

    protected int fini() {
        if ( super.fini() == 0 ) {
            ds.close() ;
        }
        return 0 ;
    }
}
