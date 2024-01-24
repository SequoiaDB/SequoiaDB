package com.sequoias3.context;

import com.sequoias3.config.ContextConfig;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.Trigger;
import org.springframework.scheduling.TriggerContext;
import org.springframework.scheduling.annotation.EnableScheduling;
import org.springframework.scheduling.annotation.SchedulingConfigurer;
import org.springframework.scheduling.config.ScheduledTaskRegistrar;
import org.springframework.scheduling.support.CronTrigger;
import org.springframework.stereotype.Component;

import java.util.Date;

@Component
@EnableScheduling
public class ContextScheduledService  implements SchedulingConfigurer {

    @Autowired
    ContextManager contextManager;

    @Autowired
    ContextConfig contextConfig;

    @Override
    public void configureTasks(ScheduledTaskRegistrar taskRegistrar){
        taskRegistrar.addTriggerTask(new Runnable() {
            @Override
            public void run() {
                contextManager.cleanExpiredContext();
            }
        }, new Trigger() {
            @Override
            public Date nextExecutionTime(TriggerContext triggerContext) {
                return new CronTrigger(contextConfig.getCron()).nextExecutionTime(triggerContext);
            }
        });
    }
}
