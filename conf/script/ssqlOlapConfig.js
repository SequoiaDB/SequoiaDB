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
@description: SequoiaSQL OLAP configuration
@author:
    2016-5-30 David Li  Init
*/

var SSQL_OLAP_CONFIG_FILE_NAME = "ssqlOlapConfig.js";

var SSQL_OLAP_CONFIG_COMMENT = "comment";

var SsqlOlapConfig = function() {
};

SsqlOlapConfig.prototype.header = '\
<?xml version="1.0" encoding="UTF-8"?>\n\
\n\
<!--\n\
Licensed to the Apache Software Foundation (ASF) under one\n\
or more contributor license agreements.  See the NOTICE file\n\
distributed with this work for additional information\n\
regarding copyright ownership.  The ASF licenses this file\n\
to you under the Apache License, Version 2.0 (the\n\
"License"); you may not use this file except in compliance\n\
with the License.  You may obtain a copy of the License at\n\
\n\
  http://www.apache.org/licenses/LICENSE-2.0\n\
\n\
Unless required by applicable law or agreed to in writing,\n\
software distributed under the License is distributed on an\n\
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY\n\
KIND, either express or implied.  See the License for the\n\
specific language governing permissions and limitations\n\
under the License.\n\
-->\n\
'

SsqlOlapConfig.prototype.configuration = [
    {
        "name": "hawq_master_address_host",
        "value": "localhost",
        "description": "The host name of hawq master."
    },
    {
        "name": "hawq_master_address_port",
        "value": "5432",
        "description": "The port of hawq master."
    },
    {
        "name": "hawq_standby_address_host",
        "value": "none",
        "description": "The host name of hawq standby master."
    },
    {
        "name": "hawq_segment_address_port",
        "value": "40000",
        "description": "The port of hawq segment."
    },
    {
        "name": "hawq_dfs_url",
        "value": "localhost:8020/hawq_default",
        "description": "URL for accessing HDFS."
    },
    {
        "name": "hawq_master_directory",
        "value": "~/sequoiasql-data-directory/masterdd",
        "description": "The directory of hawq master."
    },
    {
        "name": "hawq_segment_directory",
        "value": "~/sequoiasql-data-directory/segmentdd",
        "description": "The directory of hawq segment."
    },
    {
        "name": "hawq_master_temp_directory",
        "value": "/tmp",
        "description": "The temporary directory reserved for hawq master."
    },
    {
        "name": "hawq_segment_temp_directory",
        "value": "/tmp",
        "description": "The temporary directory reserved for hawq segment."
    },
    {
        "name": SSQL_OLAP_CONFIG_COMMENT,
        "value": "<!-- HAWQ resource manager parameters -->",
        "description": ""
    },
    {
        "name": "hawq_global_rm_type",
        "value": "none",
        "description": "The resource manager type to start for allocating resource.\n\
                                         'none' means hawq resource manager exclusively uses whole\n\
                                         cluster; 'yarn' means hawq resource manager contacts YARN\n\
                                         resource manager to negotiate resource."
    },
    {
        "name": "hawq_rm_memory_limit_perseg",
        "value": "64GB",
        "description": "The limit of memory usage in a hawq segment when\n\
                                         hawq_global_rm_type is set 'none'."
    },
    {
        "name": "hawq_rm_nvcore_limit_perseg",
        "value": "16",
        "description": "The limit of virtual core usage in a hawq segment when\n\
                                         hawq_global_rm_type is set 'none'."
    },
    {
        "name": "hawq_rm_yarn_address",
        "value": "localhost:8032",
        "description": "The address of YARN resource manager server."
    },
    {
        "name": "hawq_rm_yarn_scheduler_address",
        "value": "localhost:8030",
        "description": "The address of YARN scheduler server."
    },
    {
        "name": "hawq_rm_yarn_queue_name",
        "value": "default",
        "description": "The YARN queue name to register hawq resource manager."
    },
    {
        "name": "hawq_rm_yarn_app_name",
        "value": "hawq",
        "description": "The application name to register hawq resource manager in YARN."
    },
    {
        "name": SSQL_OLAP_CONFIG_COMMENT,
        "value": "<!-- HAWQ resource manager parameters end here. -->",
        "description": ""
    },
    {
        "name": SSQL_OLAP_CONFIG_COMMENT,
        "value": "<!-- HAWQ resource enforcement parameters -->",
        "description": ""
    },
    {
        "name": "hawq_re_cpu_enable",
        "value": "false",
        "description": "The control to enable/disable CPU resource enforcement."
    },
    {
        "name": "hawq_re_cgroup_mount_point",
        "value": "/sys/fs/cgroup",
        "description": "The mount point of CGroup file system for resource enforcement.\n\
                                         For example, /sys/fs/cgroup/cpu/hawq for CPU sub-system."
    },
    {
        "name": "hawq_re_cgroup_hierarchy_name",
        "value": "hawq",
        "description": "The name of the hierarchy to accomodate CGroup directories/files for resource enforcement.\n\
                                         For example, /sys/fs/cgroup/cpu/hawq for CPU sub-system."
    },
    {
        "name": SSQL_OLAP_CONFIG_COMMENT,
        "value": "<!-- HAWQ resource enforcement parameters end here. -->",
        "description": ""
    }
];

SsqlOlapConfig.prototype.updateProperty = function SsqlOlapConfig_updateProperty(name, value) {
    if (!isNotNullString(name)) {
        throw new SdbError(SDB_INVALIDARG, "invalid property name: " + name);
    }

    if (!isString(value)) {
        throw new SdbError(SDB_INVALIDARG, "invalid property value: " + value);
    }

    for (var i = 0; i < this.configuration.length; i++) {
        var property = this.configuration[i];
        if (property.name == name) {
            property.value = value;
        }
    }
};

SsqlOlapConfig.prototype.genXmlConfig = function SsqlOlapConfig_genXmlConfig(fileName) {
    if (!isNotNullString(fileName)) {
        throw new SdbError(SDB_INVALIDARG, "invalid file name: " + fileName);
    }

    if (File.exist(fileName)) {
        File.remove(fileName);
    }

    var file = new File(fileName);
    file.write(this.header);
    file.write("\n");
    file.write("<configuration>\n");
    for (var i = 0; i < this.configuration.length; i++) {
        var property = this.configuration[i];
        if (property.name == SSQL_OLAP_CONFIG_COMMENT) {
            file.write("\t");
            file.write(property.value);
            file.write("\n");
            file.write("\n");
        } else {
            file.write("\t");
            file.write("<property>");
            file.write("\n");

            file.write("\t\t");
            file.write("<name>");
            file.write(property.name);
            file.write("</name>");
            file.write("\n");

            file.write("\t\t");
            file.write("<value>");
            file.write(property.value);
            file.write("</value>");
            file.write("\n");

            file.write("\t\t");
            file.write("<description>");
            file.write(property.description);
            if (property.description.length > 80) {
                file.write("\n\t\t");
            }
            file.write("</description>");
            file.write("\n");

            file.write("\t");
            file.write("</property>");
            file.write("\n");

            file.write("\n");
        }
    }
    file.write("</configuration>\n");
};
