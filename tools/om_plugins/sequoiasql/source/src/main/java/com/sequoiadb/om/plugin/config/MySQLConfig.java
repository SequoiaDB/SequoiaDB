package com.sequoiadb.om.plugin.config;

import java.net.InetAddress;
import java.net.UnknownHostException;

public class MySQLConfig extends SequoiaSQLConfig {

    MySQLConfig() throws UnknownHostException {
        InetAddress addr = InetAddress.getLocalHost();
        hostName = addr.getHostName().toString();

        name = "SequoiaSQL";
        type = "sequoiasql-mysql";
    }
}