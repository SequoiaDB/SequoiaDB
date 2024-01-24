package com.sequoiadb.threadexecutor;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sequoiadb.threadexecutor.annotation.parser.MethodInfo;
import com.sequoiadb.threadexecutor.annotation.parser.ObjectInfo;
import com.sequoiadb.threadexecutor.exception.SchException;

class Worker extends Thread {
    private static final Logger logger = LoggerFactory.getLogger(Worker.class);

    private boolean runningFlag = true;
    private ThreadExecutor es;
    private Object o;
    private List<MethodInfo> methodList = new ArrayList<>();
    private Method errHandler;

    private WorkStatus status = new WorkStatus();
    private MethodInfo workingMethod;

    private WorkerErrMsg error;

    public Worker(ThreadExecutor es, Object o, ObjectInfo item) {
        this.es = es;
        this.o = o;
        methodList = item.getMethodList();
        errHandler = item.getErrHandler();
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Worker:").append(this.getName()).append(",");
        sb.append("Status:").append(status).append(",");
        sb.append("Methods:[");
        for (int i = 0; i < methodList.size(); i++) {
            MethodInfo m = methodList.get(i);
            sb.append("{");
            sb.append(m);
            sb.append("}");
            if (i != methodList.size() - 1) {
                sb.append(",");
            }
        }
        sb.append("]");

        return sb.toString();
    }

    List<MethodInfo> getMethods() {
        return methodList;
    }

    Method getErrHandler() {
        return errHandler;
    }

    private void readyForSchedule() throws SchException {
        es.finishStep(0, new MethodInfo(o, 0, null, ""));
    }

    private void waitForMyStep(MethodInfo sm) throws SchException {
        try {
            synchronized (this) {
                while (es.getRunningStep() != sm.getStep()) {
                    if (!runningFlag()) {
                        return;
                    }

                    this.wait(2000);
                }
            }

            if (es.getRunningStep() != sm.getStep()) {
                throw new SchException("step is mismatch:main step:" + es.getRunningStep()
                        + ",myStep:" + sm.getStep());
            }
        }
        catch (Exception e) {
            throw new SchException("wait for step failed:step=" + sm.getStep(), e);
        }
    }

    private void executeMethod(Object o, Method m) throws SchException {
        try {
            m.setAccessible(true);
            m.invoke(o);
        }
        catch (Exception e) {
            throw new SchException("invoke " + o + "." + m.getName() + " failed", e);
        }
    }

    private void errHandler() throws SchException {
        if (null != errHandler) {
            executeMethod(o, errHandler);
        }
    }

    private void updateStatus(int step, EN_WORK_STATUS s) {
        this.status.setStep(step);
        this.status.setStatus(s);
    }

    public WorkStatus getWorkStatus() {
        return this.status;
    }

    @Override
    public void run() {
        try {
            innerRun();
        }
        catch (Exception e) {
            logger.error("execute worker failed:worker=\n{}", this, e);
            error = new WorkerErrMsg(this.o, workingMethod, status.getStep(), e);
            es.errorStep(error);
            try {
                errHandler();
            }
            catch (Exception e1) {
                logger.error("errHandler failed", e);
            }
        }
    }

    public void setRunningFlag(boolean running) {
        runningFlag = running;
    }

    private boolean runningFlag() {
        return runningFlag;
    }

    private void innerRun() throws SchException {
        readyForSchedule();
        status.setStatus(EN_WORK_STATUS.FINISH);

        for (MethodInfo m : methodList) {
            waitForMyStep(m);
            if (!runningFlag()) {
                logger.info("stop while running flag is false");
                break;
            }
            updateStatus(m.getStep(), EN_WORK_STATUS.RUNNING);
            workingMethod = m;
            try {
                executeMethod(o, m.getMethod());
                if (!m.isExpectBlock()) {
                    es.finishStep(m.getStep(), m);
                }
                else {
                    es.finishStep(m.getContStep(), m);
                }
            }
            catch (Exception e) {
                updateStatus(m.getStep(), EN_WORK_STATUS.ERROR);
                logger.error("execute work failed:method={}", m);
                throw e;
            }

            updateStatus(m.getStep(), EN_WORK_STATUS.FINISH);
        }
    }
}
