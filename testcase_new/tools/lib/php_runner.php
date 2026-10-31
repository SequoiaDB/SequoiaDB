<?php
// PHP driver 用例执行器，由 run_php_case.sh 调用；PHPUnit 子进程也加载本文件以使用 PhpRunnerListener。
// 结构：参数与用例选择 -> JUnit 处理 -> 子进程 listener -> PhpTestRunner 执行与汇总 -> 入口。
// 退出码：0 通过，1 测试失败，2 参数/环境错误，3 没有用例；130/143 由 run_php_case.sh 处理。

class PhpRunnerException extends Exception
{
}

// 抛出带退出码的错误，由入口或 execute() 统一处理。
function runner_fail($message, $code = 2)
{
    throw new PhpRunnerException($message, $code);
}

// 相对路径按当前目录展开，不要求路径存在。
function runner_absolute($path)
{
    return $path !== '' && $path[0] === '/' ? rtrim($path, '/') : rtrim(getcwd() . '/' . $path, '/');
}

function runner_existing($path, $what)
{
    $real = realpath(runner_absolute($path));
    if ($real === false) {
        runner_fail($what . '不存在：' . $path);
    }
    return $real;
}

// $path 等于 $parent 或位于其下。
function runner_within($path, $parent)
{
    return $path === $parent || strpos($path . '/', rtrim($parent, '/') . '/') === 0;
}

// 读取 KEY=value 文件（--conf 与 testcase.conf），# 开头为注释。
function runner_kv_file($path)
{
    $text = @file_get_contents($path);
    if ($text === false) {
        runner_fail('无法读取：' . $path);
    }
    $values = array();
    foreach (preg_split('/\r\n|\r|\n/', $text) as $line) {
        $line = trim($line);
        if ($line === '' || $line[0] === '#') {
            continue;
        }
        if (!preg_match('/^([A-Za-z_]\w*)\s*=\s*(.*)$/', $line, $match)) {
            runner_fail('格式错误：' . $path . '：' . $line);
        }
        $values[$match[1]] = $match[2];
    }
    return $values;
}

// 文件缺失、为空或 XML 无效时返回 null。
function runner_load_xml($path)
{
    $text = @file_get_contents($path);
    $document = new DOMDocument('1.0', 'UTF-8');
    $previous = libxml_use_internal_errors(true);
    $loaded = $text !== false && trim($text) !== '' && $document->loadXML($text, LIBXML_NONET);
    libxml_clear_errors();
    libxml_use_internal_errors($previous);
    return $loaded ? $document : null;
}

