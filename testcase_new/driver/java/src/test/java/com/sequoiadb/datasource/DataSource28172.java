package com.sequoiadb.datasource;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * @Description seqDB-28172:DatasourceOptions类set方法中单个参数校验
 * @author xumingxing
 * @Date 2022.10.05
 * @version 1.10
 */

public class DataSource28172 extends SdbTestBase {
    private SequoiadbDatasource ds = null;

    @BeforeClass
    public void setUp() {
    }

    @Test
    public void test() throws Exception {
        // set the illegal parameter, it will throw BaseException
        DatasourceOptions options = new DatasourceOptions();
        try {
            options.setMinIdleCount( -1 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }

        try {
            options.setCheckInterval( 0 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }
        try {
            options.setCheckInterval( -1 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }

        List< String > preferredInstance = new ArrayList();
        // illegal string
        preferredInstance.add( "C" );
        // illegal number
        preferredInstance.add( "0" );
        preferredInstance.add( "256" );
        try {
            options.setPreferredInstance( preferredInstance );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }

        try {
            options.setPreferredInstanceMode( " " );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }

        try {
            options.setNetworkBlockTimeout( -1 );
            Assert.fail( "unexpect result" );
        } catch ( BaseException e ) {
            Assert.assertEquals( SDBError.SDB_INVALIDARG.getErrorCode(),
                    e.getErrorCode() );
        }

        // case2: illegal parameter will be set to minimum value
        options.setSessionTimeout( -2 );
        Assert.assertEquals( options.getSessionTimeout(), -1 );

        options.setCacheLimit( -1 );
        Assert.assertEquals( options.getCacheLimit(), 0 );
    }

    @AfterClass
    public void tearDown() {
        if ( ds != null ) {
            ds.close();
        }
    }
}