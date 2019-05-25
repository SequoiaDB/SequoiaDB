/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: install SequoiaSQL OLAP
@author:
    2016-5-25 David Li  Init
@parameter
    BUS_JSON: the format is: 
    {
        "HostName": "centos-lwc",
        "role": "master",
        "master_host": "centos-lwc",
        "master_port": "5432",
        "master_dir": "/opt/sequoiasql/database/master/5432",
        "standby_host": "",
        "segment_port": "40000",
        "segment_dir": "/opt/sequoiasql/database/segment/40000",
        "segment_hosts": [
          "ubuntu-lwc",
          "centos-lwc"
        ],
        "hdfs_url": "s:12000",
        "master_temp_dir": "/tmp",
        "segment_temp_dir": "/tmp",
        "install_dir": "/opt/sequoiasql/olap",
        "User": "root",
        "Passwd": "sequoiadb",
        "SshPort": "22"
    }
    SYS_JSON: the format is:
    {
        "TaskID": 1,
        "ClusterName": "mycluster",
        "BusinessType": "sequoiasql",
        "BusinessName": "mysql",
        "DeployMod": "olap",
        "SdbUser": "sdbadmin",
        "SdbPasswd": "sdbadmin",
        "SdbUserGroup": "sdbadmin_group",
        "InstallPacket": "" 
    }
@return
   RET_JSON: the format is: { "errno": 0, "detail": "", "HostName": "centos-lwc", "role": "master", "TaskID": 4 }
*/

/* test data
var BUS_JSON = {
    "HostName": "centos-lwc",
    "role": "master",
    "master_host": "centos-lwc",
    "master_port": "5432",
    "master_dir": "/opt/sequoiasql/database/master/5432",
    "standby_host": "",
    "segment_port": "40000",
    "segment_dir": "/opt/sequoiasql/database/segment/40000",
    "segment_hosts": [
        "ubuntu-lwc",
        "centos-lwc"
    ],
    "hdfs_url": "s:12000",
    "master_temp_dir": "/tmp",
    "segment_temp_dir": "/tmp",
    "install_dir": "/opt/sequoiasql/olap",
    "User": "root",
    "Passwd": "sequoiadb",
    "SshPort": "22"
};

var SYS_JSON = {
    "TaskID": 5,
    "ClusterName": "mycluster",
    "BusinessType": "sequoiasql",
    "BusinessName": "mysql",
    "DeployMod": "olap",
    "SdbUser": "sdbadmin",
    "SdbPasswd": "sdbadmin",
    "SdbUserGroup": "sdbadmin_group",
    "InstallPacket": "/opt/sequoiadb/bin/../packet/sequoiasql-1.0-installer.run" 
};

setLogLevel(PDDEBUG);
*/

var SSQL_OLAP_INSTALL_FILE_NAME = "ssqlOlapInstall.js";

var SsqlOlapInstaller = function(config, sysInfo) {
    this.config = config;
    this.sysInfo = sysInfo;
    this.base = new SsqlOlapCommon();
    this.logger = new Logger(SSQL_OLAP_INSTALL_FILE_NAME);
    this.retVal = new SsqlOlapReturnValue();
};