// 解析命令行并合并连接参数，返回执行所需的全部选项及按 suite 分组的用例文件。
function runner_options($args, $repo)
{
    $options = array(
        'repo' => $repo, 'type' => 'driver', 'mode' => 'normal',
        'targets' => array(), 'target_option' => '', 'case_dir' => $repo . '/testcase_new/driver/php', 'report_dir' => '', 'workdir' => '',
        'php' => 'php', 'php_ini' => '', 'extension' => '', 'phpunit' => '', 'conf_file' => '',
        'install_dir' => '', 'deploy_mode' => '', 'stop' => false, 'print' => '', 'format' => '', 'metadata' => array(),
        'dry_run' => false
    );
    $valueOptions = array(
        '-t' => 'type', '-m' => 'mode', '--case-dir' => 'case_dir', '--report-dir' => 'report_dir',
        '--workdir' => 'workdir', '--php' => 'php', '--php-ini' => 'php_ini',
        '--extension' => 'extension', '--phpunit' => 'phpunit', '--conf' => 'conf_file',
        '--install-dir' => 'install_dir', '--reportFormat' => 'format',
        '-f' => '', '-p' => '', '-s' => '', '--reportinfo' => '', '--deploy-mode' => 'deploy_mode'
    );
    $connectionOptions = array(
        '-h' => 'HOSTNAME', '-n' => 'SVCNAME', '--changed-prefix' => 'CHANGEDPREFIX',
        '--reserve-port-begin' => 'RSRVPORTBEGIN', '--reserve-port-end' => 'RSRVPORTEND',
        '--db-path' => 'RSRVNODEDIR'
    );
    $cli = array();
    for ($index = 0; $index < count($args); $index++) {
        $argument = $args[$index];
        // -print 原样输出 PHPUnit 内容；-print failed 在逐行结果之外展开失败详情。
        if ($argument === '-print') {
            $options['print'] = 'full';
            if (isset($args[$index + 1]) && $args[$index + 1] === 'failed') {
                $options['print'] = 'failed';
                $index++;
            }
            continue;
        }
        if ($argument === '--dry-run') {
            $options['dry_run'] = true;
            continue;
        }
        if (!isset($valueOptions[$argument]) && !isset($connectionOptions[$argument])) {
            runner_fail('不支持的参数：' . $argument);
        }
        if (!isset($args[$index + 1]) || $args[$index + 1] === '' || $args[$index + 1][0] === '-') {
            runner_fail('参数缺少值：' . $argument);
        }
        $value = $args[++$index];
        if (isset($connectionOptions[$argument])) {
            $cli[$connectionOptions[$argument]] = $value;
        } elseif ($argument === '-f' || $argument === '-p') {
            if ($options['target_option'] !== '' && $options['target_option'] !== $argument) {
                runner_fail('-f 与 -p 不能同时使用');
            }
            if ($argument === '-p' && $options['targets']) {
                runner_fail('-p 只能指定一个目录');
            }
            $options['target_option'] = $argument;
            $options['targets'][] = preg_replace('#^(\./)+#', '', trim(str_replace('\\', '/', $value), '/'));
        } elseif ($argument === '-s') {
            if ($value !== '0' && $value !== '1') {
                runner_fail('-s 只支持 0 或 1');
            }
            $options['stop'] = $value === '1';
        } elseif ($argument === '--reportinfo') {
            foreach (explode(';', $value) as $item) {
                $pair = explode('=', $item, 2);
                if (count($pair) !== 2 || trim($pair[0]) === '') {
                    runner_fail('--reportinfo 格式错误：' . $item);
                }
                $options['metadata'][trim($pair[0])] = trim($pair[1]);
            }
        } else {
            $options[$valueOptions[$argument]] = $value;
        }
    }

    if ($options['type'] !== 'driver') {
        runner_fail('不支持的测试类型：' . $options['type']);
    }
    if (!in_array($options['mode'], array('normal', 'configure', 'sync', 'largedata', 'all'), true)) {
        runner_fail('不支持的测试模式：' . $options['mode']);
    }
    if ($options['format'] !== '' && $options['format'] !== 'jenkins') {
        runner_fail('--reportFormat 只支持 jenkins');
    }
    if (!class_exists('DOMDocument')) {
        runner_fail('PHP 缺少 DOM/XML 扩展');
    }
    $options['case_dir'] = runner_existing($options['case_dir'], '用例目录');

    // 连接参数优先级：源 phpunit.xml < --conf（默认 tools/conf/test.conf） < 环境变量 < 命令行；
    // RSRVNODEDIR 在 phpunit.xml 之后先取 <install-dir>/database/。
    $source = runner_load_xml($options['case_dir'] . '/phpunit.xml');
    if (!$source) {
        runner_fail('phpunit.xml 无效：' . $options['case_dir'] . '/phpunit.xml');
    }
    $connection = array();
    foreach ($source->getElementsByTagName('post') as $post) {
        $connection[$post->getAttribute('name')] = $post->getAttribute('value');
    }
    $conf = array();
    if ($options['conf_file'] !== '') {
        $conf = runner_kv_file(runner_existing($options['conf_file'], '配置文件'));
    } elseif (is_file($repo . '/testcase_new/tools/conf/test.conf')) {
        $conf = runner_kv_file($repo . '/testcase_new/tools/conf/test.conf');
    }
    $options['connection'] = array();
    foreach (array('HOSTNAME', 'SVCNAME', 'CHANGEDPREFIX', 'RSRVPORTBEGIN', 'RSRVPORTEND',
            'RSRVNODEDIR') as $key) {
        $name = $key === 'HOSTNAME' ? 'DB_HOSTNAME' : $key;
        $value = isset($connection[$key]) ? $connection[$key] : '';
        if ($key === 'RSRVNODEDIR' && $options['install_dir'] !== '') {
            $value = rtrim($options['install_dir'], '/') . '/database/';
        }
        foreach (array(isset($conf[$name]) ? $conf[$name] : '', (string)getenv($name),
                isset($cli[$key]) ? $cli[$key] : '') as $override) {
            if ($override !== '') {
                $value = $override;
            }
        }
        if ($value === '') {
            runner_fail('缺少连接参数：' . $key);
        }
        if (in_array($key, array('SVCNAME', 'RSRVPORTBEGIN', 'RSRVPORTEND'), true) &&
                (!preg_match('/^\d{1,5}$/', $value) || (int)$value > 65535)) {
            runner_fail('端口无效：' . $key . '=' . $value);
        }
        $options['connection'][$key] = $value;
    }

    if ($options['phpunit'] === '') {
        $options['phpunit'] = $options['case_dir'] . '/tools/phpunit.phar';
    }
    foreach (array('php_ini', 'extension', 'phpunit') as $key) {
        if ($options[$key] !== '') {
            $options[$key] = runner_absolute($options[$key]);
        }
    }
    if ($options['report_dir'] === '') {
        $options['report_dir'] = $repo . '/local_test_report/php/' . $options['type'] . '/' . $options['mode']
            . '/php' . PHP_MAJOR_VERSION . 'dot' . PHP_MINOR_VERSION;
    }
    $options['report_dir'] = runner_absolute($options['report_dir']);
    $options['workdir'] = $options['workdir'] === ''
        ? $options['report_dir'] . '/.work' : runner_absolute($options['workdir']);
    // 报告目录每次会清理旧报告，不能落在仓库根目录、用例目录或它们的上级。
    foreach (array('report_dir', 'workdir') as $key) {
        $real = realpath($options[$key]);
        $path = $real === false ? $options[$key] : $real;
        if (runner_within($repo, $path) || runner_within($path, $options['case_dir']) ||
                runner_within($options['case_dir'], $path)) {
            runner_fail($key . ' 不能是仓库目录或用例目录：' . $options[$key]);
        }
    }
    if ($options['workdir'] === $options['report_dir']) {
        runner_fail('workdir 不能与 report_dir 相同');
    }
    $options['files'] = runner_files($options['case_dir'], $options['mode'], $options['targets'],
        $options['target_option']);
    return $options;
}

