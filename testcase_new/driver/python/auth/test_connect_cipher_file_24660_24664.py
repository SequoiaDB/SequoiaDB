# -*- coding: utf-8 -*-
# @decription: seqDB-24660:Python驱动通过密码文件连接sdb
#            : seqDB-24664:python驱动使用密码文件和token执行client
# @author:     liuli
# @createTime: 2021.11.18

import os
import unittest
from lib import sdbconfig
from lib import testlib
from pysequoiadb.error import SDBBaseError
from pysequoiadb import client

username = "sdbadmin"
password = "sdbadmin"
token = sdbconfig.sdb_config.token
host = sdbconfig.sdb_config.host_name
service = sdbconfig.sdb_config.service

class TestConnectWithCipherFile24660(testlib.SdbTestBase):
    def setUp(self):
        if testlib.is_standalone():
            self.skipTest("run mode is standalone")
        try:
            self.db.create_user(username, password)
        except SDBBaseError as e:
            if -300 != e.code:
                raise e

    def test_connect_with_cipher_file_24660_24664(self):
        passwd_file = self.writeFile()
        csName = "cs_24660"
        # 使用正确的密码文件进行连接
        sdb1 = client(host, service, username, cipher_file=passwd_file[0])
        sdb1.create_collection_space(csName)
        
        sdb1.remove_user(username, password)
        sdb1.create_user(username, token)
        # 使用错误的密码文件进行连接
        try:
            client(host, service, username, cipher_file=passwd_file[0])
            self.fail('should error but success')
        except SDBBaseError as e:
            if -179 != e.code:
                raise e
        sdb1.remove_user(username, token)
        sdb1.create_user(username, password)
        sdb1.disconnect()

        # 使用不存在的密码文件
        try:
            client(host, service, username, cipher_file='test_24660')
            self.fail('should error but success')
        except SDBBaseError as e:
            if -4 != e.code:
                raise e

        # 指定错误的密码文件和正确的password
        sdb2 = client(host, service, username,
            psw=password, cipher_file=passwd_file[1])
        sdb2.disconnect()

        # 指定不存在的密码文件和正确的password
        sdb3 = client(host, service, username,
               psw=password, cipher_file = './test')
        sdb3.disconnect()
        
        # 指定正确的密码文件和错误的password
        try:
            client(host, service, username, psw="password", cipher_file=passwd_file[0])
            self.fail('should error but success')
        except SDBBaseError as e:
            if -179 != e.code:
                raise e

        # 指定正确的密码文件，正确的token进行连接
        sdb4 = client(host, service, username, cipher_file=passwd_file[1], token=token)
        sdb4.drop_collection_space(csName)
        sdb4.disconnect()

        # 指定正确的密码文件，错误的token进行连接
        try:
            client(host, service, username, cipher_file=passwd_file[1], token="token")
            self.fail('should error but success')
        except SDBBaseError as e:
            if -179 != e.code:
                raise e

        # 指定密码文件，不指定token进行连接
        try:
            client(host, service, username, cipher_file=passwd_file[1])
            self.fail('should error but success')
        except SDBBaseError as e:
            if -179 != e.code:
                raise e

        # 指定正确的密码文件，错误的token和正确的psw
        sdb5 = client(host, service, username, psw=password, cipher_file=passwd_file[1],
                      token="token")
        sdb5.disconnect()

        # 指定正确的密码文件，正确的token和正确的psw
        sdb6 = client(host, service, username, psw=password, cipher_file=passwd_file[1], token=token)
        sdb6.disconnect()

        # 指定正确的密码文件，正确的token和错误的psw
        try:
            client(host, service, username, psw="password", cipher_file=passwd_file[0], token=token)
            self.fail('should error but success')
        except SDBBaseError as e:
            if -179 != e.code:
                raise e

        sdb = client(host, service, username, password)
        # connect使用密码文件连接
        sdb.connect(host, service, user=username, cipher_file=passwd_file[0])
        # connect使用密码文件和token连接
        sdb.connect(host, service, user=username, cipher_file=passwd_file[1], token=token)

        host_list = [{'host':host, 'service':service}]
        # connect_to_hosts使用密码文件连接
        sdb.connect_to_hosts(host_list, user=username, cipher_file=passwd_file[0])
        # connect_to_hosts使用密码文件和token连接
        sdb.connect_to_hosts(host_list, user=username, cipher_file=passwd_file[1], token=token)
        sdb.disconnect()

    def tearDown(self):
        passwd_file = ['passwd1', 'passwd2']
        sdb = client(host, service, username, password)
        try:
            sdb.remove_user(username, password)
        except SDBBaseError as e:
            if -300 != e.code:
                raise e
        finally:
            sdb.disconnect()
            self.removeFile(passwd_file)

    def writeFile(self):
        passwd = []
        with open('passwd1', 'w') as file:
            file.write(sdbconfig.sdb_config.passwd + '\n')
        file.close()
        with open('passwd2', 'w') as file:
            file.write(sdbconfig.sdb_config.passwd_token + '\n')
        file.close()
        passwd.append('passwd1')
        passwd.append('passwd2')
        return passwd

    def removeFile(self, files):
        for file in files:
            if os.path.exists(file):
                os.remove(file)