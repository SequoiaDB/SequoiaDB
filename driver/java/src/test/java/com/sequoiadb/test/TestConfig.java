package com.sequoiadb.test;

import java.io.IOException;
import java.io.InputStream;
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
    private static final String singlePort = "single.port";
    private static final String singleUsername = "single.username";
    private static final String singlePassword = "single.password";
    private static final String singleGroup = "single.group";
    private static final String nodeHost = "node.host";
    private static final String nodePort = "node.port";

    private TestConfig() {
    }

    public static String getSingleHost() {
        return properties.getProperty(singleHost);
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

    public static String getSingleGroup() {
        return properties.getProperty(singleGroup);
    }

    public static String getNodeHost() {
        return properties.getProperty(nodeHost);
    }

    public static int getNodePort() {
        String port = properties.getProperty(nodePort);
        if (port != null && !port.isEmpty()) {
            return Integer.valueOf(port);
        } else {
            return 0;
        }
    }
}