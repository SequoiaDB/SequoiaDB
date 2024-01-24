# -*- coding: utf-8 -*-
# @decription: seqDB-24661:Python驱动client host_list连接策略
# @author:     liuli
# @createTime: 2021.11.18

import unittest
import json
from lib import sdbconfig
from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)
from pysequoiadb import client
from pysequoiadb.client import SDB_SNAP_HEALTH

class TestConnectWithCipherFile24660(testlib.SdbTestBase):
    def setUp(self):
        if(len(self.getHost(self.db)) < 2):
            self.skipTest("coord less than 2")

    def test_connect_with_cipher_file_24660_24664(self):
        cs_name = 'cs_24611'
        host_list = self.getHost(self.db)

        # 指定host_list，指定一个地址进行连接
        sdb = client(host_list=[host_list[0]])
        # 检查连接可用
        sdb.create_collection_space(cs_name)
        sdb.disconnect()

        # 指定host_list，指定多个地址进行连接，policy使用默认值
        sdb = client(host_list=host_list)
        sdb.drop_collection_space(cs_name)
        sdb.disconnect()

        # 指定host_list，指定多个地址进行连接，policy使用random
        sdb = client(host_list=host_list, policy='random')
        sdb.disconnect()

        # 指定host_list，指定多个地址进行连接，policy使用local_first
        sdb = client(host_list=host_list, policy='local_first')
        # 打印节点信息，查看优先连接本地节点
        clientStr = repr(sdb)
        nodeurl = clientStr.split()[-1]
        print('Connect using local address:' + nodeurl)
        sdb.disconnect()

        # 指定host_list，指定多个地址进行连接，policy使用one_by_one
        sdb = client(host_list=host_list, policy='one_by_one')
        # 检查优先连接第一个节点
        clientStr = repr(sdb)
        nodeurl = clientStr.split()[-1].split(':')
        assert nodeurl[0] == host_list[0]['host'], 'expect host:' + nodeurl[0] + ', actual host:' + host_list[0]['host']
        assert nodeurl[1] == host_list[0]['service'], 'expect service:' + nodeurl[1] + ', actual service:' + \
                                                      host_list[0]['service']
        sdb.disconnect()

        # 构造host_list2其中第一个地址不可用
        host_list2 = []
        host_list2.append({'host':'nonehost', 'service':"11810"})
        host_list2.extend(host_list)

        # 使用host_list2连接，policy使用默认值
        sdb = client(host_list=host_list2)
        sdb.disconnect()

        # 使用host_list2连接，policy使用random
        sdb = client(host_list=host_list2, policy='random')
        sdb.disconnect()

        # 使用host_list2连接，policy使用local_first
        sdb = client(host_list=host_list2, policy='local_first')
        # 打印节点信息，查看优先连接本地节点
        clientStr = repr(sdb)
        nodeurl = clientStr.split()[-1]
        print('Connect using local address:' + nodeurl)
        sdb.disconnect()

        # 使用host_list2连接，policy使用one_by_one
        sdb = client(host_list=host_list2, policy='one_by_one')
        # 第一个地址异常，检查使用第二个地址进行连接
        clientStr = repr(sdb)
        nodeurl = clientStr.split()[-1].split(':')
        assert nodeurl[0] == host_list2[1]['host'], 'expect host:' + nodeurl[0] + ', actual host:' + host_list2[1][
            'host']
        assert nodeurl[1] == host_list2[1]['service'], 'expect host:' + nodeurl[1] + ', actual host:' + host_list2[1][
            'service']
        sdb.disconnect()

        # 同时指定host_list和host，host_list中不包含host
        host = host_list[0]['host']
        service = host_list[0]['service']
        sdb = client(host=host, service=service, host_list=host_list[1:])
        # 检查连接地址为host指定地址
        clientStr = repr(sdb)
        nodeurl = clientStr.split()[-1].split(':')
        assert nodeurl[0] == host, 'expect host:' + nodeurl[0] + ', actual host:' + host
        assert nodeurl[1] == service, 'expect host:' + nodeurl[1] + ', actual host:' + service
        sdb.disconnect()

        # 同时指定host_list和host，host_list中包含host，host指定值位于host_list非第一位，policy指定one_by_one
        host = host_list[1]['host']
        service = host_list[1]['service']
        sdb = client(host=host, service=service, host_list=host_list, policy='one_by_one')
        # 检查连接地址为host指定地址
        clientStr = repr(sdb)
        nodeurl = clientStr.split()[-1].split(':')
        assert nodeurl[0] == host, 'expect host:' + nodeurl[0] + ', actual host:' + host
        assert nodeurl[1] == service, 'expect host:' + nodeurl[1] + ', actual host:' + service
        sdb.disconnect()

        # 同时指定host_list和host，host_list中包含host，policy指定local_first
        host = host_list[0]['host']
        service = host_list[0]['service']
        sdb = client(host=host, service=service, host_list=host_list, policy='local_first')
        # 检查连接地址为host指定地址
        clientStr = repr(sdb)
        nodeurl = clientStr.split()[-1].split(':')
        assert nodeurl[0] == host, 'expect host:' + nodeurl[0] + ', actual host:' + host
        assert nodeurl[1] == service, 'expect host:' + nodeurl[1] + ', actual host:' + service
        sdb.disconnect()

        # 同时指定host_list和host，host_list中不包含host，host_list全部地址可用，host指定地址不可用
        host = 'nonehost'
        service = '11810'
        try:
            client(host=host, service=service, host_list=host_list)
        except SDBBaseError as e:
            if -15 != e.code:
                raise e

    # 获取所有的coord地址
    def getHost(self, sdb):
        hosts = []
        cursor = sdb.get_snapshot(SDB_SNAP_HEALTH, condition = {'Role': 'coord'}, selector = {'NodeName': 1})
        while True:
            try:
                rec = cursor.next()['NodeName']
                nodeInfo = rec.split(':')
                host = {'host':nodeInfo[0], 'service':nodeInfo[1]}
                hosts.append(host)
            except SDBEndOfCursor:
                break
        return hosts