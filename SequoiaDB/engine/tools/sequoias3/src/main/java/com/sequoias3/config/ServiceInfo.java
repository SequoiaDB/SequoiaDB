package com.sequoias3.config;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.boot.context.embedded.EmbeddedServletContainerInitializedEvent;
import org.springframework.context.ApplicationListener;
import org.springframework.stereotype.Component;

import java.lang.management.ManagementFactory;
import java.lang.management.RuntimeMXBean;
import java.net.InetAddress;
import java.net.UnknownHostException;

@Component
public class ServiceInfo implements ApplicationListener<EmbeddedServletContainerInitializedEvent> {
    private static final Logger logger = LoggerFactory.getLogger(ServiceInfo.class);
    private int port;
    private String host;
    private int pid;

    @Override
    public void onApplicationEvent(EmbeddedServletContainerInitializedEvent event){
        if (event != null){
            this.port = event.getEmbeddedServletContainer().getPort();
            RuntimeMXBean runtimeMXBean = ManagementFactory.getRuntimeMXBean();
            String name = runtimeMXBean.getName();
            this.pid = Integer.valueOf(name.substring(0, name.indexOf("@")));
        }
    }

    public ServiceInfo(){
        try {
            this.host = InetAddress.getLocalHost().getHostAddress();
        }catch (UnknownHostException e){
            logger.error("get server host failed. e", e);
        }
    }


    public int getPort(){
        return this.port;
    }

    public String getHost(){
        return this.host;
    }

    public int getPid() {
        return pid;
    }
}
