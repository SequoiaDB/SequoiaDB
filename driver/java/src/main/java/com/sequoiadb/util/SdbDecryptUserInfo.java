package com.sequoiadb.util;

public class SdbDecryptUserInfo {
    private String userName;
    private String clusterName;
    private String passwd;

    /**
     * Get the cluster name
     *
     * @return cluster name
     */
    public String getClusterName() {
        return clusterName;
    }

    void setClusterName(String clusterName) {
        this.clusterName = clusterName;
    }

    /**
     * Get the user name
     *
     * @return user name
     */
    public String getUserName() {
        return userName;
    }

    void setUserName(String userName) {
        this.userName = userName;
    }

    /**
     * Get the password
     *
     * @return password
     */
    public String getPasswd() {
        return passwd;
    }

    void setPasswd(String passwd) {
        this.passwd = passwd;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("user:").append(userName).append(",cluster:").append(clusterName)
                .append(",passwd:").append(passwd);

        return sb.toString();
    }
}
