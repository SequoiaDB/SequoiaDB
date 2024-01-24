package com.sequoiadb.threadexecutor.annotation.parser;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class ObjectInfo {
    List<MethodInfo> methodList = new ArrayList<>();
    Method errHandler;

    void setErrHandler(Method m) {
        errHandler = m;
    }

    void addMethod(MethodInfo methodInfo) {
        methodList.add(methodInfo);
    }

    public List<MethodInfo> getMethodList() {
        return methodList;
    }

    public Method getErrHandler() {
        return errHandler;
    }

    void organize() {
        Collections.sort(methodList, new Comparator<MethodInfo>() {

            @Override
            public int compare(MethodInfo o1, MethodInfo o2) {
                if (o1.getStep() > o2.getStep()) {
                    return 1;
                }
                else if (o1.getStep() < o2.getStep()) {
                    return -1;
                }
                return 0;
            }
        });
    }
}
