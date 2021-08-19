package com.sequoias3.config;

import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

import java.util.ArrayList;
import java.util.List;

@Component
@ConfigurationProperties(prefix = "sdbs3.sequoiadb")
public class SequoiadbConfig {
    String url;
    List<String> urlList;
    SequoiadbMetaConfig meta;
    SequoiadbDataConfig data;
    String auth;
    String username;
    String password;

    int     maxConnectionNum   = 500;
    int     deltaIncCount      = 20;
    int     maxIdleNum         = 20;
    int     keepAliveTime      = 0;
    int     CheckInterval      = 60000;
    boolean validateConnection = true;

    public String getMetaCsName() {
        return meta.getCsName();
    }

    public String getMetaDomain() {
        return meta.getDomain();
    }

    public String getDataCsName() {
        return data.getCsName();
    }

    public String getDataDomain() {
        return data.getDomain();
    }

    public int getDataCSRange() {
        return data.getCsRange();
    }

    public Integer getDataLobPageSize() {
        return data.getLobPageSize();
    }

    public Integer getDataReplSize() {
        return data.getReplSize();
    }

    public String getUrl() {
        return url;
    }

    public void setUrl(String url) {
        this.url = url;

        urlList = new ArrayList<>();
        int index = url.indexOf("://");
        String[] ipPorts = url.substring(index + 3).split(",");
        for (int i = 0; i < ipPorts.length; i++) {
            urlList.add(ipPorts[i]);
        }
    }

    public List<String> getUrlList() {
        return urlList;
    }

    public String getAuth() {
        return auth;
    }

    public void setAuth(String auth) {
        this.auth = auth;

        if (auth != null){
            String[] userPassword = auth.split(":");
            if (userPassword.length > 1) {
                this.username = userPassword[0];
                this.password = userPassword[1];
            }
        }
    }

    public String getUsername() {
        return username;
    }

    public String getPassword() {
        return password;
    }

    public SequoiadbMetaConfig getMeta() {
        return meta;
    }

    public void setMeta(SequoiadbMetaConfig meta) {
        this.meta = meta;
    }

    public SequoiadbDataConfig getData() {
        return data;
    }

    public void setData(SequoiadbDataConfig data) {
        this.data = data;
    }

    public void setCheckInterval(int checkInterval) {
        CheckInterval = checkInterval;
    }

    public int getCheckInterval() {
        return CheckInterval;
    }

    public void setDeltaIncCount(int deltaIncCount) {
        this.deltaIncCount = deltaIncCount;
    }

    public int getDeltaIncCount() {
        return deltaIncCount;
    }

    public void setKeepAliveTime(int keepAliveTime) {
        this.keepAliveTime = keepAliveTime;
    }

    public int getKeepAliveTime() {
        return keepAliveTime;
    }

    public void setMaxConnectionNum(int maxConnectionNum) {
        this.maxConnectionNum = maxConnectionNum;
    }

    public int getMaxConnectionNum() {
        return maxConnectionNum;
    }

    public void setMaxIdleNum(int maxIdleNum) {
        this.maxIdleNum = maxIdleNum;
    }

    public int getMaxIdleNum() {
        return maxIdleNum;
    }

    public void setValidateConnection(boolean validateConnection) {
        this.validateConnection = validateConnection;
    }

    public boolean getValidateConnection() {
        return validateConnection;
    }
}
