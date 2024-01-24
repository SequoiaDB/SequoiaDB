package com.sequoiadb.om.plugin.config;

import java.net.InetAddress;
import java.net.UnknownHostException;

public class MariaDBConfig extends SequoiaSQLConfig {
    MariaDBConfig() throws UnknownHostException {
        InetAddress addr = InetAddress.getLocalHost();
        hostName = addr.getHostName().toString();

        name = "SequoiaSQL";
        type = "sequoiasql-mariadb";
    }
}