// 公共文件不作为用例：tools/lib/commlib、func.php、global.php、隐藏文件及 common_list。
function runner_excluded($relative, $common)
{
    $parts = explode('/', $relative);
    if (in_array($parts[0], array('tools', 'lib', 'commlib'), true) ||
            in_array('lib', array_slice($parts, 1), true) ||
            in_array(end($parts), array('func.php', 'global.php'), true)) {
        return true;
    }
    foreach ($parts as $part) {
        if ($part === '' || $part[0] === '.') {
            return true;
        }
    }
    foreach ($common as $entry) {
        if (runner_within($relative, $entry)) {
            return true;
        }
    }
    return false;
}

// 选择用例：有 -f/-p 时只取目标；normal 取顶层目录减去各特殊列表；all 取全部顶层目录；
// 其他模式取 testcase.conf 对应列表。公共文件在所有模式下都排除。
// 目录下收集 *Test.php，按 suite（第一级目录）分组并去重，保持参数顺序。
function runner_files($root, $mode, $targets, $option = '')
{
    $lists = array();
    if (is_file($root . '/testcase.conf')) {
        foreach (runner_kv_file($root . '/testcase.conf') as $key => $value) {
            $lists[$key] = $value === '' ? array() : array_map('trim', explode(',', $value));
        }
    } elseif (!$targets) {
        runner_fail('缺少 testcase.conf');
    }
    $common = isset($lists['common_list']) ? $lists['common_list'] : array();
    $keys = array('configure' => 'configure_list', 'sync' => 'sync_list',
        'largedata' => 'large_data_list');
    if ($targets) {
        $paths = $targets;
    } elseif ($mode === 'normal' || $mode === 'all') {
        $blocked = $common;
        foreach (array_merge(array_values($keys), array('reliability_list')) as $key) {
            if ($mode === 'normal' && isset($lists[$key])) {
                $blocked = array_merge($blocked, $lists[$key]);
            }
        }
        $paths = array();
        foreach (scandir($root) as $child) {
            if (is_dir($root . '/' . $child) && !runner_excluded($child, $blocked)) {
                $paths[] = $child;
            }
        }
    } elseif (isset($lists[$keys[$mode]]) && $lists[$keys[$mode]]) {
        $paths = $lists[$keys[$mode]];
    } else {
        runner_fail('testcase.conf 未配置 ' . $mode . ' 模式的用例');
    }

    $files = array();
    foreach ($paths as $relative) {
        $path = $root . '/' . $relative;
        if ($relative === '' || strpos('/' . $relative . '/', '/../') !== false ||
                runner_excluded($relative, $common)) {
            runner_fail('不能选择的用例路径：' . $relative);
        }
        $found = array();
        if ($option === '-f' && is_dir($path)) {
            runner_fail('-f 只能指定用例文件，目录请用 -p：' . $relative);
        }
        if ($option === '-p' && !is_dir($path)) {
            runner_fail('-p 只能指定用例目录，文件请用 -f：' . $relative);
        }
        if (is_dir($path)) {
            $iterator = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($path,
                FilesystemIterator::SKIP_DOTS | FilesystemIterator::UNIX_PATHS));
            foreach ($iterator as $file) {
                $name = substr($file->getPathname(), strlen($root) + 1);
                if (substr($name, -8) === 'Test.php' && !runner_excluded($name, $common)) {
                    $found[] = $name;
                }
            }
            sort($found);
        } elseif (is_file($path) && substr($path, -4) === '.php' && strpos($relative, '/') !== false) {
            $found[] = $relative;
        } else {
            runner_fail('用例路径无效：' . $relative);
        }
        foreach ($found as $name) {
            $files[strtok($name, '/')][$root . '/' . $name] = true;
        }
    }
    if (!$files) {
        runner_fail('没有匹配的 PHP 用例', 3);
    }
    return array_map('array_keys', $files);
}

