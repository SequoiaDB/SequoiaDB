package com.sequoiadb.threadexecutor;

import java.util.ArrayList;
import java.util.List;

import com.sequoiadb.threadexecutor.annotation.parser.MethodInfo;

class MethodGroup {
    private List<MethodInfo> methodList = new ArrayList<>();

    public void add(MethodInfo m) {
        methodList.add(m);
    }

    public List<MethodInfo> getMethod() {
        return methodList;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (MethodInfo m : methodList) {
            sb.append(m).append("\n");
        }

        return sb.toString();
    }

    public Object getSimpleInfo() {
        StringBuilder sb = new StringBuilder();
        for (MethodInfo m : methodList) {
            sb.append(m.getSimpleInfo()).append("\n");
        }

        return sb.toString();
    }
}
