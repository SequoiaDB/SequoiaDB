<?php
// 为 PHP 运行生成的 JUnit 报告补充 Jenkins 类名及失败信息，并通过 php_runner.php 的 --reportFormat jenkins 触发调用。
// --reportinfo 可传递以下键值：
//   execute_host                          -> EXECUTE HOST，默认取本机 主机名/IP
//   cluster_hosts                         -> CLUSTER HOST，缺省取环境变量 TESTHOST_LIST
//   diaglog_url                           -> DOWNLOAD DIAGLOG，缺省取环境变量 DIAGLOG_URL
//   test_type, deploy_mode, build_number  -> 未传 diaglog_url 时拼装 <diaglog_base>/<type>_<mode>_<number>.tar.gz
//   diaglog_base                          -> 默认 http://192.168.28.27:8081

// 报告格式与原 CI 保持一致：classname 为 <type>.<suite>_<runtime>，name 为 类名.方法名，failure 前后附带日志信息。
function php_jenkins_format($document, $label, $metadata)
{
    $xpath = new DOMXPath($document);
    // 移除所有包含 <skipped> 的 <testcase> 节点，对齐老 CI 报告只展示实际执行用例
    foreach ($xpath->query('//testcase[skipped]') as $node) {
        $node->parentNode->removeChild($node);
    }
    foreach ($xpath->query('//testsuite') as $suiteNode) {
        $testcases = $xpath->query('.//testcase', $suiteNode);
        $failures = $xpath->query('.//testcase[failure]', $suiteNode);
        $errors = $xpath->query('.//testcase[error]', $suiteNode);
        $suiteNode->setAttribute('tests', (string)$testcases->length);
        $suiteNode->setAttribute('failures', (string)$failures->length);
        $suiteNode->setAttribute('errors', (string)$errors->length);
        $suiteNode->setAttribute('skipped', '0');
    }
    foreach ($xpath->query('//testcase') as $node) {
        $class = $node->getAttribute('classname') ?: $node->getAttribute('class');
        $node->setAttribute('classname', $label);
        $node->setAttribute('name', ($class === '' ? '' : $class . '.') . $node->getAttribute('name'));
    }
    $info = $metadata + array('diaglog_base' => 'http://192.168.28.27:8081',
        'cluster_hosts' => (string)getenv('TESTHOST_LIST'), 'diaglog_url' => (string)getenv('DIAGLOG_URL'));
    if (!isset($info['execute_host'])) {
        $host = gethostname();
        $info['execute_host'] = $host . '/' . gethostbyname($host);
    }
    if ($info['diaglog_url'] === '' && !empty($info['test_type']) && !empty($info['deploy_mode'])
            && !empty($info['build_number'])) {
        $info['diaglog_url'] = rtrim($info['diaglog_base'], '/') . '/' . $info['test_type'] . '_'
            . $info['deploy_mode'] . '_' . $info['build_number'] . '.tar.gz';
    }
    $head = "\nEXECUTE HOST: " . $info['execute_host'] . "\n\n";
    $head .= $info['cluster_hosts'] !== '' ? 'CLUSTER HOST: ' . $info['cluster_hosts'] . "\n\n" : '';
    $tail = $info['diaglog_url'] !== '' ? "\nDOWNLOAD DIAGLOG :\n" . $info['diaglog_url'] . "\n" : '';
    foreach ($xpath->query('//failure') as $node) {
        $node->insertBefore($document->createTextNode($head), $node->firstChild);
        $node->appendChild($document->createTextNode($tail));
    }
}