// ---- JUnit 处理 ----

function runner_children($node, $name)
{
    $result = array();
    foreach ($node->childNodes as $child) {
        if ($child instanceof DOMElement && $child->tagName === $name) {
            $result[] = $child;
        }
    }
    return $result;
}

function runner_find_suite($node, $name)
{
    foreach (runner_children($node, 'testsuite') as $suite) {
        if ($suite->getAttribute('name') === $name) {
            return $suite;
        }
        $nested = runner_find_suite($suite, $name);
        if ($nested) {
            return $nested;
        }
    }
    return null;
}

// 向同名 testsuite（找不到时为根节点）追加一条 error 用例，用于 setUpBeforeClass 失败和 PHPUnit 崩溃。
function runner_add_case($document, $suite, $class, $method, $type, $message, $time)
{
    $parent = runner_find_suite($document->documentElement, $suite);
    if (!$parent) {
        $parent = $document->documentElement;
    }
    $case = $parent->appendChild($document->createElement('testcase'));
    $case->setAttribute('classname', $class);
    $case->setAttribute('name', $method);
    if ($parent->hasAttribute('file')) {
        $case->setAttribute('file', $parent->getAttribute('file'));
    }
    $case->setAttribute('time', sprintf('%.6F', $time));
    $error = $case->appendChild($document->createElement('error'));
    $error->setAttribute('type', $type);
    $error->appendChild($document->createTextNode($message));
}

// 重新统计各级 tests/failures/errors/skipped/time；PHPUnit 3.7 把跳过和未完成记成 error，这里改为 skipped。
function runner_count($document, $node)
{
    $counts = array('tests' => 0, 'failures' => 0, 'errors' => 0, 'skipped' => 0, 'time' => 0.0);
    foreach (runner_children($node, 'testsuite') as $suite) {
        foreach (runner_count($document, $suite) as $key => $value) {
            $counts[$key] += $value;
        }
    }
    foreach (runner_children($node, 'testcase') as $case) {
        foreach (runner_children($case, 'error') as $error) {
            if (in_array($error->getAttribute('type'), array('PHPUnit_Framework_SkippedTestError',
                    'PHPUnit_Framework_IncompleteTestError'), true)) {
                $skipped = $document->createElement('skipped');
                while ($error->firstChild) {
                    $skipped->appendChild($error->firstChild);
                }
                $case->replaceChild($skipped, $error);
            }
        }
        $counts['tests']++;
        foreach (array('failure' => 'failures', 'error' => 'errors', 'skipped' => 'skipped') as $tag => $key) {
            if (runner_children($case, $tag)) {
                $counts[$key]++;
                break;
            }
        }
        $counts['time'] += max(0.0, (float)$case->getAttribute('time'));
    }
    foreach ($counts as $key => $value) {
        $node->setAttribute($key, $key === 'time' ? sprintf('%.6F', $value) : (string)$value);
    }
    return $counts;
}

// 递归删除文件或目录，路径不存在视为成功。
function runner_remove($path)
{
    if (is_dir($path) && !is_link($path)) {
        foreach (scandir($path) as $entry) {
            if ($entry !== '.' && $entry !== '..') {
                runner_remove($path . '/' . $entry);
            }
        }
        return @rmdir($path);
    }
    return !file_exists($path) && !is_link($path) || @unlink($path);
}

// ---- PHPUnit 子进程中的 listener：每个用例结束写一行 JSON 到进度文件，由主进程实时打印 ----

