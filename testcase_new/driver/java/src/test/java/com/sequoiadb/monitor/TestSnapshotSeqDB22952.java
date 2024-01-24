package com.sequoiadb.monitor;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;


/**
 * @Descreption seqDB-22952:新增Snapshot 接口，SDB_SNAP_QUERIES、SDB_SNAP_LATCHWAITS、SDB_SNAP_LOCKWAITS
 * @Author Yipan
 * @Date 2020/12/2 14:12
 */
public class TestSnapshotSeqDB22952 extends SdbTestBase {
    private Sequoiadb sdb = null;

    @BeforeClass
    public void setSdb() {
        // 建立 SequoiaDB 数据库连接
        sdb = new Sequoiadb(SdbTestBase.coordUrl, "", "");
        if (CommLib.isStandAlone(sdb)) {
            throw new SkipException("skip standalone");
        }
    }

    @Test
    public void test() {
        // 获取快照类型检查
        sdb.getSnapshot(Sequoiadb.SDB_SNAP_QUERIES, "", null, null);
        Assert.assertEquals(Sequoiadb.SDB_SNAP_QUERIES, 18);
        sdb.getSnapshot(Sequoiadb.SDB_SNAP_LATCHWAITS, "", null, null);
        Assert.assertEquals(Sequoiadb.SDB_SNAP_LATCHWAITS, 19);
        sdb.getSnapshot(Sequoiadb.SDB_SNAP_LOCKWAITS, "", null, null);
        Assert.assertEquals(Sequoiadb.SDB_SNAP_LOCKWAITS, 20);
    }

    @AfterClass
    public void closeSdb() {
        sdb.close();
    }

}

