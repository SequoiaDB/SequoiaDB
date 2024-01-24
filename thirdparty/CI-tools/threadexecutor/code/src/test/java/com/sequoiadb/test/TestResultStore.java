package com.sequoiadb.test;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.testng.annotations.Test;

import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

class TestResultCollector1 extends ResultStore {
    private static final Logger logger = LoggerFactory.getLogger(TestResultCollector1.class);

    @ExecuteOrder(step = 1)
    public void initData() {
        try {
            throw new Exception("error1");
        }
        catch (Exception e) {
            saveResult(-1, e);
        }
    }
}

class TestResultCollector2 extends ResultStore {
    private static final Logger logger = LoggerFactory.getLogger(TestResultCollector2.class);

    @ExecuteOrder(step = 1)
    public void remove() throws InterruptedException {
        try {
            throw new Exception("error2");
        }
        catch (Exception e) {
            saveResult(-2, e);
        }
    }
}

public class TestResultStore {
    private static final Logger logger = LoggerFactory.getLogger(TestResultStore.class);

    @Test
    public void test() throws Exception {
        ThreadExecutor es = new ThreadExecutor();
        TestResultCollector1 t1 = new TestResultCollector1();
        TestResultCollector2 t2 = new TestResultCollector2();
        es.addWorker(t1);
        es.addWorker(t2);

        es.run();

        logger.info("t1 result:code={}, e={}", t1.getRetCode(), t1.getThrowable().getMessage());
        logger.info("t2 result:code={}, e={}", t2.getRetCode(), t2.getThrowable().getMessage());
    }
}