if (interface_exists('PHPUnit_Framework_TestListener')) {
    final class PhpRunnerListener implements PHPUnit_Framework_TestListener
    {
        private $path;
        private $results = array();
        private $emitted = array();

        public function __construct($path)
        {
            $this->path = $path;
        }

        private function emit($name, $status, $time, $problem, $detail, $scope = '')
        {
            $event = array('name' => $name, 'status' => $status, 'time' => max(0.0, (float)$time),
                'problem' => $problem, 'detail' => $detail, 'scope' => $scope);
            if (file_put_contents($this->path, json_encode($event) . "\n", FILE_APPEND | LOCK_EX) === false) {
                throw new RuntimeException('无法写入 PHP 用例执行状态');
            }
        }

        // setUpBeforeClass 失败时 PHPUnit 把错误记在 suite 上，单独输出一条 [fail]。
        private function record(PHPUnit_Framework_Test $test, $status, Exception $error, $time)
        {
            $problem = substr(preg_replace('/\s+/', ' ', trim($error->getMessage())), 0, 200);
            $detail = $status === 'fail' ? trim(PHPUnit_Framework_TestFailure::exceptionToString($error)
                . "\n" . PHPUnit_Util_Filter::getFilteredStacktrace($error)) : '';
            if (!($test instanceof PHPUnit_Framework_TestSuite)) {
                $this->results[spl_object_hash($test)] = array($status, $problem, $detail);
                return;
            }
            $key = spl_object_hash($test) . '|' . $status . '|' . $problem;
            if (!isset($this->emitted[$key])) {
                $this->emitted[$key] = true;
                $name = $test->getName() === '' ? 'suite' : $test->getName();
                $this->emit($name . '#setUpBeforeClass', $status, $time, $problem, $detail, 'suite');
            }
        }

        public function addError(PHPUnit_Framework_Test $test, Exception $e, $time)
        {
            $this->record($test, 'fail', $e, $time);
        }

        public function addFailure(PHPUnit_Framework_Test $test,
            PHPUnit_Framework_AssertionFailedError $e, $time)
        {
            $this->record($test, 'fail', $e, $time);
        }

        public function addIncompleteTest(PHPUnit_Framework_Test $test, Exception $e, $time)
        {
            $this->record($test, 'skip', $e, $time);
        }

        public function addSkippedTest(PHPUnit_Framework_Test $test, Exception $e, $time)
        {
            $this->record($test, 'skip', $e, $time);
        }

        public function startTestSuite(PHPUnit_Framework_TestSuite $suite)
        {
        }

        public function endTestSuite(PHPUnit_Framework_TestSuite $suite)
        {
        }

        public function startTest(PHPUnit_Framework_Test $test)
        {
            $this->results[spl_object_hash($test)] = array('pass', '', '');
        }

        public function endTest(PHPUnit_Framework_Test $test, $time)
        {
            $id = spl_object_hash($test);
            $result = isset($this->results[$id]) ? $this->results[$id] : array('pass', '', '');
            unset($this->results[$id]);
            $this->emit(get_class($test) . '#' . $test->getName(), $result[0], $time, $result[1], $result[2]);
        }
    }
}

// ---- 执行：准备报告目录 -> 运行前检查 -> 逐 suite 运行 PHPUnit -> 汇总 ----

class PhpTestRunner
{
    private $options;
    private $log;
    private $work = '';
    private $started;
    private $runtime;
    private $color;
    private $exitCode = 0;
    private $codes = array();
    private $messages = array();
    private $suiteFailures = array();
    private $totals = array('tests' => 0, 'passed' => 0, 'failures' => 0, 'errors' => 0, 'skipped' => 0);

    public function __construct($options)
    {
        $this->options = $options;
        $this->started = time();
        $this->runtime = 'php' . PHP_MAJOR_VERSION . 'dot' . PHP_MINOR_VERSION;
        $this->color = getenv('PHP_RUNNER_COLOR') === '1';
    }

    // 同时写终端和运行日志；$terminal 为带颜色的终端版本，日志不写颜色码；$echo 为 false 时只写日志。
    private function line($text, $terminal = null, $echo = true)
    {
        if ($echo) {
            echo ($terminal === null ? $text : $terminal) . "\n";
        }
        if (is_resource($this->log)) {
            fwrite($this->log, $text . "\n");
        }
    }

    // 仓库内路径显示为相对路径。
    private function display($path)
    {
        $prefix = $this->options['repo'] . '/';
        return strpos($path, $prefix) === 0 ? substr($path, strlen($prefix)) : $path;
    }

    private function save($path, $contents)
    {
        if (@file_put_contents($path, $contents) !== strlen($contents)) {
            runner_fail('无法写入：' . $path);
        }
    }

    private function makeDirectory($path)
    {
        if (!is_dir($path) && !@mkdir($path, 0755, true)) {
            runner_fail('无法创建目录：' . $path);
        }
    }

    // 打印一行用例状态；失败原因只写日志，-print failed 时改为在终端和日志输出完整详情；-print 时状态也只写日志。
    // suite 级失败暂存，PHPUnit 结束后补进 JUnit。
    private function printCase($case)
    {
        $label = sprintf('%-70s', $case['name']);
        $status = '[' . $case['status'] . ']';
        $time = sprintf('%6.3fs', $case['time']);
        $colors = array('pass' => '32', 'fail' => '31', 'skip' => '33');
        $colored = $this->color && isset($colors[$case['status']])
            ? "\033[" . $colors[$case['status']] . 'm' . $status . "\033[0m" : $status;
        $this->line("$label $status $time", "$label $colored $time", $this->options['print'] !== 'full');
        $detail = isset($case['detail']) && $case['detail'] !== '' ? $case['detail'] : $case['problem'];
        if ($case['status'] === 'fail' && $this->options['print'] === 'failed' && $detail !== '') {
            foreach (preg_split('/\r\n|\r|\n/', $detail) as $text) {
                $this->line('  ' . $text);
            }
        } elseif ($case['status'] === 'fail' && $case['problem'] !== '') {
            $this->line('  ' . $case['problem'], null, false);
        }
        if ($case['scope'] === 'suite' && $case['status'] === 'fail') {
            $this->suiteFailures[] = $case;
        }
    }

