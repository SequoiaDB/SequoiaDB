本文档主要介绍 Java 驱动如何配置日志环境。

##日志环境配置##

Java 驱动使用 [SLF4J][SLF4J] 日志框架提供日志功能，项目中需要包含如下依赖：
* SLF4J 日志门面
* 日志实现
* 日志门面与日志实现的桥接器

> **Note:**
>
> 项目不包含 SLF4J 日志依赖时，Java 驱动日志功能将不生效。

###LogBack示例###

以 SLF4J 和 LogBack 为例：

1.需要在 pom.xml 文件中引入日志依赖。
 ```lang-xml
 <!-- SLF4J -->
 <dependency>
   <groupId>org.slf4j</groupId>
   <artifactId>slf4j-api</artifactId>
   <version>1.7.9</version>
 </dependency>

 <!-- LogBack, LogBack 自身已包含连接器 -->
 <dependency>
     <groupId>ch.qos.logback</groupId>
     <artifactId>logback-core</artifactId>
     <version>1.1.1</version>
 </dependency>
 <dependency>
     <groupId>ch.qos.logback</groupId>
     <artifactId>logback-classic</artifactId>
     <version>1.1.1</version>
 </dependency>
 ```

2.在 logback.xml 中指定 LogBack 的配置，建议单独为 com.sequoiadb 配置一个 Logger。
 ```lang-xml
 <configuration>
     <appender name="console"
               class="ch.qos.logback.core.ConsoleAppender">
         <encoder>
             <pattern>
                 %d %-5level [%thread] %logger{60} %line - %msg%n
             </pattern>
         </encoder>
     </appender>
     <logger name="com.sequoiadb" level="INFO" additivity="false">
         <appender-ref ref="console"/>
     </logger>
     <root level="OFF">
         <appender-ref ref="console" />
     </root>
 </configuration>
 ```
3.后续使用 Java 驱动即可打印日志信息。如需关闭 Java 驱动日志功能，可将 com.sequoiadb 的 Logger level 设置为 OFF。

###Log4J2示例###

以 SLF4J 和 Log4J2 为例：

1.需要在 pom.xml 文件中引入日志依赖。
 ```lang-xml
 <!-- SLF4J -->
 <dependency>
   <groupId>org.slf4j</groupId>
   <artifactId>slf4j-api</artifactId>
   <version>1.7.9</version>
 </dependency>

 <!-- SLF4J 与 Log4J2 连接器 -->
 <dependency>
   <groupId>org.apache.logging.log4j</groupId>
   <artifactId>log4j-slf4j-impl</artifactId>
   <version>2.13.2</version>
 </dependency>

 <!-- Log4J2 -->
 <dependency>
   <groupId>org.apache.logging.log4j</groupId>
   <artifactId>log4j-api</artifactId>
   <version>2.17.1</version>
 </dependency>
 <dependency>
   <groupId>org.apache.logging.log4j</groupId>
   <artifactId>log4j-core</artifactId>
   <version>2.17.1</version>
 </dependency>
 ```

2.在 log4j2.xml 中指定 Log4J2 的配置，建议单独为 com.sequoiadb 配置一个 Logger。
 ```lang-xml
 <?xml version="1.0" encoding="UTF-8"?>
 <Configuration status="INFO">
     <Appenders>
         <Console name="console" target="SYSTEM_OUT">
             <PatternLayout pattern="%d{yyyy-MM-dd HH:mm:ss.SSS} %-5level [%t] %logger{60} - %msg%n"/>
         </Console>
     </Appenders>
     <Loggers>
         <Logger name="com.sequoiadb" level="INFO" additivity="false">
            <AppenderRef ref="console"/>
         </Logger>
         <Root level="OFF">
             <AppenderRef ref="console"/>
         </Root>
     </Loggers>
 </Configuration>
 ```
3.后续使用 Java 驱动即可打印日志信息。如需关闭 Java 驱动日志功能，可将 com.sequoiadb 的 Logger level 设置为 OFF。


[^_^]:
    本文使用的所有引用和链接
[SLF4J]:https://www.slf4j.org