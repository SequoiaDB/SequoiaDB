package com.sequoiadb.threadexecutor;

class WorkStatus {
    private int step = 0;
    private EN_WORK_STATUS status = EN_WORK_STATUS.INIT;

    public WorkStatus() {
    }

    public int getStep() {
        return step;
    }

    public EN_WORK_STATUS getStatus() {
        return status;
    }

    public void setStep(int step) {
        this.step = step;
    }

    public void setStatus(EN_WORK_STATUS status) {
        this.status = status;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Step:").append(step).append(",");
        sb.append("Status:").append(status);

        return sb.toString();
    }
}
