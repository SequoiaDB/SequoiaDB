package com.sequoiadb.metaopr.commons;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;

import java.util.ArrayList;
import java.util.List;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-5-4
 * @Version 1.00
 */
public class MyDataSource {
    private static SequoiadbDatasource _ds;

    private MyDataSource() {
    }

    public static SequoiadbDatasource getDataSource() {
        if ( _ds != null )
            return _ds;
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMaxCount( 500 );
        dsOpt.setDeltaIncCount( 20 );
        dsOpt.setMaxIdleCount( 20 );
        dsOpt.setCheckInterval( 60 * 1000 );
        dsOpt.setValidateConnection( true );
        ConfigOptions configOptions = new ConfigOptions();
        // 不打开keepalive，某一端断网后，另一端无法感知。
        configOptions.setSocketKeepAlive( true );
        List< String > urls = new ArrayList<>( 10 );
        urls.add( SdbTestBase.coordUrl );
        _ds = new SequoiadbDatasource( urls, "", "", configOptions, dsOpt );
        return _ds;
    }
}
