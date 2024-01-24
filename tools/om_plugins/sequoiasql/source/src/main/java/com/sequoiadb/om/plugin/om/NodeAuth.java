package com.sequoiadb.om.plugin.om;

public class NodeAuth {
    private String user;
    private String passwd;
    private String defaultDb;

    public void setUser(String user) {
        this.user = user;
    }

    public String getUser() {
        return user;
    }

    public void setPasswd(String passwd) {
        this.passwd = passwd;
    }

    public String getPasswd() {
        return passwd;
    }

    public void setDefaultDb(String defaultDb) {
        this.defaultDb = defaultDb;
    }

    public String getDefaultDb() {
        return defaultDb;
    }
}
