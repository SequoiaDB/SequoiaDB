package com.sequoias3.config;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

@Component
@ConfigurationProperties(prefix = "sdbs3.context")
public class ContextConfig {
    private int lifecycle;
    private String cron;

    public void setLifecycle(int lifecycle) {
        this.lifecycle = lifecycle;
    }

    public int getLifecycle() {
        return lifecycle;
    }

    public void setCron(String cron) {
        this.cron = cron;
    }

    public String getCron() {
        return cron;
    }
}
