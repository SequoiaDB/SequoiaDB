code：源码目录及测试样例
example：testng测试代码，只是代码。要运行请直接加到测试现有sequoiadb-testcases-story-java工程中，并修改相应的testng.xml

有时候testng中有些异常无法显示，时testng插件中自己过滤掉了。可以在eclipse中testng的配置项中不过滤异常。做法如下：
在windows->Preferences中找到testng配置项 Excluded StackTrace中去掉对所有异常的过滤即可。