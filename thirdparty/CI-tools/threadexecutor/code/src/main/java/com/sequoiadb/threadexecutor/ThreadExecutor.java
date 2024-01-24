package com.sequoiadb.threadexecutor;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.atomic.AtomicInteger;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.sequoiadb.threadexecutor.annotation.parser.AnnotationParser;
import com.sequoiadb.threadexecutor.annotation.parser.MethodInfo;
import com.sequoiadb.threadexecutor.annotation.parser.ObjectInfo;
import com.sequoiadb.threadexecutor.exception.SchException;

public class ThreadExecutor {
    private static final Logger logger = LoggerFactory.getLogger(ThreadExecutor.class);

    private static final long PERIOD_WAIT_TIME = 1000;
    private static final long DEFAULT_MAXSTEPRUNTIME = 120000;

    // use step for map key
    private Map<Integer, MethodGroup> stepMap = new HashMap<>();
    // use contStep for map key
    private Map<Integer, MethodGroup> blockFinishStepMap = new HashMap<>();
    private Map<Object, Worker> object2Worker = new HashMap<>();
    private static AtomicInteger runningStep = new AtomicInteger(0);

    private WorkingContext context;
    private long maxStepRunTime;

    private ScheduledExecutorService executeService;

    private WorkerErrMsg workErr;

    public ThreadExecutor() {
        this(DEFAULT_MAXSTEPRUNTIME);
    }

    public ThreadExecutor(long maxStepRunTime) {
        this.maxStepRunTime = maxStepRunTime;
    }

    public void addWorker(Object o) throws SchException {
        ObjectInfo oai = AnnotationParser.parse(o);
        List<MethodInfo> methodList = oai.getMethodList();
        addToStepMap(o, methodList);
        Worker work = new Worker(this, o, oai);
        object2Worker.put(o, work);
    }

    private WorkingContext createStepContext(int step) {
        WorkingContext context = new WorkingContext(this, step);
        if (0 == step) {
            for (Object o : object2Worker.keySet()) {
                context.addExpectFinishMethod(new MethodInfo(o, 0, null, ""));
            }
        } else {
            MethodGroup omg = stepMap.get(step);
            for (MethodInfo m : omg.getMethod()) {
                Object o = m.getObject();
                context.addNeedNotifyWorker(object2Worker.get(o));
                if (!m.isExpectBlock()) {
                    context.addExpectFinishMethod(m);
                } else {
                    context.addExpectBlockingMethd(m);
                }
            }

            omg = blockFinishStepMap.get(step);
            if (null != omg) {
                for (MethodInfo m : omg.getMethod()) {
                    context.addExpectFinishMethod(m);
                }
            }
        }

        return context;
    }

    public void finishStep(int step, MethodInfo o) throws SchException {
        context.finishStep(step, o);
    }

    private void addToBlockStepMap(Object o, MethodInfo m) {
        // add to block map, but use contStep for map key
        MethodGroup blockMethodGroup = blockFinishStepMap.get(m.getContStep());
        if (blockMethodGroup == null) {
            blockMethodGroup = new MethodGroup();
            blockFinishStepMap.put(m.getContStep(), blockMethodGroup);
        }

        blockMethodGroup.add(m);
    }

    private void addToStepMap(Object o, List<MethodInfo> methodList) {
        for (MethodInfo m : methodList) {
            MethodGroup omg = stepMap.get(m.getStep());
            if (omg == null) {
                omg = new MethodGroup();
                stepMap.put(m.getStep(), omg);
            }

            omg.add(m);

            if (m.isExpectBlock()) {
                addToBlockStepMap(o, m);
            }
        }
    }

    private long waitWorkers(long waitTime) throws SchException {
        long actualWaitTime = 0;
        long times = waitTime / PERIOD_WAIT_TIME;
        if (times == 0) {
            times = 1;
        }

        Set<MethodInfo> methods = null;
        try {
            for (int i = 0; i < times; i++) {
                synchronized (this) {
                    this.wait(PERIOD_WAIT_TIME);
                }

                WorkerErrMsg errMsg = getError();
                if (null != errMsg) {
                    logger.error("execute failed:step={},object={},method={}", errMsg.getStep(),
                            errMsg.getObj(), errMsg.getMethodInfo());
                    throw errMsg.getException();
                }

                methods = this.context.getExpectFinishMethod();
                if (methods.size() == 0) {
                    return actualWaitTime;
                }

                actualWaitTime += PERIOD_WAIT_TIME;
            }

            MethodInfo tmpMethod = null;
            for (MethodInfo m : methods) {
                tmpMethod = m;
                logger.error("method is not finish:m={}", m);
            }

            throw new SchException("wait worker timeout in step " + getRunningStep() + ":method="
                    + tmpMethod + ",timeout=" + waitTime);
        } catch (SchException e) {
            throw e;
        } catch (Exception e) {
            throw new SchException("wait failed:", e);
        }
    }

    public void notifySchedule() throws SchException {
        try {
            synchronized (this) {
                this.notify();
            }
        } catch (Exception e) {
            throw new SchException("wait failed:", e);
        }
    }

    public void display() throws Exception {
        checkValid();
        for (Entry<Integer, MethodGroup> entry : stepMap.entrySet()) {
            MethodGroup mg = entry.getValue();
            logger.info("step {}\n{}", entry.getKey(), mg.getSimpleInfo());
        }
    }