SsqlOlapInstaller.prototype._checkConfig = function SsqlOlapInstaller__checkConfig (config) {
    try {
        this.base.checkConfig(config);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapInstaller.prototype._checkSysInfo = function SsqlOlapInstaller__checkSysInfo(sysInfo) {
    try {
        this.base.checkSysInfo(sysInfo);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapInstaller.prototype.init = function SsqlOlapInstaller_init() {
    if (!isTypeOf(this.sysInfo[TaskID], "number")) {
        var error = new SdbError(SDB_INVALIDARG, "TaskID is missed or invalid in sysInfo");
        this.logger.log(PDERROR, error);
        throw error;
    }
    this.logger.setTaskId(this.sysInfo[TaskID]);
    this._checkConfig(this.config);
    this._checkSysInfo(this.sysInfo);

    this.retVal[HostName] = this.config[HostName];
    this.retVal[Role] = this.config[Role];
    this.retVal[TaskID] = this.sysInfo[TaskID];

    this.logger.log(PDDEBUG, JSON.stringify(this.config));
    this.logger.log(PDDEBUG, JSON.stringify(this.sysInfo));
};

SsqlOlapInstaller.prototype._checkPackage = function SsqlOlapInstaller__checkPackage() {
    var pack = this.sysInfo[InstallPacket];
    if (!File.exist(pack)) {
        var msg = sprintf("failed to find package [?]", pack);
        var error = new SdbError(SDB_INVALIDARG, msg);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapInstaller.prototype._connectToHost = function SsqlOlapInstaller__connectToHost() {
    var hostName = this.config[HostName];
    var user = this.config[User];
    var passwd = this.config[Passwd];
    var sshPort = parseInt(this.config[SshPort]);
    var userSsh;

    try {
        userSsh = this.base.connectToHost(hostName, user, passwd, sshPort);
    } catch(e) {
        this.logger.log(PDERROR, e);
        throw e;
    }

    this.userSsh = userSsh;
};

SsqlOlapInstaller.prototype._connectToHostBySdb = function SsqlOlapInstaller__connectToHostBySdb() {
    var hostName = this.config[HostName];
    var sshPort = parseInt(this.config[SshPort]);
    var sdbUser = this.sysInfo[SdbUser];
    var sdbPasswd = this.sysInfo[SdbPasswd];
    var sdbSsh;

    try {
        sdbSsh = this.base.connectToHost(hostName, sdbUser, sdbPasswd, sshPort);
    } catch(e) {
        this.logger.log(PDERROR, e);
        throw e;
    }

    this.sdbSsh = sdbSsh;
};

SsqlOlapInstaller.prototype._disconnectFromHost = function SsqlOlapInstaller__disconnectFromHost() {
    if (this.userSsh != undefined) {
        try { this.userSsh.close(); } catch(e) {}
        this.userSsh = null;
    }

    if (this.sdbSsh != undefined) {
        try { this.sdbSsh.close(); } catch(e) {}
        this.sdbSsh = null;
    }
};

SsqlOlapInstaller.prototype._checkHost = function SsqlOlapInstaller__checkHost() {
    var hostName = this.config[HostName];
    var role = this.config[Role];
    var installDir = this.config[InstallDir];
    var dataDir;
    var tempDir;

    if (role == Master || role == Standby) {
        dataDir = this.config[MasterDir];
        tempDir = this.config[MasterTempDir];
    } else if (role == Segment) {
        dataDir = this.config[SegmentDir];
        tempDir = this.config[SegmentTempDir];
    } else {
        throw new SdbError(SDB_INVALIDARG, sprintf("invalid role: ?", role));
    }

    if (this.userSsh.isPathExist(installDir)) {
        if (!this.userSsh.isEmptyDirectory(installDir)) {
            var error = new SdbError(SDB_SYS, sprintf("the path [?] is not empty in host[?]", installDir, hostName));
            this.logger.log(PDERROR, error);
            throw error;
        }
    }

    if (this.userSsh.isPathExist(dataDir)) {
        if (!this.userSsh.isEmptyDirectory(dataDir)) {
            var error = new SdbError(SDB_SYS, sprintf("the path [?] is not empty", dataDir, hostName));
            this.logger.log(PDERROR, error);
            throw error;
        }
    }
};

SsqlOlapInstaller.prototype._needInstall = function SsqlOlapInstaller__needInstall() {
    try {
        return this.base.needInstall(this.config);
    } catch(e) {
        this.logger.log(PDERROR, e);
        throw e;
    }
};

SsqlOlapInstaller.prototype._installPackage = function SsqlOlapInstaller__installPackage() {
    if (!this._needInstall()) {
        var msg = sprintf("no need to install package for [?:?]", this.config[HostName], this.config[Role]);
        this.logger.log(PDEVENT, msg);
        return;
    }

    var pack = this.sysInfo[InstallPacket];
    var packName = getPacketName(pack);
    var remotePack = adaptPath(OMA_PATH_TEMP_PACKET_DIR) + packName;
    var installDir = this.config[InstallDir];
    var sdbUser = this.sysInfo[SdbUser];
    var sdbPasswd = this.sysInfo[SdbPasswd];

    if (!this.userSsh.isPathExist(OMA_PATH_TEMP_PACKET_DIR)) {
        this.userSsh.mkdir(OMA_PATH_TEMP_PACKET_DIR);
    }

    try {
        this.userSsh.push(pack, remotePack);
    } catch(e) {
        var error = new SdbError(e, sprintf("failed to copy package[?] to remote host[?:?]", pack, this.config[HostName], remotePack));
        this.logger.log(PDERROR, error);
        throw error;
    }

    try {
        var shell = sprintf("? --prefix ? --username ? --userpasswd ? --mode unattended", remotePack, installDir, sdbUser, sdbPasswd);
        this.logger.log(PDDEBUG, "install cmd: " + shell);
        this.userSsh.exec(shell);
    } catch(e) {
        var error = new SdbError(e, sprintf("failed to install package[?] in host[?]", remotePack, this.config[HostName]));
        this.logger.log(PDERROR, error);
        throw error;
    }

    try {
        this.userSsh.remove(OMA_PATH_TEMP_PACKET_DIR, true, true);
    } catch(e) {
        this.logger.log(PDWARNING, e);
    }

};

SsqlOlapInstaller.prototype._config = function SsqlOlapInstaller__config() {  
    if (this.config[Role] != Master) {
        var msg = sprintf("no need to config for [?:?]", this.config[HostName], this.config[Role]);
        this.logger.log(PDEVENT, msg);
        return;
    }

    this._connectToHostBySdb();

    var installDir = this.config[InstallDir];
    var etc = adaptPath(installDir) + "etc";
    var slaves = adaptPath(etc) + "slaves";
    var segmentHosts = this.config[SegmentHosts];

    if (this.sdbSsh.isPathExist(slaves)) {
        this.sdbSsh.remove(slaves, true);
    }

    // append segment hosts to "slaves" file
    for (var i = 0; i < segmentHosts.length; i++) {
        var shell = sprintf("echo ? >> ?", segmentHosts[i], slaves);
        try {
            this.sdbSsh.exec(shell);
        } catch(e) {
            var error = new SdbError(e, "failed to write segment host to slaves");
            this.logger.log(PDERROR, error);
            throw error;
        }
    }

    var tmpConf = adaptPath(OMA_PATH_TEMP_TEMP_DIR) + "ssqlOlapConfig";
    var conf = adaptPath(etc) + "sequoiasql-site.xml";

    try {
        var config = new SsqlOlapConfig();
        config.updateProperty("hawq_master_address_host", this.config[MasterHost]);
        config.updateProperty("hawq_master_address_port", this.config[MasterPort]);
        config.updateProperty("hawq_master_directory", this.config[MasterDir]);
        config.updateProperty("hawq_standby_address_host", this.config[StandbyHost]);
        config.updateProperty("hawq_segment_address_port", this.config[SegmentPort]);
        config.updateProperty("hawq_segment_directory", this.config[SegmentDir]);
        config.updateProperty("hawq_dfs_url", this.config[HdfsUrl]);
        config.updateProperty("hawq_master_temp_directory", this.config[MasterTempDir]);
        config.updateProperty("hawq_segment_temp_directory", this.config[SegmentTempDir]);
        config.genXmlConfig(tmpConf);
    } catch(e) {
        var error = new SdbError(e, "failed to generate xml config");
        this.logger.log(PDERROR, error);
        throw error;
    }

    if (this.sdbSsh.isPathExist(conf)) {
        this.sdbSsh.remove(conf, true);
    }

    try {
        this.sdbSsh.push(tmpConf, conf);
    } catch(e) {
        var error = new SdbError(e, sprintf("failed to copy [?] to [?:?]", tmpConf, this.config[HostName], conf));
        this.logger.log(PDERROR, error);
        throw error;
    }

    this.sdbSsh.chmod(conf, 644);
    File.remove(tmpConf);
};

SsqlOlapInstaller.prototype._ensureDirs = function SsqlOlapInstaller__ensureDirs() {
    var hostName = this.config[HostName];
    var role = this.config[Role];
    var dataDir;
    var tempDir;
    var logDir;

    if (role == Master || role == Standby) {
        dataDir = this.config[MasterDir];
        tempDir = this.config[MasterTempDir];
    } else if (role == Segment) {
        dataDir = this.config[SegmentDir];
        tempDir = this.config[SegmentTempDir];
    } else {
        throw new SdbError(SDB_INVALIDARG, sprintf("invalid role: ?", role));
    }
    logDir = this.config[LogDir];

    this.logger.log(PDDEBUG, "ensure dir: " + hostName + ":" + dataDir);
    this.logger.log(PDDEBUG, "ensure dir: " + hostName + ":" + tempDir);
    this.logger.log(PDDEBUG, "ensure dir: " + hostName + ":" + logDir);

    if (this.userSsh.isPathExist(dataDir)) {
        if (!this.userSsh.isEmptyDirectory(dataDir)) {
            var error = new SdbError(SDB_SYS, sprintf("the path [?] is not empty", dataDir, hostName));
            this.logger.log(PDERROR, error);
            throw error;
        }
    } else {
        this.userSsh.mkdir(dataDir);
        this.userSsh.chown(dataDir, this.sysInfo[SdbUser], this.sysInfo[SdbUserGroup], true);
    }

    if (!this.userSsh.isPathExist(tempDir)) {
        this.userSsh.mkdir(tempDir);
        this.userSsh.chmod(tempDir, 777, true);
    }

    if (!this.userSsh.isPathExist(logDir)) {
        this.userSsh.mkdir(logDir);
        this.userSsh.chown(logDir, this.sysInfo[SdbUser], this.sysInfo[SdbUserGroup], true);
    }
};

SsqlOlapInstaller.prototype.install = function SsqlOlapInstaller_install() {
    this.logger.log(PDEVENT, sprintf("begin to install sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
    try {
        this._checkPackage();
        this._connectToHost();
        this._checkHost();
        this._installPackage();
        this._config();
        this._ensureDirs();
    } catch(e) {
        throw e;
    } finally {
        this._disconnectFromHost();
    }
    this.logger.log(PDEVENT, sprintf("finsh installing sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
};

SsqlOlapInstaller.prototype.finalize = function SsqlOlapInstaller_finalize() {
    return this.retVal;
};

try {
    var installer = new SsqlOlapInstaller(BUS_JSON, SYS_JSON);
    installer.init();
    installer.install();
    installer.finalize();
} catch(e) {
    var error = new SdbError(e);
    installer.logger.log(PDERROR, error);
    installer.logger.log(PDERROR, sprintf("failed to install sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
    throw error;
}