    // 读取文件自 $offset 起新增的内容。
    private function tail($path, &$offset)
    {
        clearstatcache(true, $path);
        $chunk = is_file($path) ? (string)@file_get_contents($path, false, null, $offset) : '';
        $offset += strlen($chunk);
        return $chunk;
    }

    // PHPUnit 子进程使用与本进程相同的 PHP、ini 和扩展；只给扩展时用 -n 避开默认 ini 中的旧扩展。
    private function phpCommand()
    {
        $command = array($this->options['php']);
        if ($this->options['php_ini'] !== '') {
            array_push($command, '-c', $this->options['php_ini']);
        } elseif ($this->options['extension'] !== '') {
            array_push($command, '-n', '-d', 'date.timezone=UTC');
        }
        if ($this->options['extension'] !== '') {
            array_push($command, '-d', 'extension=' . $this->options['extension']);
        }
        return $command;
    }

    // 子进程输出直接写文件，主循环转存到运行日志、-print 时原样回显，并逐行处理用例状态。
    private function process($arguments, $cwd, $stdout, $stderr, $progress)
    {
        $process = proc_open(implode(' ', array_map('escapeshellarg', $arguments)),
            array(0 => array('file', '/dev/null', 'r'), 1 => array('file', $stdout, 'w'),
                2 => array('file', $stderr, 'w')), $pipes, $cwd);
        if (!is_resource($process)) {
            runner_fail('无法启动 PHPUnit 进程');
        }
        $offsets = array(0, 0, 0);
        $buffer = '';
        do {
            $status = proc_get_status($process);
            foreach (array(1 => $stdout, 2 => $stderr) as $index => $path) {
                $chunk = $this->tail($path, $offsets[$index]);
                if ($chunk !== '') {
                    fwrite($this->log, $chunk);
                    if ($this->options['print'] === 'full') {
                        fwrite($index === 1 ? STDOUT : STDERR, $chunk);
                    }
                }
            }
            $buffer .= $this->tail($progress, $offsets[0]);
            while (($newline = strpos($buffer, "\n")) !== false) {
                $case = json_decode(substr($buffer, 0, $newline), true);
                $buffer = substr($buffer, $newline + 1);
                if (is_array($case)) {
                    $this->printCase($case);
                }
            }
            if ($status['running']) {
                usleep(100000);
            }
        } while ($status['running']);
        proc_close($process);
        return (int)$status['exitcode'];
    }

    // 清理上次生成的报告和日志，创建本次的临时目录并打印开头信息。
    private function prepare()
    {
        $report = $this->options['report_dir'];
        $this->makeDirectory($report);
        foreach (array_merge(array($report . '/raw', $report . '/runtest_php.log',
                $report . '/result.txt'), glob($report . '/*.xml') ?: array()) as $path) {
            if (!runner_remove($path)) {
                runner_fail('无法清理旧报告：' . $path);
            }
        }
        $this->makeDirectory($report . '/raw');
        $this->log = fopen($report . '/runtest_php.log', 'wb');
        if (!$this->log) {
            runner_fail('无法创建 runtest_php.log');
        }
        $this->work = $this->options['workdir'] . '/' . gmdate('Ymd_His') . '_' . getmypid();
        $this->makeDirectory($this->work);

        $this->line('======> Begin PHP test: ' . $this->options['type'] . '/' . $this->options['mode'] . ' =====>');
        $this->line(' Case dir  : ' . $this->display($this->options['case_dir']));
        $this->line(' PHP       : ' . PHP_VERSION);
        $this->line(' COORD     : ' . $this->options['connection']['HOSTNAME'] . ':'
            . $this->options['connection']['SVCNAME']);
        $this->line(' Log       : ' . $this->display($report . '/runtest_php.log'));
        $this->line('');
    }

    // 运行前检查 PHP 版本、扩展、PHPUnit 文件和 COORD 连接，失败即硬错误。
    private function preflight()
    {
        if (!preg_match('/^(5\.[456]|7\.[01])\./', PHP_VERSION)) {
            runner_fail('仅支持 PHP 5.4–7.1，当前为 ' . PHP_VERSION);
        }
        foreach (array('Phar', 'Sequoiadb') as $class) {
            if (!class_exists($class)) {
                runner_fail('缺少 PHP 扩展：' . $class . '（ini=' . (php_ini_loaded_file() ?: '无')
                    . '，extension=' . ($this->options['extension'] ?: '未指定') . '）');
            }
        }
        if (!is_file($this->options['phpunit'])) {
            runner_fail('PHPUnit 文件不存在：' . $this->options['phpunit']);
        }
        $coord = $this->options['connection']['HOSTNAME'] . ':' . $this->options['connection']['SVCNAME'];
        $database = new Sequoiadb();
        $database->connect($coord);
        $error = $database->getError();
        $database->close();
        if (!isset($error['errno']) || (int)$error['errno'] !== 0) {
            runner_fail('连接 COORD 失败：' . $coord . '，errno='
                . (isset($error['errno']) ? $error['errno'] : '未知'));
        }
    }

