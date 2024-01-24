package com.sequoiadb.threadexecutor.annotation;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

@Retention(RetentionPolicy.RUNTIME)
@Target({ ElementType.METHOD })
public @interface ExpectBlock {
    public boolean expectBlock() default true;

    // 等待一定时间，认为该函数的确被阻塞。提前退出会抛异常
    public int confirmTime();

    // 明确到第几步之后才会解除阻塞。提前退出，或到达指定步骤后仍未解除阻塞会抛异常
    public int contOnStep();
}
