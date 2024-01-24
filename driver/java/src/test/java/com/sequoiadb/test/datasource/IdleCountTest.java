package com.sequoiadb.test.datasource;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.DatasourceOptions;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.junit.*;

import java.util.ArrayList;
import java.util.List;


public class IdleCountTest {

    @Test
    public void testInit() throws Exception {
        DatasourceOptions dsOpt = new DatasourceOptions();

        // case 1: default value
        Assert.assertEquals(0, dsOpt.getMinIdleCount());
        Assert.assertEquals(10, dsOpt.getMaxIdleCount());
        // min 0, max 10
        checkInitDS(dsOpt.getMinIdleCount(), dsOpt.getMaxIdleCount());

        // case 2: min 0, max 0
        checkInitDS(0, 0);

        // case 3: min = 5, max = 10
        checkInitDS(5, 10);

        // case 4: min = 10, max = 10;
        checkInitDS(10, 10);

        // case 5: min = 10, max = 5
        try {
            checkInitDS(10, 5);
        } catch (BaseException e) {
            Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
        }
    }

    private void checkInitDS(int min, int max) throws Exception {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMaxCount(30);
        dsOpt.setMinIdleCount(min);
        dsOpt.setMaxIdleCount(max);
        dsOpt.setCheckInterval(500); // 500ms

        SequoiadbDatasource ds = new SequoiadbDatasource(Constants.COOR_NODE_CONN, "", "", dsOpt);
        try {
            checkIdleCount(ds, dsOpt);
        } finally {
            ds.close();
        }
    }

    @Test
    public void testUpdateConf() throws Exception {
        DatasourceOptions dsOpt = new DatasourceOptions();
        dsOpt.setMaxCount(30);
        dsOpt.setCheckInterval(500);
        dsOpt.setMinIdleCount(0);
        dsOpt.setMaxIdleCount(20);
        SequoiadbDatasource ds = new SequoiadbDatasource(Constants.COOR_NODE_CONN, "", "", dsOpt);
        try {
            // min = 0, max = 20
            checkIdleCount(ds, dsOpt);

            // case 1: update min = 0, max = 10
            checkUpdateConf(ds, 0, 10);

            // case 2: update min = 5, max = 10
            checkUpdateConf(ds, 5, 10);

            // case 3: update min = 10, max = 10
            checkUpdateConf(ds, 10, 10);

            // case 4: update min = 20, max = 10
            try {
                checkUpdateConf(ds, 20, 10);
            } catch (BaseException e) {
                Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
            }

            // case 5: update min = 10, max = 5
            try {
                checkUpdateConf(ds, 10, 5);
            } catch (BaseException e) {
                Assert.assertEquals(SDBError.SDB_INVALIDARG.getErrorCode(), e.getErrorCode());
            }
        } finally {
            ds.close();
        }
    }

    private void checkUpdateConf(SequoiadbDatasource ds, int min, int max) throws Exception {
        DatasourceOptions dsOpt = ds.getDatasourceOptions();
        dsOpt.setMinIdleCount(min);
        dsOpt.setMaxIdleCount(max);

        ds.updateDatasourceOptions(dsOpt);
        Assert.assertEquals(dsOpt.getMinIdleCount(), ds.getDatasourceOptions().getMinIdleCount());
        Assert.assertEquals(dsOpt.getMaxIdleCount(), ds.getDatasourceOptions().getMaxIdleCount());
    }

    private void checkIdleCount(SequoiadbDatasource ds, DatasourceOptions dsOpt) throws Exception {
        int avgCount = (dsOpt.getMinIdleCount() + dsOpt.getMaxIdleCount()) / 2;
        List<Sequoiadb> connList = new ArrayList<>();

        Assert.assertEquals(dsOpt.getMinIdleCount(), ds.getDatasourceOptions().getMinIdleCount());
        Assert.assertEquals(dsOpt.getMaxIdleCount(), ds.getDatasourceOptions().getMaxIdleCount());

        connList.add(ds.getConnection());
        // wait for CreateConnectionTask
        Thread.sleep(dsOpt.getCheckInterval());
        Assert.assertEquals(avgCount, ds.getIdleConnNum());

        // idleCount < avgCount, new connection will create by CreateConnectionTask
        connList.add(ds.getConnection());
        // wait for CheckConnectionTask and CreateConnectionTask
        Thread.sleep(dsOpt.getCheckInterval() * 2L);
        Assert.assertEquals(avgCount, ds.getIdleConnNum());

        for (Sequoiadb db: connList) {
            ds.releaseConnection(db);
        }
    }
}
