#-*- coding:utf-8 -*-
import json
import os
import os.path

class SdbConfig(object):
   def __init__(self):
      self.get_config()

   def get_config(self):
      config_file = os.path.join(os.getcwd(), "config.json")
      fp = open(config_file)
      configs = json.load(fp)
      #协调节点主机名，默认localhost
      self.host_name = configs['HOSTNAME']
      #协调节点端口号，默认11810
      self.service = configs['SVCNAME']
      #公共CS
      self.changed_prefix = configs['CHANGEDPREFIX']
      #用例创建节点预留端口号最小值
      self.rsrv_port_begin = configs['RSRVPORTBEGIN']
      #用例创建节点预留端口号最大值
      self.rsrv_port_end = configs['RSRVPORTEND']
      #用例创建节点存放节点数据目录
      self.rsrv_node_dir = configs['RSRVNODEDIR']
      #用例存放临时文件的目录
      self.work_dir = configs['WORKDIR']
      #失败是否停止用例开关
      self.break_on_failure = configs['BREAK_ON_FAILURE']
      #加密文件的字符串
      self.passwd = configs['PASSWORD']
      #使用token加密令牌生成的密码文件的字符串
      self.passwd_token = configs['PASSWORDTOKEN']
      #使用的token令牌
      self.token = configs['TOKEN']

      fp.close()

sdb_config = SdbConfig()

