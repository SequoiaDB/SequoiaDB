package com.sequoiadb.om.plugin.config;

public abstract class SequoiaSQLConfig {

    protected final String role = "plugin";
    protected String hostName;
    protected String name;
    protected String type;
    protected boolean isRegister = false;


    public String getHostName() {
        return hostName;
    }

    public String getName() {
        return name;
    }

    public String getRole() {
        return role;
    }

    public String getType() {
        return type;
    }

    public boolean getIsRegister(){ return isRegister; }

    public void setIsRegister(Boolean isRegister){ this.isRegister = isRegister; }
}
