#SequoiaDB Java Driver#

We use Maven to compile, test and package.

See http://maven.apache.org.

##Compile##

Run command:

```
mvn clean compile
```

##Test##

You should make sure that SequoiaDB servers are running, and set a profile for Maven.

For example:

```
<profile>
    <id>dev</id>
    <properties>
        <single.host>ubuntu-dev1</single.host>
        <single.ip>192.168.20.42</single.ip>
        <single.port>11810</single.port>
        <single.username></single.username>
        <single.password></single.password>
        <cluster.urls>${single.host}:${single.port}</cluster.urls>
        <cluster.username>${single.username}</cluster.username>
        <cluster.password>${single.password}</cluster.password>
        <coord.host>${single.host}</coord.host>
        <coord.port>${single.port}</coord.port>
        <coord.path>/sequoiadb/database/coord/${coord.port}</coord.path>
        <data.group>db1</data.group>
        <data.host>${single.host}</data.host>
        <data.port>20000</data.port>
        <rbac.root.username>${rbac.root.username}</data.port>
        <rbac.root.password>${rbac.root.password}</data.port>
        <rbac.coord.host>${rbac.coord.host}</data.port>
        <rbac.coord.port>${rbac.coord.port}</data.port>
        <rbac.newNode.dbPathPrefix>${rbac.newNode.dbPathPrefix}</data.port>
    </properties>
</profile>
```

The profile can be set in maven setting.xml file in your home path(recommended) or pom.xml, 

Then run command:

```
mvn clean test -Pdev
```

You can specify which testcase to run by using -Dtest=<testcase>:

```
mvn clean test -Pdev -Dtest=Test*
```

See maven-surefire-plugin document for more details.

##Package##

Run command:

```
mvn clean package
```

If you want to skip test when package, run command:
```
mvn clean package -Dmaven.test.skip=true
```

If you also want to generate sourcecode and javadoc package, run command:

```
mvn clean package -Prelease
```

##Release to Central Repository##
```
mvn clean deploy -Prelease
```

If you only want to install to local(skip test/gpg), run command:

```
mvn clean install -Prelease -Dmaven.test.skip=true -Dgpg.skip
```
Note that you should have a OSSRH JIRA account and executable "pgp" command in local host.
See http://central.sonatype.org/pages/ossrh-guide.html for details.
And you should configure password of GPG secret key and OSSRH JIRA account in ~/.m2/settings.xml:
```
<profiles>
    <profile>
        <id>ossrh</id>
        <activation>
            <activeByDefault>true</activeByDefault>
        </activation>
        <properties>
            <gpg.passphrase>password of GPG secret key</gpg.passphrase>
        </properties>
    </profile>
</profiles>

<servers>
    <server>
        <id>ossrh</id>
        <username>your JIRA username</username>
        <password>your JIRA password</password>
    </server>
</servers>

```