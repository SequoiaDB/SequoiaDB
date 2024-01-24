/*******************************************************************************

   Copyright (C) 2012-2018 SequoiaDB Ltd.

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
@description: init SequoiaSQL OLAP cluster
@author:
    2016-6-8 David Li  Init
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

var SSQL_OLAP_INIT_FILE_NAME = "ssqlOlapInit.js";

var SsqlOlapInitializer = function(config, sysInfo) {
    this.config = config;
    this.sysInfo = sysInfo;
    this.base = new SsqlOlapCommon();
    this.logger = new Logger(SSQL_OLAP_INIT_FILE_NAME);
    this.retVal = new SsqlOlapReturnValue();
};

SsqlOlapInitializer.prototype._checkConfig = function SsqlOlapInitializer__checkConfig (config) {
    try {
        this.base.checkConfig(config);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapInitializer.prototype._checkSysInfo = function SsqlOlapInitializer__checkSysInfo(sysInfo) {
    try {
        this.base.checkSysInfo(sysInfo);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapInitializer.prototype.init = function SsqlOlapInitializer_init() {
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

SsqlOlapInitializer.prototype._connectToHost = function SsqlOlapInitializer__connectToHost() {
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

SsqlOlapInitializer.prototype._disconnectFromHost = function SsqlOlapInitializer__disconnectFromHost() {
    if (this.sdbSsh != undefined) {
        try { this.sdbSsh.close(); } catch(e) {}
        this.sdbSsh = null;
    }
};

SsqlOlapInitializer.prototype._initNode = function SsqlOlapInitializer__initNode(role) {
    if (role != Master && role != Standby && role != Segment && role != Cluster) {
        var error = new SdbError(SDB_INVALIDARG, sprintf("invalid role: ?", role));
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
    
    var logDir = this.config[LogDir];
    var maxConnections = this.config[MaxConnections];
    var sharedBuffers = this.config[SharedBuffers];

    var shell = "source " + env + "; ";
    shell += ssql + " init " + role;
    shell += " --logdir=" + logDir;
    shell += " --max_connections=" + maxConnections;
    shell += " --shared_buffers=" + sharedBuffers + "MB";
    shell += " -a -v";
    this.logger.log(PDDEBUG, "init cmd: " + shell);

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

SsqlOlapInitializer.prototype._initCluster = function SsqlOlapInitializer__initCluster() {
    try {
        this._initNode(Cluster);
    } catch(e) {
        var error = new SdbError(e, sprintf("failed to init cluster"));
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapInitializer.prototype._initMaster = function SsqlOlapInitializer__initMaster() {
    try {
        this._initNode(Master);
    } catch(e) {
        var error = new SdbError(e, sprintf("failed to init master"));
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapInitializer.prototype._initStandby = function SsqlOlapInitializer__initStandby() {
    if (this.config[StandbyHost] == "" || this.config[StandbyHost] == "none") {
        this.logger.log(PDDEBUG, "no standby, no need to init");
        return;
    }

    try {
        this._initNode(Standby);
    } catch(e) {
        var error = new SdbError(e, sprintf("failed to init standby"));
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapInitializer.prototype._initSegments = function SsqlOlapInitializer__initSegments() {
    try {
        this._initNode(Segment);
    } catch(e) {
        var error = new SdbError(e, sprintf("failed to init segments"));
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapInitializer.prototype.initCluster = function SsqlOlapInitializer_initCluster() {
    var role = this.config[Role];
    if (role != Master) {
        var error = new SdbError(SDB_SYS, sprintf("init cluster should be in master host, but given ?", role));
        this.logger.log(PDERROR, error);
        throw error;
    }

    this.logger.log(PDEVENT, "begin to init cluster for sequoiasql olap");
    try {
        this._connectToHost();
        //this._initMaster();
        //this._initStandby();
        //this._initSegments();
        this._initCluster();
    } catch(e) {
        throw e;
    } finally {
        this._disconnectFromHost();
    }
    this.logger.log(PDEVENT, "finsh initializing cluster for sequoiasql olap");
};

SsqlOlapInitializer.prototype.finalize = function SsqlOlapInitializer_finalize() {
    return this.retVal;
};

try {
    var initer = new SsqlOlapInitializer(BUS_JSON, SYS_JSON);
    initer.init();
    initer.initCluster();
    initer.finalize();
} catch(e) {
    var error = new SdbError(e);
    truster.logger.log(PDERROR, error);
    truster.logger.log(PDERROR, "failed to init cluster for sequoiasql olap");
    throw error;
}