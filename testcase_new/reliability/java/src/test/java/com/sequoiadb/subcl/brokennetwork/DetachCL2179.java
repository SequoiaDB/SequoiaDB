package com.sequoiadb.subcl.brokennetwork;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.commlib.CommLib;
import com.sequoiadb.commlib.GroupMgr;
import com.sequoiadb.commlib.GroupWrapper;
import com.sequoiadb.commlib.SdbTestBase;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;
import com.sequoiadb.fault.BrokenNetwork;
import com.sequoiadb.subcl.brokennetwork.commlib.DetachCLTask;
import com.sequoiadb.subcl.brokennetwork.commlib.Utils;
import com.sequoiadb.task.FaultMakeTask;
import com.sequoiadb.task.TaskMgr;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * @FileName seqDB-2179: detachCL过程中dataRG主节点连续降备
 * @Author linsuqiang
 * @Date 2017-03-14
 * @Version 1.00
 */

/*
 * 1、创建主表和子表
 * 2、批量执行db.collectionspace.collection.detachCL()分离多个子表
 * 3、子表分离过程中将cl所在dataRG主节点网络断掉（如：使用cutnet.sh工具，命令格式为nohup ./cutnet.sh &），检查detachCL执行结果
 * 4、dataRG重新选主后立即将新主的网络断掉
 * 5、重复步骤4两道三遍
 * 6、将dataRG主节点网络恢复，并对分离成功的子表（普通表）做基本操作（如insert)
 */

public class DetachCL2179 extends SdbTestBase {
    private GroupMgr groupMgr = null;
    private boolean runSuccess = false;
    private String mclName = "cl_2179";
    private String clGroup = null;

    @BeforeClass
    public void setUp() {
        Sequoiadb db = null;
        try {
            System.out.println("the TestCase Name:" + this.getClass().getName() + ". the TestCase begin at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));

            groupMgr = GroupMgr.getInstance();
            if (!groupMgr.checkBusiness()) {
                throw new SkipException("checkBusiness failed");
            }

            db = new Sequoiadb(coordUrl, "", "");
            clGroup = groupMgr.getAllDataGroupName().get(0);
            Utils.createMclAndScl(db, mclName, clGroup);
            Utils.attachAllScl(db, mclName);
        } catch (ReliabilityException e) {
            Assert.fail(this.getClass().getName() + " setUp error, error description:" + e.getMessage() + "\r\n"
                    + Utils.getKeyStack(e, this));
        } finally {
            if (db != null) {
                db.close();
            }
        }
    }

    @Test
    public void test() {
        Sequoiadb db = null;
        try {
            GroupWrapper dataGroup = groupMgr.getGroupByName(clGroup);
            String dataPriHost = dataGroup.getMaster().hostName();

            FaultMakeTask faultTask = BrokenNetwork.getFaultMakeTask(dataGroup, 3, 10);
            TaskMgr mgr = new TaskMgr(faultTask);
            String safeUrl = CommLib.getSafeCoordUrl(dataPriHost);
            DetachCLTask dTask = new DetachCLTask(mclName, safeUrl);
            mgr.addTask(dTask);
            mgr.execute();
            Assert.assertEquals(mgr.isAllSuccess(), true, mgr.getErrorMsg());

            if (!groupMgr.checkBusinessWithLSN(600)) {
                Assert.fail("checkBusinessWithLSN() occurs timeout");
            }

            GroupWrapper cataGroup = groupMgr.getGroupByName("SYSCatalogGroup");
            Utils.checkConsistency(groupMgr);
            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            Utils.checkIntegrated(db, mclName);
            Utils.checkDetached(db, mclName, dTask.getDetachedSclCnt());
            runSuccess = true;
        } catch (ReliabilityException e) {
            e.printStackTrace();
            Assert.fail(e.getMessage());
        } finally {
            if (db != null) {
                db.close();
            }
        }
    }

    @AfterClass
    public void tearDown() {
        if (!runSuccess) {
            throw new SkipException("to save environment");
        }
        Sequoiadb db = null;
        try {
            db = new Sequoiadb(SdbTestBase.coordUrl, "", "");
            Utils.dropMclAndScl(db, mclName);
        } catch (BaseException e) {
            Assert.fail(e.getMessage() + "\r\n" + Utils.getKeyStack(e, this));
        } finally {
            if (db != null) {
                db.close();
            }
            System.out.println("the TestCase Name:" + this.getClass().getName() + ". the TestCase end at:"
                    + new SimpleDateFormat("YYYY-MM-dd HH:mm:ss.SSS").format(new Date()));
        }
    }
}