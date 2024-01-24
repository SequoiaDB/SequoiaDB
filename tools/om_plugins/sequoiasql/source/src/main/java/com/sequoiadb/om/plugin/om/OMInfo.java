package com.sequoiadb.om.plugin.om;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.PropertySource;
import org.springframework.stereotype.Component;

@Component
public class OMInfo {

    @Autowired
    private PluginConfig publicConf;

    private String hostName = "127.0.0.1";
    private String httpname;
    private String svcname;
    private String user;
    private String passwd;
    private long leaseTime;
    private long registerTime;

    public String getHostName() {
        return hostName;
    }

    public void setHostName(String hostName) {
        this.hostName = hostName;
    }

    public String getHttpname() {
        return publicConf.getOmhttpname();
    }

    public String getSvcname() {
        return svcname;
    }

    public void setSvcname(String svcname) {
        this.svcname = svcname;
    }

    public String getUser() {
        return user;
    }

    public void setUser(String user) {
        this.user = user;
    }

    public String getPasswd() {
        return passwd;
    }

    public void setPasswd(String passwd) {
        this.passwd = passwd;
    }

    public long getLeaseTime() {
        return leaseTime;
    }

    public void setLeaseTime(long leaseTime) {
        this.leaseTime = leaseTime;
    }

    public long getRegisterTime() {
        return registerTime;
    }

    public void setRegisterTime(long registerTime) {
        this.registerTime = registerTime;
    }
}

@Component
@ConfigurationProperties
@PropertySource("file:../../plugin.conf")
class PluginConfig {

    private String omhttpname = "8000";

    public String getOmhttpname() {
        return omhttpname;
    }

    public void setOmhttpname(String omhttpname) {
        this.omhttpname = omhttpname;
    }
}
