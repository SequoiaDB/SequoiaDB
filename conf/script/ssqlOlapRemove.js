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
@description: remove SequoiaSQL OLAP
@author:
    2016-6-1 David Li  Init
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

var SSQL_OLAP_REMOVE_FILE_NAME = "ssqlOlapRemove.js";

var SsqlOlapRemover = function(config, sysInfo) {
    this.config = config;
    this.sysInfo = sysInfo;
    this.base = new SsqlOlapCommon();
    this.logger = new Logger(SSQL_OLAP_REMOVE_FILE_NAME);
    this.retVal = new SsqlOlapReturnValue();
};

SsqlOlapRemover.prototype._checkConfig = function SsqlOlapRemover__checkConfig (config) {
    try {
        this.base.checkConfig(config);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapRemover.prototype._checkSysInfo = function SsqlOlapRemover__checkSysInfo(sysInfo) {
    try {
        this.base.checkSysInfo(sysInfo);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapRemover.prototype.init = function SsqlOlapRemover_init() {
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

SsqlOlapRemover.prototype._connectToHost = function SsqlOlapRemover__connectToHost() {
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

SsqlOlapRemover.prototype._connectToHostBySdb = function SsqlOlapRemover__connectToHostBySdb() {
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

SsqlOlapRemover.prototype._disconnectFromHost = function SsqlOlapRemover__disconnectFromHost() {
    if (this.userSsh != undefined) {
        try { this.userSsh.close(); } catch(e) {}
        this.userSsh = null;
    }

    if (this.sdbSsh != undefined) {
        try { this.sdbSsh.close(); } catch(e) {}
        this.sdbSsh = null;
    }
};

SsqlOlapRemover.prototype._removeData = function SsqlOlapRemover__removeData() {
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

    if (this.userSsh.isPathExist(dataDir)) {
        this.userSsh.remove(dataDir, true, true);
    }
};

SsqlOlapRemover.prototype._needUninstall = function SsqlOlapRemover__needUninstall() {
    try {
        return this.base.needInstall(this.config);
    } catch(e) {
        this.logger.log(PDERROR, e);
        throw e;
    }
};

SsqlOlapRemover.prototype._stopProcesses = function SsqlOlapRemover__stopProcesses() {
    if (!this._needUninstall()) {
        var msg = sprintf("no need to stop processes for [?:?]", this.config[HostName], this.config[Role]);
        this.logger.log(PDDEBUG, msg);
        return;
    }

    var stopMaster = false;
    var stopStandby = false;
    var stopSegment = false;
    var hostName = this.config[HostName];
    var segments = this.config[SegmentHosts];

    if (hostName == this.config[MasterHost]) {
        stopMaster = true;
    }
    if (hostName == this.config[StandbyHost]) {
        stopStandby = true;
    }
    for (var i in segments) {
        if (hostName == segments[i]) {
            stopSegment = true;
            break;
        }
    }

    try {
        if (stopSegment) {
            this.logger.log(PDDEBUG, "stop segment at " + hostName);
            this.base.stop(this.sdbSsh, this.config, ObjSegment);
        }
        if (stopStandby) {
            this.logger.log(PDDEBUG, "stop standby at " + hostName);
            this.base.stop(this.sdbSsh, this.config, ObjStandby);
        }
        if (stopMaster) {
            this.logger.log(PDDEBUG, "stop master at " + hostName);
            this.base.stop(this.sdbSsh, this.config, ObjMaster);
        }
    } catch(e) {
        if (e instanceof SdbError) {
            this.logger.log(PDWARNING, e);
        } else {
            var error = new SdbError(e);
            this.logger.log(PDERROR, error);
            throw error;
        }
    }

    var shell = "killall sequoiasql";
    try { this.sdbSsh.exec(shell); } catch(e) {}

    //shell = "killall gpcheck";
    //try { this.sdbSsh.exec(shell); } catch(e) {}
};

SsqlOlapRemover.prototype._uninstallPackage = function SsqlOlapRemover__uninstallPackage() {
    if (!this._needUninstall()) {
        var msg = sprintf("no need to uninstall package for [?:?]", this.config[HostName], this.config[Role]);
        this.logger.log(PDDEBUG, msg);
        return;
    }

    var installDir = this.config[InstallDir];
    var uninstall = adaptPath(installDir) + "uninstall";

    if (!this.userSsh.isPathExist(installDir)) {
        return;
    }

    if (!this.userSsh.isEmptyDirectory(installDir)) {
        if (!this.userSsh.isPathExist(uninstall)) {
            var error = new SdbError(sprintf("failed to find [?] in host[?]", uninstall, this.config[HostName]));
            this.logger.log(PDERROR, error);
            throw error;
        }

        try {
            var shell = sprintf("? --mode unattended", uninstall);
            this.logger.log(PDDEBUG, "uninstall cmd: " + shell);
            this.userSsh.exec(shell);
        } catch(e) {
            var error = new SdbError(e, sprintf("failed to uninstall sequoiasql olap[?] in host[?]", installDir, this.config[HostName]));
            this.logger.log(PDERROR, error);
            throw error;
        }
    }

    try {
        this.userSsh.remove(installDir, true, true);
    } catch(e) {
        this.logger.log(PDERROR, e);
        throw e;
    }
};

SsqlOlapRemover.prototype.remove = function SsqlOlapRemover_remove() {
    this.logger.log(PDEVENT, sprintf("begin to remove sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
    try {
        this._connectToHost();
        this._connectToHostBySdb();
        this._stopProcesses();
        this._uninstallPackage();
        this._removeData();
    } catch(e) {
        throw e;
    } finally {
        this._disconnectFromHost();
    }
    this.logger.log(PDEVENT, sprintf("finsh removing sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
};

SsqlOlapRemover.prototype.finalize = function SsqlOlapRemover_finalize() {
    return this.retVal;
};

try {
    var remover = new SsqlOlapRemover(BUS_JSON, SYS_JSON);
    remover.init();
    remover.remove();
    remover.finalize();
} catch(e) {
    var error = new SdbError(e);
    remover.logger.log(PDERROR, error);
    remover.logger.log(PDERROR, sprintf("failed to remove sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
    throw error;
}
