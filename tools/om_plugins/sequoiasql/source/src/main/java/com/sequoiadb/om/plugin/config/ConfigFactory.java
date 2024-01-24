package com.sequoiadb.om.plugin.config;

import org.springframework.stereotype.Component;

import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

@Component
public class ConfigFactory {
    private List<SequoiaSQLConfig> configList = new ArrayList<SequoiaSQLConfig>();

    ConfigFactory() throws UnknownHostException {
        configList.add(new PostgreSQLConfig());
        configList.add(new MySQLConfig());
        configList.add(new MariaDBConfig());
    }

    public void init(){

    }

    public void initConfig(){
        for(int i = 0 ; i < configList.size() ; i++) {
            configList.get(i).setIsRegister(false);
        }
    }

    public Iterator iterator(){
        return configList.iterator();
    }
}
