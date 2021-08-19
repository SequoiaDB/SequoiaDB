package com.sequoias3.core;

public class TaskTable {
    public static final String TASK_TYPE  = "TaskType";
    public static final String TASK_ID    = "TaskId";

    public static final String TASK_INDEX = "taskIndex";

    public static final int TASK_TYPE_DELIMITER     = 1;
    public static final int TASK_COMPLETE_UPLOAD    = 2;

    private int taskType;
    private long taskId;

    public void setTaskId(long taskId) {
        this.taskId = taskId;
    }

    public long getTaskId() {
        return taskId;
    }

    public void setTaskType(int taskType) {
        this.taskType = taskType;
    }

    public int getTaskType() {
        return taskType;
    }
}