    // 生成单个 suite 的临时 phpunit.xml：连接参数、用例文件、listener 和原始 JUnit 输出位置。
    private function writeConfig($suite, $rawReport, $progress, $path)
    {
        $document = new DOMDocument('1.0', 'UTF-8');
        $document->formatOutput = true;
        $root = $document->appendChild($document->createElement('phpunit'));
        if ($this->options['stop']) {
            $root->setAttribute('stopOnFailure', 'true');
            $root->setAttribute('stopOnError', 'true');
        }
        $php = $root->appendChild($document->createElement('php'));
        $php->appendChild($document->createElement('includePath'))
            ->appendChild($document->createTextNode($this->options['case_dir']));
        foreach ($this->options['connection'] as $name => $value) {
            $post = $php->appendChild($document->createElement('post'));
            $post->setAttribute('name', $name);
            $post->setAttribute('value', $value);
        }
        $node = $root->appendChild($document->createElement('testsuites'))
            ->appendChild($document->createElement('testsuite'));
        $node->setAttribute('name', $suite);
        foreach ($this->options['files'][$suite] as $file) {
            $node->appendChild($document->createElement('file'))
                ->appendChild($document->createTextNode($file));
        }
        $listener = $root->appendChild($document->createElement('listeners'))
            ->appendChild($document->createElement('listener'));
        $listener->setAttribute('class', 'PhpRunnerListener');
        $listener->setAttribute('file', __FILE__);
        $listener->appendChild($document->createElement('arguments'))
            ->appendChild($document->createElement('string'))
            ->appendChild($document->createTextNode($progress));
        $log = $root->appendChild($document->createElement('logging'))
            ->appendChild($document->createElement('log'));
        $log->setAttribute('type', 'junit');
        $log->setAttribute('target', $rawReport);
        $log->setAttribute('logIncompleteSkipped', 'true');
        $this->save($path, $document->saveXML());
    }

    // 运行一个 suite，生成 <suite>_<runtime>_<host>.xml，返回 0 通过、1 失败。
    private function runSuite($suite)
    {
        $name = preg_replace('/[^A-Za-z0-9_.-]+/', '_', $suite);
        $report = $this->options['report_dir'];
        $rawReport = $report . '/raw/' . $name . '.junit.xml.raw';
        $stdout = $report . '/raw/' . $name . '.stdout.log';
        $stderr = $report . '/raw/' . $name . '.stderr.log';
        $progress = $this->work . '/' . $name . '.progress.jsonl';
        $config = $this->work . '/' . $name . '.phpunit.xml';
        $this->writeConfig($suite, $rawReport, $progress, $config);

        $code = $this->process(array_merge($this->phpCommand(),
            array($this->options['phpunit'], '-c', $config)),
            $this->options['case_dir'] . '/' . $suite, $stdout, $stderr, $progress);
        $this->codes[$suite] = $code;

        $problem = 'PHPUnit 未生成有效的 JUnit 报告';
        $document = is_file($rawReport) ? runner_load_xml($rawReport) : null;
        if ($document && in_array($document->documentElement->tagName, array('testsuites', 'testsuite'), true)) {
            foreach ($this->suiteFailures as $failure) {
                list($class, $method) = explode('#', $failure['name'], 2);
                runner_add_case($document, $class, $class, $method, 'Exception', $failure['problem'], $failure['time']);
            }
            $counts = runner_count($document, $document->documentElement);
            $failed = $counts['failures'] + $counts['errors'] > 0;
            $problem = !$counts['tests'] ? 'JUnit 报告没有测试用例'
                : ($code !== 0 && !$failed ? 'PHPUnit 异常退出' : '');
        } else {
            $document = new DOMDocument('1.0', 'UTF-8');
            $document->appendChild($document->createElement('testsuites'))
                ->appendChild($document->createElement('testsuite'))->setAttribute('name', $suite);
        }
        $this->suiteFailures = array();

        // PHPUnit 异常退出时记一条 phpunit error，按测试失败处理，后续 suite 继续执行。
        if ($problem !== '') {
            $detailPath = trim((string)@file_get_contents($stderr)) !== '' ? $stderr : $stdout;
            $lines = preg_split('/\r\n|\r|\n/', trim((string)@file_get_contents($detailPath)));
            $message = $problem . '（PHPUnit 退出码 ' . $code . '）'
                . (end($lines) !== '' ? '：' . trim(end($lines)) : '') . '；日志：' . $this->display($detailPath);
            $this->printCase(array('name' => $name . '#phpunit', 'status' => 'fail', 'time' => 0.0,
                'problem' => $message, 'scope' => ''));
            $this->messages[] = $suite . '：' . $message;
            runner_add_case($document, $suite, $suite, 'phpunit', 'PHPUnitCrash', $message, 0.0);
            $counts = runner_count($document, $document->documentElement);
        }

        if ($this->options['format'] === 'jenkins') {
            require_once dirname(__DIR__) . '/report-formatters/php_jenkins.php';
            php_jenkins_format($document, $this->options['type'] . '_php.' . $suite . '_' . $this->runtime,
                $this->options['metadata']);
        }
        $document->formatOutput = true;
        $host = preg_replace('/[^A-Za-z0-9_.-]+/', '_', gethostname());
        $this->save($report . '/' . $name . '_' . $this->runtime . '_' . $host . '.xml', $document->saveXML());

        $counts['passed'] = $counts['tests'] - $counts['failures'] - $counts['errors'] - $counts['skipped'];
        foreach ($this->totals as $key => $value) {
            $this->totals[$key] += $counts[$key];
        }
        $suiteExit = $counts['failures'] + $counts['errors'] > 0 ? 1 : 0;
        $this->exitCode = max($this->exitCode, $suiteExit);
        return $suiteExit;
    }

