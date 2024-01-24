package com.sequoiadb.threadexecutor.annotation.parser;

import java.lang.reflect.Method;

public class MethodInfo {
    private static final int DEFAULT_BLOCK_CONFIRM_TIME = 10;
    private Method m;

    private Object o;
    private int step;
    private String desc;

    private int contStep;
    private boolean isExpectBlock = false;
    private int blockConfirmTime = DEFAULT_BLOCK_CONFIRM_TIME;

    public MethodInfo(Object o, int step, Method m, String desc) {
        this.o = o;
        this.step = step;
        this.m = m;
        this.desc = desc;
    }

    public int getStep() {
        return step;
    }

    public Method getMethod() {
        return m;
    }

    public Object getObject() {
        return o;
    }

    public String getDesc() {
        return desc;
    }

    public void enableExpectBlock(int confirmTime) {
        if (confirmTime <= 0) {
            confirmTime = DEFAULT_BLOCK_CONFIRM_TIME;
        }

        isExpectBlock = true;
        blockConfirmTime = confirmTime;
    }

    public void disableExpectBlock() {
        isExpectBlock = false;
    }

    public boolean isExpectBlock() {
        return isExpectBlock;
    }

    public int getBlockConfirmTime() {
        return blockConfirmTime;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Object:").append(o).append(",");
        sb.append("Method:").append(m.getName()).append(",");
        sb.append("Desc:").append(desc).append(",");
        sb.append("Step:").append(step);
        if (isExpectBlock) {
            sb.append(",");
            sb.append("ExpectBlock:true").append(",");
            sb.append("BlockConfirmTime:").append(blockConfirmTime).append(",");
            sb.append("contStep:").append(contStep);
        }
        return sb.toString();
    }

    public String getSimpleInfo() {
        StringBuilder sb = new StringBuilder();
        sb.append(o.getClass().getSimpleName()).append(".").append(m.getName());
        sb.append(",").append(desc);
        if (isExpectBlock) {
            sb.append(",");
            sb.append("ExpectBlock:true").append(",");
            sb.append("BlockConfirmTime:").append(blockConfirmTime).append(",");
            sb.append("contStep:").append(contStep);
        }

        return sb.toString();
    }

    public void setContStep(int contStep) {
        this.contStep = contStep;
    }

    public int getContStep() {
        return contStep;
    }

    @Override
    public int hashCode() {
        return o.hashCode() + step;
    }

    @Override
    public boolean equals(Object r) {
        if (r == null || !(r instanceof MethodInfo)) {
            return false;
        }

        MethodInfo m = (MethodInfo) r;
        return this.o == m.o && this.step == m.step && this.m == m.m;
    }
}
