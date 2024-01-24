/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:OperateTaskFactory.java 类的详细描述
 *
 * @author wenjingwang Date:2017-2-28上午9:28:37
 * @version 1.00
 */
package com.sequoiadb.task;

public final class OperateTaskFactory {
    private OperateTaskFactory() {

    }

    public static OperateTask newTask( String taskClassName, TaskMgr mgr ) {
        ClassLoader classLoader = OperateTaskFactory.class.getClassLoader();
        OperateTask ret;
        try {
            Class< ? > taskClass = classLoader.loadClass( taskClassName );
            ret = ( OperateTask ) taskClass.newInstance();
            ret.setMgr( mgr );
        } catch ( Exception e ) {
            e.printStackTrace();
            return null;
        }

        return ret;
    }
}