    // 删除临时目录，写 result.txt 并打印汇总。
    private function finish()
    {
        if ($this->work !== '') {
            runner_remove($this->work);
            @rmdir($this->options['workdir']);
        }
        if (!is_resource($this->log)) {
            return $this->exitCode;
        }
        $report = $this->options['report_dir'];
        $lines = array('runner_exit=' . $this->exitCode);
        foreach ($this->totals as $key => $value) {
            $lines[] = $key . '=' . $value;
        }
        foreach ($this->codes as $suite => $code) {
            $lines[] = 'phpunit_rc[' . $suite . ']=' . $code;
        }
        foreach ($this->messages as $message) {
            $lines[] = 'error=' . $message;
        }
        @file_put_contents($report . '/result.txt', implode("\n", $lines) . "\n");

        $this->line('');
        $this->line('<====== Finish PHP test ======');
        $this->line('');
        $this->line('***********************************************************');
        $this->line('                    ***test result***');
        $this->line(' begin time : ' . gmdate('Y-m-d H:i:s', $this->started) . ' UTC');
        $this->line(' end time   : ' . gmdate('Y-m-d H:i:s') . ' UTC');
        $this->line(' use time   : ' . sprintf('%.3f(s)', time() - $this->started));
        $this->line(' total      : ' . $this->totals['tests']);
        $this->line(' succeed    : ' . $this->totals['passed']);
        $this->line(' failed     : ' . ($this->totals['failures'] + $this->totals['errors']));
        $this->line(' skipped    : ' . $this->totals['skipped']);
        $this->line(' log        : ' . $this->display($report . '/runtest_php.log'));
        $this->line(' report     : ' . $this->display($report));
        $this->line(' exit code  : ' . $this->exitCode);
        $this->line('***********************************************************');
        fclose($this->log);
        return $this->exitCode;
    }

    // 硬错误立即停止；-s 1 时 PHPUnit 在第一个失败用例后停止，剩余 suite 也不再执行。
    public function execute()
    {
        try {
            $this->prepare();
            $this->preflight();
            foreach (array_keys($this->options['files']) as $suite) {
                if ($this->runSuite($suite) === 1 && $this->options['stop']) {
                    break;
                }
            }
        } catch (Exception $error) {
            $code = $error instanceof PhpRunnerException ? $error->getCode() : 2;
            $this->exitCode = max($this->exitCode, in_array($code, array(2, 3), true) ? $code : 2);
            $this->messages[] = $error->getMessage();
            fwrite(STDERR, 'PHP runner：' . $error->getMessage() . "\n");
        }
        return $this->finish();
    }
}

// ---- 入口：被 PHPUnit 作为 listener 文件加载时只定义类，不执行 ----

if (realpath(isset($_SERVER['SCRIPT_FILENAME']) ? $_SERVER['SCRIPT_FILENAME'] : '') !== __FILE__) {
    return;
}

try {
    $options = runner_options(array_slice($argv, 1), dirname(dirname(dirname(__DIR__))));
    if ($options['dry_run']) {
        foreach ($options['files'] as $files) {
            foreach ($files as $file) {
                echo substr($file, strlen($options['case_dir']) + 1), "\n";
            }
        }
        exit(0);
    }
    $runner = new PhpTestRunner($options);
    exit($runner->execute());
} catch (Exception $error) {
    $code = $error instanceof PhpRunnerException ? $error->getCode() : 2;
    fwrite(STDERR, 'PHP runner：' . $error->getMessage() . "\n");
    exit(in_array($code, array(2, 3), true) ? $code : 2);
}
