package com.sequoias3.config;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

@Component
@ConfigurationProperties(prefix = "sdbs3.bucket")
public class BucketConfig {
    private int limit;
    private boolean allowreput = false;

    public void setLimit(int limit){
        this.limit = limit;
    }

    public int getLimit(){
        return limit;
    }

    public void setAllowreput(boolean allowreput) {
        this.allowreput = allowreput;
    }

    public boolean getAllowreput() {
        return allowreput;
    }
}
