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
@description: establish trust between SequoiaSQL OLAP hosts
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

var SSQL_OLAP_TRUST_FILE_NAME = "ssqlOlapTrust.js";

var SsqlOlapTruster = function(config, sysInfo) {
    this.config = config;
    this.sysInfo = sysInfo;
    this.base = new SsqlOlapCommon();
    this.logger = new Logger(SSQL_OLAP_TRUST_FILE_NAME);
    this.retVal = new SsqlOlapReturnValue();
};

SsqlOlapTruster.prototype._checkConfig = function SsqlOlapTruster__checkConfig (config) {
    try {
        this.base.checkConfig(config);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapTruster.prototype._checkSysInfo = function SsqlOlapTruster__checkSysInfo(sysInfo) {
    try {
        this.base.checkSysInfo(sysInfo);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapTruster.prototype.init = function SsqlOlapTruster_init() {
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

    if (this.config[HostName] != this.config[MasterHost]) {
        var msg = sprintf("establish trust should be in master host [?], but actually [?]", this.config[MasterHost], this.config[HostName]);
        var error = new SdbError(SDB_INVALIDARG, msg);
        this.logger.log(PDERROR, error);
        throw error;
    }

    this.retVal[HostName] = this.config[HostName];
    this.retVal[Role] = this.config[Role];
    this.retVal[TaskID] = this.sysInfo[TaskID];
};

SsqlOlapTruster.prototype._connectToHost = function SsqlOlapTruster__connectToHost() {
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

SsqlOlapTruster.prototype._disconnectFromHost = function SsqlOlapTruster__disconnectFromHost() {
    if (this.sdbSsh != undefined) {
        try { this.sdbSsh.close(); } catch(e) {}
        this.sdbSsh = null;
    }
};

SsqlOlapTruster.prototype._getAllHosts = function SsqlOlapTruster__getAllHosts() {
    var hosts = new Array();
    var map = new Object(); // use Object as map

    hosts.push(this.config[MasterHost]);
    map[this.config[MasterHost]] = 1;
    if (this.config[StandbyHost] != "" && this.config[StandbyHost] != "none") {
        hosts.push(this.config[StandbyHost]);
        map[this.config[StandbyHost]] = 1;
    }

    var segmentHosts = this.config[SegmentHosts];
    for (var i in segmentHosts) {
        var host = segmentHosts[i];
        if (map[host] == undefined) {
            hosts.push(host);
            map[host] = 1;
        }
    }

    return hosts;
};

SsqlOlapTruster.prototype._establishTrust = function SsqlOlapTruster__establishTrust() {
    var role = this.config[Role];
    if (role != Master) {
        var error = new SdbError(SDB_SYS, sprintf("establish trust should be in master host, but given ?", role));
        this.logger.log(PDERROR, error);
        throw error;
    }

    var hostName = this.config[HostName];
    var installDir = this.config[InstallDir];

    if (!this.sdbSsh.isPathExist(installDir)) {
        var error = new SdbError(SDB_SYS, sprintf("the directory [?] does not exist in host [?]", installDir, hostName));
        this.logger.log(PDERROR, error);
        throw error;
    }

    var binDir = adaptPath(installDir) + "bin";
    if (!this.sdbSsh.isPathExist(binDir)) {
        var error = new SdbError(SDB_SYS, sprintf("the directory [?] does not exist in host [?]", binDir, hostName));
        this.logger.log(PDERROR, error);
        throw error;
    }

    var env = adaptPath(installDir) + "sequoiasql_path.sh";
    var ssql = adaptPath(binDir) + "ssql";
    if (!this.sdbSsh.isPathExist(ssql)) {
        var error = new SdbError(SDB_SYS, sprintf("the file [?] does not exist in host [?]", ssql, hostName));
        this.logger.log(PDERROR, error);
        throw error;
    }
    if (!this.sdbSsh.isFile(ssql)) {
        var error = new SdbError(SDB_SYS, sprintf("the path [?] is not file in host [?]", ssql, hostName));
        this.logger.log(PDERROR, error);
        throw error;
    }

    var shell = "source " + env + "; ";
    shell += ssql + " ssh-exkeys ";
    var hosts = this._getAllHosts();
    for (var i in hosts) {
        shell += " -h " + hosts[i];
    }
    shell += " -p " + this.sysInfo[SdbPasswd];
    shell += " -v";
    this.logger.log(PDDEBUG, "establish trust cmd: " + shell);

    try {
        this.sdbSsh.exec(shell);
    } catch(e) {
    }

    if (this.sdbSsh.getLastRet() != 0) {
        var msg = this.sdbSsh.getLastOut();
        var error = new SdbError(SDB_SYS, msg);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapTruster.prototype.trust = function SsqlOlapTruster_trust() {
    this.logger.log(PDEVENT, "begin to establish trust for sequoiasql olap");
    try {
        this._connectToHost();
        this._establishTrust();
    } catch(e) {
        throw e;
    } finally {
        this._disconnectFromHost();
    }
    this.logger.log(PDEVENT, "finsh establishing trust for sequoiasql olap");
};

SsqlOlapTruster.prototype.finalize = function SsqlOlapTruster_finalize() {
    return this.retVal;
};

try {
    var truster = new SsqlOlapTruster(BUS_JSON, SYS_JSON);
    truster.init();
    truster.trust();
    truster.finalize();
} catch(e) {
    var error = new SdbError(e);
    truster.logger.log(PDERROR, error);
    truster.logger.log(PDERROR, "failed to establish trust for sequoiasql olap");
    throw error;
}