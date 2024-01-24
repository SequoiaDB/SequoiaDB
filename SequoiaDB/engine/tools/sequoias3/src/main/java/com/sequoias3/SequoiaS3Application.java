package com.sequoias3;

import org.springframework.boot.Banner;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;

@SpringBootApplication
public class SequoiaS3Application {

    public static void main(String[] args) {
        SpringApplication app = new SpringApplication(SequoiaS3Application.class);
        app.setBannerMode(Banner.Mode.OFF);
        app.run(args);
    }
}
