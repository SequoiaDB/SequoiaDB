package com.sequoiadb.datasource;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

public class DatasourceOptionsTest {


    @Test
    public void checkSingleDatasourceOption() {
        DatasourceOptions options = new DatasourceOptions();

        // case1: single variable, set the illegal parameter, it will throw BaseException.
        try {
            options.setDeltaIncCount(0);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", deltaIncCount should be more than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setMaxIdleCount(-1);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", maxIdleCount can't be less than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setMinIdleCount(-1);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", minIdleCount can't be less than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setMaxCount(-1);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", maxCount can't be less than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setKeepAliveTimeout(-1);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", keepAliveTimeout can't be less than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setCheckInterval(0);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", checkInterval should be more than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setSyncCoordInterval(-1);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", syncCoordInterval can't be less than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        List<String> preferredInstance = new ArrayList();
        // illegal string
        preferredInstance.add("B");
        // illegal number
        preferredInstance.add("256");
        preferredInstance.add("0");
        try {
            options.setPreferredInstance(preferredInstance);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", error preferred instance value");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setPreferredInstanceMode("Test");
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", error preferred instance mode value");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setNetworkBlockTimeout(-1);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", networkBlockTimeout can't be less than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case2: illegal parameter will be set to minimum value
        options.setSessionTimeout(-2);
        Assert.assertEquals(options.getSessionTimeout(), -1);

        options.setCacheLimit(-1);
        Assert.assertEquals(options.getCacheLimit(), 0);

        // case3: deprecated interface but still have implementation
        try {
            options.setMaxIdeNum(-1);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", maxIdleCount can't be less than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setMaxConnectionNum(-1);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", maxCount can't be less than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setRecheckCyclePeriod(0);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", checkInterval should be more than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setAbandonTime(-1);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", keepAliveTimeout can't be less than 0");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        List<String> preferedInstance = new ArrayList<>();
        preferedInstance.add("B");
        preferedInstance.add("256");
        preferedInstance.add("0");
        try {
            options.setPreferedInstance(preferredInstance);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", error preferred instance value");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        try {
            options.setPreferedInstanceMode("Test");
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", error preferred instance mode value");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    @Test
    public void checkMultipleDatasourceOption() {
        SequoiadbDatasource ds;

        // case1: minIdleCount > maxIdleCount
        DatasourceOptions options = new DatasourceOptions();
        options.setMinIdleCount(20);
        options.setMaxIdleCount(10);
        try {
            ds = new SequoiadbDatasource(Constants.COOR_NODE_CONN, "", "", options);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", minIdleCount can't be more than maxIdleCount");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case2 keepAliveTime != 0 && checkInterval > keepAliveTimeout
        options = new DatasourceOptions();
        options.setKeepAliveTimeout(10);
        options.setCheckInterval(20);
        try {
            ds = new SequoiadbDatasource(Constants.COOR_NODE_CONN, "", "", options);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", when keepAliveTimeout is not 0, checkInterval should be less than keepAliveTimeout");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case3: maxCount != 0 && deltaIncCount > maxCount
        options = new DatasourceOptions();
        options.setDeltaIncCount(20);
        options.setMaxCount(10);
        try {
            ds = new SequoiadbDatasource(Constants.COOR_NODE_CONN, "", "", options);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", deltaIncCount can't be more than maxCount");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

        // case4: maxCount != 0 && maxIdleCount > maxCount
        options = new DatasourceOptions();
        options.setMaxIdleCount(20);
        options.setMaxCount(10);
        try {
            ds = new SequoiadbDatasource(Constants.COOR_NODE_CONN, "", "", options);
            Assert.fail("Should get " + SDBError.SDB_INVALIDARG + ", maxIdleCount can't be more than maxCount");
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }

    }
}
