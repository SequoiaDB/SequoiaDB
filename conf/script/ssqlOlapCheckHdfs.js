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
@description: check HDFS for SequoiaSQL OLAP
@author:
    2016-6-7 David Li  Init
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

var SSQL_OLAP_CHECK_HDFS_FILE_NAME = "ssqlOlapCheckHdfs.js";

var SsqlOlapHdfsChecker = function(config, sysInfo) {
    this.config = config;
    this.sysInfo = sysInfo;
    this.base = new SsqlOlapCommon();
    this.logger = new Logger(SSQL_OLAP_CHECK_HDFS_FILE_NAME);
    this.retVal = new SsqlOlapReturnValue();
};

SsqlOlapHdfsChecker.prototype._checkConfig = function SsqlOlapHdfsChecker__checkConfig (config) {
    try {
        this.base.checkConfig(config);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapHdfsChecker.prototype._checkSysInfo = function SsqlOlapHdfsChecker__checkSysInfo(sysInfo) {
    try {
        this.base.checkSysInfo(sysInfo);
    } catch(e) {
        var error = new SdbError(e);
        this.logger.log(PDERROR, error);
        throw error;
    }
};

SsqlOlapHdfsChecker.prototype.init = function SsqlOlapHdfsChecker_init() {
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

SsqlOlapHdfsChecker.prototype._connectToHost = function SsqlOlapHdfsChecker__connectToHost() {
    var hostName = this.config[HostName];
    var sdbUser = this.sysInfo[SdbUser];
    var sdbPasswd = this.sysInfo[SdbPasswd];
    var sshPort = parseInt(this.config[SshPort]);
    var sdbSsh;

    try {
        sdbSsh = this.base.connectToHost(hostName, sdbUser, sdbPasswd, sshPort);
    } catch(e) {
        this.logger.log(PDERROR, e);
        throw e;
    }

    this.sdbSsh = sdbSsh;
};

SsqlOlapHdfsChecker.prototype._disconnectFromHost = function SsqlOlapHdfsChecker__disconnectFromHost() {
    if (this.sdbSsh != undefined) {
        try { this.sdbSsh.close(); } catch(e) {}
        this.sdbSsh = null;
    }
};

SsqlOlapHdfsChecker.prototype._checkHdfs = function SsqlOlapHdfsChecker__checkHdfs() {
    var hostName = this.config[HostName];
    var installDir = this.config[InstallDir];
    var hdfsUrl = this.config[HdfsUrl];

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
    var check = adaptPath(binDir) + "gpcheckhdfs";
    if (!this.sdbSsh.isPathExist(check)) {
        var error = new SdbError(SDB_SYS, sprintf("the file [?] does not exist in host [?]", check, hostName));
        this.logger.log(PDERROR, error);
        throw error;
    }
    if (!this.sdbSsh.isFile(check)) {
        var error = new SdbError(SDB_SYS, sprintf("the path [?] is not file in host [?]", check, hostName));
        this.logger.log(PDERROR, error);
        throw error;
    }

    var shell = "source " + env + "; ";
    shell += check + " hdfs ";
    shell += this.config[HdfsUrl];
    shell += " off";
    this.logger.log(PDDEBUG, "check hdfs cmd: " + shell);

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

SsqlOlapHdfsChecker.prototype.check = function SsqlOlapHdfsChecker_check() {
    this.logger.log(PDEVENT, sprintf("begin to check HDFS for sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
    try {
        this._connectToHost();
        this._checkHdfs();
    } catch(e) {
        throw e;
    } finally {
        this._disconnectFromHost();
    }
    this.logger.log(PDEVENT, sprintf("finsh checking HDFS for sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
};

SsqlOlapHdfsChecker.prototype.finalize = function SsqlOlapHdfsChecker_finalize() {
    return this.retVal;
};

try {
    var checker = new SsqlOlapHdfsChecker(BUS_JSON, SYS_JSON);
    checker.init();
    checker.check();
    checker.finalize();
} catch(e) {
    var error = new SdbError(e);
    checker.logger.log(PDERROR, error);
    checker.logger.log(PDERROR, sprintf("failed to check HDFS for sequoiasql olap[?:?]", this.config[HostName], this.config[Role]));
    throw error;
}