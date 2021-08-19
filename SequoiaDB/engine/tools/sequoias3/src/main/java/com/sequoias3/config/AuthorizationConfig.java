package com.sequoias3.config;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

@Component
@ConfigurationProperties(prefix = "sdbs3.authorization")
public class AuthorizationConfig {
    private boolean check = true;

    public void setCheck(Boolean check) {
        this.check = check;
    }

    public boolean getCheck() {
        return check;
    }

    public boolean isCheck(){
        return check;
    }
}
