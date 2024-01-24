package com.sequoias3.config;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

@Component
@ConfigurationProperties(prefix = "sdbs3.multipartupload")
public class MultiPartUploadConfig {
    private boolean partlistinuse = true;
    private boolean partsizelimit = true;
    private int     incompletelifecycle = 3;
    private int     completereservetime = 24 * 60;

    public void setPartlistinuse(boolean partlistinuse) {
        this.partlistinuse = partlistinuse;
    }

    public boolean isPartlistinuse() {
        return partlistinuse;
    }

    public void setPartsizelimit(boolean partsizelimit) {
        this.partsizelimit = partsizelimit;
    }

    public boolean isPartSizeLimit(){
        return this.partsizelimit;
    }

    public void setIncompletelifecycle(int incompletelifecycle) {
        this.incompletelifecycle = incompletelifecycle;
    }

    public int getIncompletelifecycle() {
        return incompletelifecycle;
    }

    public void setCompletereservetime(int completereservetime) {
        this.completereservetime = completereservetime;
    }

    public int getCompletereservetime() {
        return completereservetime;
    }
}
