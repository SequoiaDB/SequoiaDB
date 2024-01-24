/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:TaskMgr.java
 *
 * @author wenjingwang Date:2017-2-21下午4:54:48
 * @version 1.00
 */
package com.sequoiadb.task;

import com.sequoiadb.exception.ReliabilityException;

import java.util.ArrayList;
import java.util.List;
import java.util.logging.Logger;

public class TaskMgr {
    private final static Logger log = Logger
            .getLogger( TaskMgr.class.getName() );

    private List< Task > taskList = new ArrayList<>( 10 );

    public TaskMgr( FaultMakeTask faultMakeTask ) {
        taskList.add( faultMakeTask );
    }

    public TaskMgr() {
    }

    /**
     * add by jt
     *
     * @param faultMakeTask
     * @param operateTasks
     */
    public TaskMgr( FaultMakeTask faultMakeTask, OperateTask... operateTasks ) {
        this( faultMakeTask );
        for ( OperateTask task : operateTasks ) {
            addTask( task );
        }
    }

    /**
     * add by jt.
     *
     * @param faultMakeTask
     * @return
     */

    public static TaskMgr getTaskMgr( FaultMakeTask faultMakeTask,
            OperateTask... operateTasks ) {
        TaskMgr taskMgr = new TaskMgr( faultMakeTask );
        for ( OperateTask task : operateTasks ) {
            taskMgr.addTask( task );
        }
        return taskMgr;
    }

    @Deprecated
    public void addTask( String taskClassName ) {
        OperateTask task = OperateTaskFactory.newTask( taskClassName, this );
        if ( task == null ) {
            return;
        }
        taskList.add( task );
    }

    public TaskMgr addTask( Task task ) {
        taskList.add( task );
        return this;
    }

    /**
     * @param task
     *            任务
     */
    public void removeTask( Task task ) {
        taskList.remove( task );
    }

    public void init() throws ReliabilityException {
        for ( Task task : taskList ) {
            task.init();
        }
    }

    public void start() {
        for ( Task task : taskList ) {
            task.start();
        }
    }

    public void join() {
        for ( Task task : taskList ) {
            try {
                task.join();
            } catch ( InterruptedException e ) {
                log.warning( e.getMessage() );
            }
        }
    }

    public void check() throws ReliabilityException {
        if ( !this.isAllSuccess() ) {
            throw new ReliabilityException( this.getErrorMsg() );
        }
        for ( Task task : taskList ) {
            task.check();
        }
    }

    public void fini() throws ReliabilityException {
        for ( Task task : taskList ) {
            task.fini();
        }
    }

    /**
     * @param name
     *            任务名
     * @return 返回对应任务名的任务，不存在返回null,如果存在多个只返回第一个
     */
    public Task getTaskByName( String name ) {
        for ( Task task : taskList ) {
            if ( task.getName().equals( name ) )
                return task;
        }
        return null;
    }

    public List< Exception > getExceptions() {
        List< Exception > list = new ArrayList<>();
        for ( Task task : taskList ) {
            if ( task.getException() != null )
                list.add( task.getException() );
        }
        return list;
    }

    public boolean isAllSuccess() {
        return getExceptions().isEmpty();
    }

    public String getErrorMsg() {
        StringBuffer reStr = new StringBuffer();
        for ( Task task : taskList ) {
            reStr.append( task.getErrorMsg() );
        }
        return reStr.toString();
    }

    public void clear() {
        taskList.clear();
    }

    /**
     * 顺序调用init(),start(),join(),fini()
     *
     * @throws ReliabilityException
     */
    public void execute() throws ReliabilityException {
        init();
        start();
        join();
        fini();
    }
}
