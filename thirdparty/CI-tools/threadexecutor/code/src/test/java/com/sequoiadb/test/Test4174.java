package com.sequoiadb.test;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.testng.annotations.Test;

import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

class Trans1 {
    private static final Logger logger = LoggerFactory.getLogger(Trans1.class);

    @ExecuteOrder(step = 1)
    public void initData() {
        // logger.info("创建索引，插入记录R1");
    }

    @ExecuteOrder(step = 3)
    public void update() {
        // logger.info("开启事务并更新R1为R2");
    }

    @ExecuteOrder(step = 4)
    public void commit() {
        // logger.info("提交事务");
    }

    @ExecuteOrder(step = 5)
    public void checkResult() {

    }
}

class Trans2 {
    private static final Logger logger = LoggerFactory.getLogger(Trans2.class);

    @ExecuteOrder(step = 1)
    // 预期该函数会阻塞（当超过10s，函数还没有返回，则认为该函数阻塞；提前退出阻塞则认为不符合用例预期）
    // @ExpectBlock(confirmTime = 4, contOnStep = 2)
    public void remove() throws InterruptedException {
        // logger.info("开启事务2，不带条件删除，强制走索引扫描");
    }

    @ExecuteOrder(step = 5)
    public void checkResult() {
        // logger.info("事务2中分别走索引扫描及表扫描查询，检查结果");
    }
}

public class Test4174 {
    private static final Logger logger = LoggerFactory.getLogger(Test4174.class);

    @Test
    public void test() throws Exception {
        for (int i = 0; i < 1; ++i) {
            ThreadExecutor es = new ThreadExecutor();
            es.addWorker(new Trans1());
            es.addWorker(new Trans2());

            logger.info("haha={}", es);

            es.run();
        }
    }
}
