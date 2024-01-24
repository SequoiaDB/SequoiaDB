package com.sequoiadb.om.plugin.om;

import com.sequoiadb.om.plugin.config.ConfigFactory;
import com.sequoiadb.om.plugin.config.SequoiaSQLConfig;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import java.util.*;

@Component
public class AutoRegister extends TimerTask {

    private final Logger logger = LoggerFactory.getLogger(AutoRegister.class);
    private Timer timer = new Timer();

    @Autowired
    private OMInfo omConf;

    @Autowired
    private OMClient client;

    @Autowired
    private ConfigFactory factory;

    public void start() {
        timer.schedule(this, 0, 1000);
    }

    public synchronized boolean register(boolean isInit) {

        if (isInit) {
            factory.initConfig();
        }

        boolean rc = true;
        Iterator it = factory.iterator();
        while (it.hasNext()) {
            if (client.registerPlugin((SequoiaSQLConfig) it.next()) == false) {
                rc = false;
            }
        }

        if (rc) {
            logger.info("Event: timer cancel");
            timer.cancel();
        }

        return rc;
    }

    public synchronized boolean isTimeout() {
        long currentTime = new Date().getTime() / 1000;
        return currentTime - omConf.getRegisterTime() >= omConf.getLeaseTime();
    }

    public void run() {
        register(false);
    }


}