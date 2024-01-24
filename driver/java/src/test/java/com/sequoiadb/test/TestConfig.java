package com.sequoiadb.test;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

public class TestConfig {
    private static final String propertiesFileName = "test.properties";
    private static Properties properties;

    static {
        properties = new Properties();
        InputStream in = TestConfig.class.getClassLoader().getResourceAsStream(propertiesFileName);
        try {
            properties.load(in);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static final String singleHost = "single.host";
    private static final String singleIP = "single.ip";
    private static final String singlePort = "single.port";
    private static final String singleUsername = "single.username";
    private static final String singlePassword = "single.password";

    private static final String clusterUrls = "cluster.urls";
    private static final String clusterUsername = "cluster.username";
    private static final String clusterPassword = "cluster.password";
    private static final String coordHost = "coord.host";
    private static final String coordPort = "coord.port";
    private static final String coordPath = "coord.path";
    private static final String dataGroup = "data.group";
    private static final String dataHost = "data.host";
    private static final String dataPort = "data.port";
    private static final String datasourceAddress = "datasource.address";
    private static final String datasourceUrls = "datasource.urls";
    private static final String dbPath = "dbPath";
    private static final String rbacRootUsername = "rbac.root.username";
    private static final String rbacRootPassword = "rbac.root.password";
    private static final String rbacCoordHost = "rbac.coord.host";
    private static final String rbacCoordPort = "rbac.coord.port";
    private static final String rbacNewNodeDbPathPrefix = "rbac.newNode.dbPathPrefix";

    private TestConfig() {
    }

    public static String getSingleHost() {
        return properties.getProperty(singleHost);
    }

    public static String getSingleIP() {
        return properties.getProperty(singleIP);
    }

    public static String getSinglePort() {
        return properties.getProperty(singlePort);
    }

    public static String getSingleUsername() {
        return properties.getProperty(singleUsername);
    }

    public static String getSinglePassword() {
        return properties.getProperty(singlePassword);
    }

    public static List<String> getClusterUrls() {
        String urls = properties.getProperty(clusterUrls);
        String[] elems = urls.split(",");
        List<String> results = new ArrayList<String>();
        for(String url : elems) {
            if(!url.isEmpty()) {
                results.add(url);
            }
        }
        return results;
    }

    public static String getClusterUsername() {
        return properties.getProperty(clusterUsername);
    }

    public static String getClusterPassword() {
        return properties.getProperty(clusterPassword);
    }

    public static String getCoordHost() {
        return properties.getProperty(coordHost);
    }

    public static String getCoordPort() {
        return properties.getProperty(coordPort);
    }

    public static String getCoordPath() {
        return properties.getProperty(coordPath);
    }

    public static String getDataGroupName() {
        return properties.getProperty(dataGroup);
    }

    public static String getDataHost() {
        return properties.getProperty(dataHost);
    }

    public static int getDataPort() {
        String port = properties.getProperty(dataPort);
        if (port != null && !port.isEmpty()) {
            return Integer.valueOf(port);
        } else {
            return 0;
        }
    }

    public static String getDatasourceAddress() {
        return properties.getProperty(datasourceAddress);
    }

    public static String getDatasourceUrls() {
        return properties.getProperty(datasourceUrls);
    }

    public static String getDBPath() {
        return properties.getProperty(dbPath);
    }

    public static String getRbacRootUsername() {
        return properties.getProperty(rbacRootUsername);
    }

    public static String getRbacRootPassword() {
        return properties.getProperty(rbacRootPassword);
    }

    public static String getRbacCoordHost() {
        return properties.getProperty(rbacCoordHost);
    }

    public static int getRbacCoordPort() {
        return Integer.parseInt(properties.getProperty(rbacCoordPort));
    }

    public static String getRbacNewNodeDbPathPrefix() {
        return properties.getProperty(rbacNewNodeDbPathPrefix);
    }

}