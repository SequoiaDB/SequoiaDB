package com.sequoias3;

import org.springframework.boot.autoconfigure.batch.BatchProperties;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.scheduling.annotation.SchedulingConfigurer;
import org.springframework.scheduling.concurrent.ThreadPoolTaskScheduler;
import org.springframework.scheduling.config.ScheduledTaskRegistrar;

import java.lang.reflect.Method;

@Configuration
@EnableScheduling
public class ScheduleConfig implements SchedulingConfigurer {
    @Override
    public void configureTasks(ScheduledTaskRegistrar scheduledTaskRegistrar) {
        scheduledTaskRegistrar.setTaskScheduler(myThreadPoolScheduler());
    }

    @Bean
    public ThreadPoolTaskScheduler myThreadPoolScheduler(){
        ThreadPoolTaskScheduler taskScheduler = new ThreadPoolTaskScheduler();
        taskScheduler.setPoolSize(getPoolSize());
        taskScheduler.setWaitForTasksToCompleteOnShutdown(true);
        taskScheduler.setAwaitTerminationSeconds(10);
        return taskScheduler;
   }

    private int getPoolSize(){
        Method[] methods = BatchProperties.Job.class.getMethods();
        int scheduleSize = 5;
        int newScheduleSize = 0;
        if (methods != null && methods.length > 0){
            for (Method method : methods) {
                Scheduled annotation = method.getAnnotation(Scheduled.class);
                if (annotation != null){
                    newScheduleSize++;
                }
            }
        }
        if (scheduleSize > newScheduleSize){
            newScheduleSize = scheduleSize;
        }
        return newScheduleSize;
    }
}