    public void run() throws Exception {
        checkValid();
        if (null == executeService) {
            executeService = Executors.newScheduledThreadPool(object2Worker.size());
        }

        try {
            context = createStepContext(0);

            for (Worker w : object2Worker.values()) {
                executeService.execute(w);
            }

            logger.info("start to wait for all threads ready...");
            waitWorkers(maxStepRunTime);

            logger.info("stat to running steps...");
            for (Entry<Integer, MethodGroup> entry : stepMap.entrySet()) {
                context = createStepContext(entry.getKey());
                setRunningStep(entry.getKey());
                logger.info("step {}", entry.getKey());
                if (logger.isDebugEnabled()) {
                    String info = getContextInfo(context);
                    logger.debug("\n{}", info);
                }

                notifyAllRelateWorkers(context.getNeedNotifyWorkers());
                long waitWorkTime = waitWorkers(maxStepRunTime);

                Set<MethodInfo> methods = context.getExpectBlockingMethd();
                int maxTime = 0;
                for (MethodInfo m : methods) {
                    if (maxTime < m.getBlockConfirmTime()) {
                        maxTime = m.getBlockConfirmTime();
                    }
                }

                long confirmTime = maxTime * 1000;
                if (confirmTime > waitWorkTime) {
                    logger.info("sleep {}ms to confirm block methods", confirmTime - waitWorkTime);
                    Thread.sleep(confirmTime - waitWorkTime);
                }

                checkAllBlockingMethodIsRunning(entry.getKey(), methods);
            }
        } catch (Throwable t) {
            notifyAllWorkerQuit();
            logger.error("run schedule failed", t);
            throw t;
        } finally {
            logger.info("shutting down all threads");
            executeService.shutdown();
            // wait all threads is really quit
            while (!executeService.isTerminated()) {
                try {
                    Thread.sleep(1000);
                } catch (Exception e) {
                }
            }

            logger.info("all threads are shutted down");
        }
    }

    private void notifyAllWorkerQuit() throws SchException {
        try {
            object2Worker.values();
            Set<Worker> workers = new HashSet<>();
            for (Worker w : object2Worker.values()) {
                w.setRunningFlag(false);
                workers.add(w);
            }

            // wait up all threads, let them have opportunity to check the
            // running flag
            notifyAllRelateWorkers(workers);
        } catch (Exception e) {
            logger.warn("notify all workers failed", e);
        }
    }

    private String getContextInfo(WorkingContext context) {
        StringBuilder sb = new StringBuilder();
        sb.append("ExpectFinishMethods:").append("\n");
        Set<MethodInfo> methods = context.getExpectFinishMethod();
        for (MethodInfo m : methods) {
            sb.append("\t").append(m).append("\n");
        }

        sb.append("ExpectBlockingMethds:").append("\n");
        methods = context.getExpectBlockingMethd();
        for (MethodInfo m : methods) {
            sb.append("\t").append(m).append("\n");
        }

        return sb.toString();
    }

    private void checkAllBlockingMethodIsRunning(int step, Set<MethodInfo> methods)
            throws SchException {
        for (MethodInfo mi : methods) {
            Worker w = object2Worker.get(mi.getObject());
            if (w.getWorkStatus().getStatus() == EN_WORK_STATUS.RUNNING
                    && w.getWorkStatus().getStep() == step) {
                return;
            }

            throw new SchException("worker is not running in step=" + step + ",status="
                    + w.getWorkStatus() + ",method=" + mi);
        }
    }

    private void checkValid() throws SchException {
        List<MethodInfo> blockAbleMethods = new ArrayList<>();
        for (MethodGroup omg : stepMap.values()) {
            MethodInfo lastBlockMethod = null;
            for (MethodInfo m : omg.getMethod()) {
                if (m.isExpectBlock()) {
                    blockAbleMethods.add(m);
                    if (null == lastBlockMethod) {
                        lastBlockMethod = m;
                    } else {
                        throw new SchException(
                                "could not exist more one blocking method in same object "
                                        + "and step:method1=" + m.getSimpleInfo() + ",method2="
                                        + lastBlockMethod.getSimpleInfo());
                    }
                }
            }
        }

        MethodInfo lastErrMethod = null;
        for (MethodInfo m : blockAbleMethods) {
            if (!stepMap.containsKey(m.getContStep())) {
                lastErrMethod = m;
                logger.error(
                        "step{} is missing, while blocking method should be continued:method={}",
                        m.getContStep(), m.getSimpleInfo());
            }
        }

        if (null != lastErrMethod) {
            StringBuilder sb = new StringBuilder();
            sb.append("step").append(lastErrMethod.getContStep());
            sb.append(" is missing, while blocking method should be continued:method=");
            sb.append(lastErrMethod.getSimpleInfo());
            throw new SchException(sb.toString());
        }
    }

    private void notifyWorker(Worker w) {
        synchronized (w) {
            w.notify();
        }
    }

    private void notifyAllRelateWorkers(Set<Worker> workers) throws SchException {
        for (Worker w : workers) {
            notifyWorker(w);
        }
    }

    public int getRunningStep() {
        return runningStep.get();
    }

    private void setRunningStep(int step) {
        this.runningStep.set(step);
    }

    public synchronized void errorStep(WorkerErrMsg error) {
        this.workErr = error;
    }

    public synchronized WorkerErrMsg getError() {
        return workErr;
    }
}
