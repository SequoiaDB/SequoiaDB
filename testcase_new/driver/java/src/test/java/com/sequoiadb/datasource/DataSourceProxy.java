/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:DataSourceProxy.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2020年7月6日上午9:35:03
 *  @version 1.00
 */
package com.sequoiadb.datasource;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class DataSourceProxy {
    private SequoiadbDatasource ds = null;
    private AtomicInteger refCount = new AtomicInteger(0) ;
    private String coordAddr ;
    private String userName ;
    private String password;
    
    public DataSourceProxy(String coordAddr, String userName, String password) {
        init(coordAddr, userName, password); 
        this.coordAddr = coordAddr;
        this.userName = userName ;
        this.password = password ;
    }
    
    public synchronized void init(String coordAddr, String userName, String password) {
        if ( refCount.get() > 0 ) return ;
        DatasourceOptions sdbOption = new DatasourceOptions();
        sdbOption.setMaxCount( 500 );
        sdbOption.setMaxIdleCount( 10 );
        sdbOption.setCheckInterval( 5 * 1000  );
        
        ConfigOptions connectOpt = new ConfigOptions();
        connectOpt.setConnectTimeout( 10000 );
        connectOpt.setMaxAutoConnectRetryTime( 0 );
        
        ArrayList< String > urls = new ArrayList< String >();
        urls.add( coordAddr );
        ds = new SequoiadbDatasource( urls, userName, password,
                    connectOpt, sdbOption );
        refCount.incrementAndGet();
    }
    
    public Sequoiadb getConnection() throws BaseException, InterruptedException {
        if ( refCount.get() <= 0 ) {
            init(this.coordAddr, this.userName, this.password) ;
        }
        Sequoiadb sdb =  ds.getConnection() ;
        return sdb ;
    }
    
    public DatasourceOptions getDatasourceOptions() {
        if ( refCount.get() <= 0 ) {
            init(this.coordAddr, this.userName, this.password) ;
        }
        DatasourceOptions opt = ds.getDatasourceOptions();
        return opt ;
    }
    
    public void close() {
        if (refCount.decrementAndGet() == 0) {
            ds.close();
        }
    }
    
    public void releaseConnection(Sequoiadb sdb) {
        if ( refCount.get() <= 0 ) {
            init(this.coordAddr, this.userName, this.password) ;
        }
     
        ds.releaseConnection( sdb );
    }
    
    public void addCoord(String url) {
        if ( refCount.get() <= 0 ) {
            init(this.coordAddr, this.userName, this.password) ;
        }
        ds.addCoord( url );
    }
    
    public void removeCoord(String url) {
        if ( ds == null ) {
            init(this.coordAddr, this.userName, this.password) ;
        }
        ds.removeCoord( url );
    }
    
    public void updateDatasourceOptions(DatasourceOptions dsOpt) {
        if ( refCount.get() <= 0 ) {
            init(this.coordAddr, this.userName, this.password) ;
        }
        ds.updateDatasourceOptions( dsOpt );
    }
    
    public void enableDatasource() {
        if ( refCount.get() <= 0 ) {
            init(this.coordAddr, this.userName, this.password) ;
        }
        ds.enableDatasource();
    }
    
    public void disableDatasource() {
        if ( refCount.get() <= 0 ) {
            init(this.coordAddr, this.userName, this.password) ;
        }
        ds.disableDatasource();
    }
    
    public Sequoiadb getConnection(long timeout) throws BaseException, InterruptedException {
        if ( refCount.get() <= 0 ) {
            init(this.coordAddr, this.userName, this.password) ;
        }
        return ds.getConnection( timeout ) ;
    }
}
