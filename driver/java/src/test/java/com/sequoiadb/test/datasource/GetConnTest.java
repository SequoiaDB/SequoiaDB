package com.sequoiadb.test.datasource;

import com.sequoiadb.base.ConfigOptions;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.datasource.SequoiadbDatasource;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.common.Constants;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.junit.*;

public class GetConnTest {

    @Test
    public void test() {
        // invalid port
        String address = Constants.HOST + ":" + 51000;
        Assert.assertFalse(checkNode(address));
        testAddress(address);
    }

    private boolean checkNode(String nodeName) {
        try (Sequoiadb db = new Sequoiadb(Constants.COOR_NODE_CONN, "", "")){
            BSONObject matcher = new BasicBSONObject("NodeName", nodeName);
            BSONObject selector = new BasicBSONObject("NodeName", "");

            try (DBCursor cursor = db.getSnapshot(Sequoiadb.SDB_SNAP_HEALTH, matcher, selector, null)) {
                return cursor.hasNext();
            }
        } catch (BaseException e){
            if (e.getErrorCode() == SDBError.SDB_CLS_NODE_NOT_EXIST.getErrorCode()) {
                return false;
            }
            throw e;
        }
    }

    private void testAddress(String address) {
        // case 1: connTime = 0
        testTimeOut(address, 2000, 0, 2000, SDBError.SDB_TIMEOUT.getErrorCode());

        // case 2: retryTime = 0
        testTimeOut(address, 2000, 2000, 0, SDBError.SDB_TIMEOUT.getErrorCode());

        // case 3: connTime < retryTime
        testTimeOut(address, 3000, 1000, 3000, SDBError.SDB_TIMEOUT.getErrorCode());

        // case 4: connTime = retryTime
        testTimeOut(address, 3000, 2000, 2000, SDBError.SDB_NETWORK.getErrorCode());

        // case 5: connTime > retryTime
        testTimeOut(address, 3000, 2000, 1000, SDBError.SDB_NETWORK.getErrorCode());

        // case 6: timeout = 0
        testTimeOut(address, 0, 2000, 2000, SDBError.SDB_NETWORK.getErrorCode());

        // case 7: timeout < connTime
        testTimeOut(address, 1000, 2000, 3000, SDBError.SDB_TIMEOUT.getErrorCode());
    }

    private void testTimeOut(String address, int timeout,int connTime, int retryTime, int errorCode) {
        int deviationTime = 1000;
        ConfigOptions netOpt = new ConfigOptions();
        netOpt.setConnectTimeout(connTime);
        netOpt.setMaxAutoConnectRetryTime(retryTime);

        SequoiadbDatasource ds = SequoiadbDatasource.builder()
                .serverAddress(address)
                .configOptions(netOpt)
                .build();

        long startTime = System.currentTimeMillis();
        try {
            ds.getConnection(timeout);
        } catch (BaseException e) {
            Assert.assertEquals(e.getMessage(), errorCode, e.getErrorCode());
        } catch (InterruptedException e) {
            // ignore
        } finally {
            long endTime = System.currentTimeMillis();
            long time = (endTime - startTime);

            ds.close();

            System.out.println("timeout: " + timeout + "  actual: " + time);
            if (timeout > 0 && (time > (timeout + deviationTime))) {
                Assert.fail("Inaccurate timeout, except time: " + timeout + " actual time: " + time);
            }
        }
    }
}
