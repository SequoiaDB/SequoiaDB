#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import time
import subprocess

# Parent directory for sub config directories of all nodes.
# Inside this directory, there should be sub-directories like
# 11830, 11840, etc.
cfg_base_path = "conf/seadapter"  # type: str


def get_node_cfg_paths():
    global cfg_base_path
    if not os.path.isabs(cfg_base_path):
        script_path = os.path.dirname(os.path.abspath(__file__))
        cfg_base_path = os.path.join(script_path, cfg_base_path)
    print("Config base path: " + cfg_base_path)
    if "" == cfg_base_path:
        print("Error: Adapter configuration path is not set. Please set it in "
              "this monitor")
        exit(1)
    # List all sub directories for
    node_cfg_paths = os.listdir(cfg_base_path)
    return node_cfg_paths


def get_exe_path():
    sdb_default_file = "/etc/default/sequoiadb"
    if not os.path.exists(sdb_default_file):
        print("Error: " + sdb_default_file + " doesn't exist. Maybe SequoiaDB "
              "is not installed.")
        exit(2)
    f = open(sdb_default_file, "r")
    lines = f.readlines()
    for line in lines:
        if "INSTALL_DIR" in line:
            break
    f.close()
    path = line.strip().split('=')[1]
    return os.path.join(path, "bin", "sdbseadapter")


def main():
    cfg_file_name = "sdbseadapter.conf"
    node_configs = get_node_cfg_paths()
    exe_path = get_exe_path()
    abs_cfg_base_path = os.path.abspath(cfg_base_path)
    for node_config in node_configs:
        abs_node_config = os.path.join(abs_cfg_base_path, node_config)
        cfg_file = os.path.join(abs_node_config, cfg_file_name)
        if not os.path.exists(cfg_file):
            print("Configuration file dose not exist for " + node_config +
                  ". Skip.")
            continue
        command = [exe_path, '-c', abs_node_config]
        adpt_process = subprocess.Popen(command, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
        # Wait for one second for the subprocess to startup. Otherwise the
        # invoke of poll will return None below.
        time.sleep(1)
        if adpt_process.poll() is not None:
            out, error = adpt_process.communicate()
            if 0 != adpt_process.returncode:
                if error:
                    print("Failed to adapter for " + node_config + ". Error: " +
                          error)
                else:
                    print("The adapter for " + node_config + " is running")
        else:
            print("Start adapter for " + node_config + " successfully")


if __name__ == "__main__":
    main()
