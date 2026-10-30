#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""为本次 Python 执行的 JUnit 报告补充 Jenkins 分类及失败诊断信息。"""
from __future__ import print_function
import argparse
import glob
import os
import socket
import sys
import xml.etree.ElementTree as ET


def format_reports(report_dir, test_type, reportinfo):
    """仅处理本次报告目录中的测试 XML，不改变测试结果。"""
    info = dict([part.strip() for part in item.split('=', 1)] for item in reportinfo.split(';') if '=' in item)
    if not info.get('execute_host'):
        host = socket.gethostname()
        try:
            info['execute_host'] = '%s/%s' % (host, socket.gethostbyname(host))
        except socket.error:
            info['execute_host'] = host
    cluster = (info.get('cluster_hosts') or os.environ.get('TESTHOST_LIST', '')).strip()
    # 诊断地址优先用 reportinfo，其次环境变量；仍缺少时按构建信息拼接。
    url = info.get('diaglog_url') or os.environ.get('DIAGLOG_URL', '')
    if not url and all(info.get(key) for key in ('test_type', 'deploy_mode', 'build_number')):
        url = '%s/%s_%s_%s.tar.gz' % ((info.get('diaglog_base') or 'http://192.168.28.27:8081').rstrip('/'),
                                     info['test_type'], info['deploy_mode'], info['build_number'])
    head = '\nEXECUTE HOST: %s\n\n' % info['execute_host'] + ('CLUSTER HOST: %s\n\n' % cluster if cluster else '')
    tail = '\nDOWNLOAD DIAGLOG :\n%s\n' % url if url else ''
    # 与原 CI 报告改写一致：classname 整体替换为 CI 类型，driver 再加 Python 主版本；name 保留方法名。
    label = {'driver': 'driver_python.python%d' % sys.version_info[0],
             's3': 'thirdparty_s3_python.thirdparty_s3_python',
             'fapmongo': 'thirdparty_fapmongo_python.thirdparty_fapmongo_python'}[test_type]
    for path in sorted(glob.glob(os.path.join(report_dir, 'TEST-*.xml')) + glob.glob(os.path.join(report_dir, 'configure_*.xml'))):
        tree = ET.parse(path)
        for case in tree.findall('.//testcase'):
            if case.get('classname') == label:
                continue
            case.set('classname', label)
            for tag in ('failure', 'error'):
                for node in case.findall(tag):
                    node.text = head + (node.text or '') + tail
        tree.write(path, encoding='utf-8', xml_declaration=True)


def main(argv=None):
    """支持单独格式化已生成的报告，也可由正式执行入口调用。"""
    parser = argparse.ArgumentParser(description='Format one run of Python JUnit reports')
    parser.add_argument('--report-dir', required=True)
    parser.add_argument('--type', required=True, choices=('driver', 's3', 'fapmongo'))
    parser.add_argument('--reportinfo', default='')
    args = parser.parse_args(argv)
    format_reports(args.report_dir, args.type, args.reportinfo)
    return 0


if __name__ == '__main__':
    sys.exit(main())
