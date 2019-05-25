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
@description: check SequoiaSQL OLAP
@author:
    2016-6-3 David Li  Init
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

var SSQL_OLAP_CHECK_FILE_NAME = "ssqlOlapCheck.js";

var SsqlOlapChecker = function(config, sysInfo) {
    this.config = config;
    this.sysInfo = sysInfo;
    this.base = new SsqlOlapCommon();
    this.logger = new Logger(SSQL_OLAP_CHECK_FILE_NAME);
    this.retVal = new SsqlOlapReturnValue();
};

SsqlOlapChecker.prototype._checkConfig = function SsqlOlapChecker__checkConfig (config) {
    try {
        this.base.checkConfig(config);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapChecker.prototype._checkSysInfo = function SsqlOlapChecker__checkSysInfo(sysInfo) {
    try {
        this.base.checkSysInfo(sysInfo);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapChecker.prototype.init = function SsqlOlapChecker_init() {
    if (!isTypeOf(this.sysInfo[TaskID], "number")) {
        var error = new SdbError(SDB_INVALIDARG, "TaskID is missed or invalid in sysInfo");
        this.logger.log(PDERROR, error);
        throw error;
    }
    this.logger.setTaskId(this.sysInfo[TaskID]);

    this.logger.log(PDDEBUG, JSON.stringify(this.config));
    this.logger.log(PDDEBUG, JSON.stringify(this.sysInfo));
    this._checkConfig(this.config);
    this._checkSysInfo(this.sysInfo);

    this.retVal[HostName] = this.config[HostName];
    this.retVal[Role] = this.config[Role];
    this.retVal[TaskID] = this.sysInfo[TaskID];
};

SsqlOlapChecker.prototype._connectToHost = function SsqlOlapChecker__connectToHost() {
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

SsqlOlapChecker.prototype._disconnectFromHost = function SsqlOlapChecker__disconnectFromHost() {
    if (this.userSsh != undefined) {
        try { this.userSsh.close(); } catch(e) {}
        this.userSsh = null;
    }
};

SsqlOlapChecker.prototype._needCheckInstallDir = function SsqlOlapChecker__needCheckInstallDir() {
    try {
        return this.base.needInstall(this.config);
    } catch(e) {
        this.logger.log(PDERROR, e);
        throw e;
    }
};

SsqlOlapChecker.prototype._checkInstallDir = function SsqlOlapChecker__checkInstallDir() {
    if (!this._needCheckInstallDir()) {
        var msg = sprintf("no need to check install dir for [?:?]", this.config[HostName], this.config[Role]);
        this.logger.log(PDDEBUG, msg);
        return;
    }

    var installDir = this.config[InstallDir];

    if (!this.userSsh.isPathExist(installDir)) {
        return;
    }
    
    if (!this.userSsh.isDirectory(installDir)) {
        var error = new SdbError(SDB_SYS, sprintf("sequoiasql olap install directory[?] is not directory in host[?]", installDir, this.config[HostName]));
        this.logger.log(PDERROR, error);
        throw error;
    }

    if (!this.userSsh.isEmptyDirectory(installDir)) {
        var error = new SdbError(SDB_SYS, sprintf("sequoiasql olap install directory[?] is not empty in host[?]", installDir, this.config[HostName]));
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapChecker.prototype._checkDataDir = function SsqlOlapChecker__checkDataDir() {
    var hostName = this.config[HostName];
    var role = this.config[Role];
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
    
    if (!this.userSsh.isPathExist(dataDir)) {
        return;
    }
    
    if (!this.userSsh.isDirectory(dataDir)) {
        var error = new SdbError(SDB_SYS, sprintf("sequoiasql olap data directory[?] is not directory in host[?]", dataDir, this.config[HostName]));
        this.logger.log(PDERROR, error);
        throw error;
    }

    if (!this.userSsh.isEmptyDirectory(dataDir)) {
        var error = new SdbError(SDB_SYS, sprintf("sequoiasql olap data directory[?] is not empty in host[?]", dataDir, this.config[HostName]));
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapChecker.prototype.check = function SsqlOlapChecker_check() {
    this.logger.log(PDEVENT, sprintf("begin to check sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
    try {
        this._connectToHost();
        this._checkInstallDir();
        this._checkDataDir();
    } catch(e) {
        throw e;
    } finally {
        this._disconnectFromHost();
    }
    this.logger.log(PDEVENT, sprintf("finsh checking sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
};

SsqlOlapChecker.prototype.finalize = function SsqlOlapChecker_finalize() {
    return this.retVal;
};

try {
    var checker = new SsqlOlapChecker(BUS_JSON, SYS_JSON);
    checker.init();
    checker.check();
    checker.finalize();
} catch(e) {
    var error = new SdbError(e);
    checker.logger.log(PDERROR, error);
    checker.logger.log(PDERROR, sprintf("failed to check sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
    throw error;
}