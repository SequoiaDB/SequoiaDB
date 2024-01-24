package com.sequoias3.config;

import java.io.IOException;

public class SequoiadbDataConfig {
    String csName;
    String domain;
    int csRange;
    int lobPageSize;
    int replSize;

    public String getCsName() {
        return csName;
    }

    public void setCsName(String csName) {
        this.csName = csName;
    }

    public void setDomain(String domain) {
        this.domain = domain;
    }

    public String getDomain() {
        return domain;
    }

    public void setCsRange(int csRange) {
        this.csRange = csRange;
    }

    public int getCsRange() {
        return csRange;
    }

    public int getLobPageSize() {
        return lobPageSize;
    }

    public void setLobPageSize(int lobPageSize) throws IOException {
        this.lobPageSize = lobPageSize;
        //0,4096,8192,16384,32768,65536,131072,262144,524288
        if (lobPageSize != 0
                && lobPageSize != 4096
                && lobPageSize != 8192
                && lobPageSize != 16384
                && lobPageSize != 32768
                && lobPageSize != 65536
                && lobPageSize != 131072
                && lobPageSize != 262144
                && lobPageSize != 524288){
            throw new IOException("lobPageSize must in 0,4096,8192,16384,32768,65536,131072,262144,524288");
        }
    }

    public void setReplSize(int replSize) throws IOException {
        this.replSize = replSize;
        if (replSize < -1
                || replSize > 7){
            throw new IOException("replsize must be one of -1,0,1-7.");
        }
    }

    public int getReplSize() {
        return replSize;
    }
}
