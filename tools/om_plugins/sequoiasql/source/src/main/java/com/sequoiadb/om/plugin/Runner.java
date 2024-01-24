package com.sequoiadb.om.plugin;

import com.sequoiadb.om.plugin.om.AutoRegister;
import com.sequoiadb.om.plugin.om.OMClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.CommandLineRunner;
import org.springframework.stereotype.Component;

@Component
public class Runner implements CommandLineRunner {

    @Autowired
    private AutoRegister reg;

    @Override
    public void run(String... strings) throws Exception {
        final Logger logger = LoggerFactory.getLogger(Runner.class);

        logger.info("Event: Init");

        reg.start();
    }
}

