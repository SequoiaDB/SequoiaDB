package com.sequoiadb.om.plugin.om;

public class SequoiaSQLNode {
    private String hostName;
    private String svcName;

    public void setHostName(String hostName) {
        this.hostName = hostName;
    }

    public String getHostName() {
        return hostName;
    }

    public void setSvcName(String svcName) {
        this.svcName = svcName;
    }

    public String getSvcName() {
        return svcName;
    }
}
