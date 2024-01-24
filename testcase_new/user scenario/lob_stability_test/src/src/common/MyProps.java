package common;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.Properties;

public class MyProps {
    private static final String PROPS_PATH = "./lob-stability.properties";
    
    private static Properties props = new Properties();
    static {
        try {
            props.load(new FileInputStream(PROPS_PATH));
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(-1);
        }
    }
    
    public static String get(final String key) {
        return props.getProperty(key);
    }
    
    public static int getInt(final String key) {
        return Integer.parseInt(props.getProperty(key));
    }
}
