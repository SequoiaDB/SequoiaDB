package com.sequoiadb.threadexecutor.annotation.parser;

import java.lang.reflect.Method;

import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;
import com.sequoiadb.threadexecutor.annotation.ExecutorErrHandler;
import com.sequoiadb.threadexecutor.annotation.ExpectBlock;
import com.sequoiadb.threadexecutor.exception.SchException;

public class AnnotationParser {
    public static ObjectInfo parse(Object o) throws SchException {
        ObjectInfo objectInfo = new ObjectInfo();
        Class<?> c = o.getClass();
        Method[] methods = c.getDeclaredMethods();
        for (Method m : methods) {
            if (m.isAnnotationPresent(ExecuteOrder.class)) {
                ExecuteOrder order = m.getAnnotation(ExecuteOrder.class);
                if (order.step() == 0) {
                    throw new SchException(c + "." + m + "'s ExecutorStep can't be 0");
                }

                MethodInfo methodInfo = new MethodInfo(o, order.step(), m, order.desc());

                if (m.isAnnotationPresent(ExpectBlock.class)) {
                    ExpectBlock eb = m.getAnnotation(ExpectBlock.class);
                    if (eb.expectBlock()) {
                        methodInfo.enableExpectBlock(eb.confirmTime());
                        if (eb.contOnStep() <= order.step()) {
                            throw new SchException(c + "." + m + " ExpectBlock's contOnStep:"
                                    + eb.contOnStep() + "must greater than step:" + order.step());
                        }
                        methodInfo.setContStep(eb.contOnStep());
                    }
                }

                objectInfo.addMethod(methodInfo);
            }

            if (m.isAnnotationPresent(ExecutorErrHandler.class)) {
                objectInfo.setErrHandler(m);
            }
        }

        objectInfo.organize();

        return objectInfo;
    }
}
