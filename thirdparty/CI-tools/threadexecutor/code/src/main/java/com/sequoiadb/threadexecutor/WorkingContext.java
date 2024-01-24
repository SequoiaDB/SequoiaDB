package com.sequoiadb.threadexecutor;

import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import com.sequoiadb.threadexecutor.annotation.parser.MethodInfo;
import com.sequoiadb.threadexecutor.exception.SchException;

class WorkingContext {
    private Lock lock = new ReentrantLock();

    private Set<MethodInfo> expectFinishMethods = new HashSet<>();
    private Set<MethodInfo> expectBlockingMethods = new HashSet<>();
    private Set<Worker> notifyWorkers = new HashSet<>();
    private int step;
    private ThreadExecutor es;

    WorkingContext(ThreadExecutor es, int step) {
        this.es = es;
        this.step = step;
    }

    public synchronized void addExpectFinishMethod(MethodInfo m) {
        lock.lock();
        try {
            expectFinishMethods.add(m);
        }
        finally {
            lock.unlock();
        }
    }

    public Set<MethodInfo> getExpectFinishMethod() {
        lock.lock();
        try {
            Set<MethodInfo> tmp = new HashSet<>();
            tmp.addAll(expectFinishMethods);
            return tmp;
        }
        finally {
            lock.unlock();
        }
    }

    public void addNeedNotifyWorker(Worker w) {
        notifyWorkers.add(w);
    }

    public Set<Worker> getNeedNotifyWorkers() {
        return notifyWorkers;
    }

    public void addExpectBlockingMethd(MethodInfo m) {
        expectBlockingMethods.add(m);
    }

    public Set<MethodInfo> getExpectBlockingMethd() {
        return expectBlockingMethods;
    }

    public void finishStep(int step, MethodInfo m) throws SchException {
        lock.lock();
        try {
            if (this.step != step) {
                throw new SchException("method should be stop in step:" + step
                        + ",while stop in step:" + this.step + ",method=" + m);
            }

            if (!expectFinishMethods.contains(m)) {
                throw new SchException("running step:" + this.step + " method is mismatch=" + m);
            }

            expectFinishMethods.remove(m);

            if (expectFinishMethods.size() == 0) {
                es.notifySchedule();
            }
        }
        finally {
            lock.unlock();
        }
    }
}
