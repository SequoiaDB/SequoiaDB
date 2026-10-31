#!/usr/bin/env bash
# 被 runC.sh / runCPP.sh source。入口设置路径和语言变量。
# detect 默认找 $SCRIPT_DIR/lib/func.js，不把 js 放进这个目录。
# hookConsume：-1 不认识，0 只消费本参数，1 再消费下一个参数。
function hookParseOption()
{
   hookConsume=-1
}

function hookApplyCi() { :; }
function hookSkipCase() { return 1; }
function hookCaseArgs() { :; }
function hookCaseFailed() { :; }
function hookBeforeRun() { :; }
function hookSelect() { :; }
function hookValidate() { :; }
function hookReproArgs() { :; }

function prependLibPath()
{
   local dir=$1

   [ -n "$dir" ] && [ -d "$dir" ] || return 0
   case ":${LD_LIBRARY_PATH:-}:" in
      *":$dir:"*) ;;
      *) export LD_LIBRARY_PATH="$dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" ;;
   esac
}

# 默认退出 2；第二参数可指定 3（空发现）
function reportError()
{
   echo "[${RUNNER_TAG}][ERROR] $1" >&2
   exit ${2:-2}
}


function first_file()
{
   local f
   for f in "$@"
   do
      [ -f "$f" ] && { printf '%s' "$f"; return 0; }
   done
   return 1
}

function findSdb()
{
   local candidate="${INSTALL_DIR:-}/bin/sdb"

   if [ -x "$candidate" ]; then
      printf '%s' "$candidate"
   else
      command -v sdb 2>/dev/null
   fi
}

function makeAbsolutePath()
{
   realpath -m -- "$1"
}

function defaultReportDir()
{
   printf '%s/local_test_report/%s/driver/%s' \
      "$PROJECT_ROOT" "$CASE_TYPE" "$testType"
}

function isExecutableCase()
{
   [ -f "$1" ] && [ -x "$1" ]
}

function listExecutableCases()
{
   find "$1" -maxdepth 1 -type f -executable \
      ! -name '.*' \
      ! -name '*.sh' ! -name '*.bak' ! -name '*.conf' \
      ! -name '*.xml' ! -name '*.log' ! -name '*.txt' \
      ! -name 'SConstruct' ! -name 'SConscript' \
      ! -name '.sconsign.dblite' \
      -print | LC_ALL=C sort
}

function makeSafeName()
{
   printf '%s' "$1" | tr -cs '[:alnum:]._ -' '_'
}

function termCols()
{
   local cols=${COLUMNS:-0}

   if ! [[ "$cols" =~ ^[0-9]+$ ]] || [ "$cols" -lt 40 ] || [ "$cols" -gt 160 ]; then
      cols=$(tput cols 2>/dev/null || echo 80)
   fi
   if ! [[ "$cols" =~ ^[0-9]+$ ]] || [ "$cols" -lt 40 ] || [ "$cols" -gt 160 ]; then
      cols=80
   fi
   printf '%s' "$cols"
}

_caseLinePrinted=""

function beginCaseLine()
{
   local name=$1
   local cols maxName

   _caseLinePrinted=""
   [ -t 1 ] || return 0
   cols=$(termCols)
   maxName=$((cols - 18))
   [ "$maxName" -lt 8 ] && maxName=8
   if [ ${#name} -gt "$maxName" ]; then
      name="${name:0:$((maxName - 1))}~"
   fi
   _caseLinePrinted=$name
   printf '%s' "$name"
}

function finishCaseLine()
{
   local name=$1
   local kind=$2
   local extra=$3
   local plain color reset='' cols pad printed

   case $kind in
      pass)
         plain=$(printf '[pass] %ss' "$extra")
         color=$'\033[32m'
         ;;
      fail)
         plain=$(printf '[fail] %ss' "$extra")
         color=$'\033[31m'
         ;;
      error)
         plain=$(printf '[error] %s' "$extra")
         color=$'\033[31m'
         ;;
      *)
         plain=$extra
         color=''
         ;;
   esac

   if [ ! -t 1 ]; then
      printf '%s %s\n' "$name" "$plain"
      return
   fi

   [ -n "$color" ] && reset=$'\033[0m'
   cols=$(termCols)
   printed=${_caseLinePrinted:-$name}
   pad=$((cols - ${#printed} - ${#plain}))
   [ "$pad" -lt 1 ] && pad=1
   printf '%*s%s%s%s\n' "$pad" "" "$color" "$plain" "$reset"
   _caseLinePrinted=""
}

function extractXmlFailures()
{
   local xml=$1

   [ -s "$xml" ] || return 0
   command -v python3 >/dev/null 2>&1 || return 0
   python3 - "$xml" <<'PY'
import html, re, sys
text = open(sys.argv[1], encoding="utf-8", errors="replace").read()
for tag in ("failure", "error"):
    for m in re.finditer(r"<%s\b([^>]*)>(.*?)</%s>" % (tag, tag), text, re.S):
        attrs, body = m.group(1), m.group(2)
        am = re.search(r'message="([^"]*)"', attrs)
        msg = html.unescape(am.group(1).replace("&#x0A;", "\n")) if am else ""
        body = re.sub(r"<!\[CDATA\[(.*?)\]\]>", r"\1", body, flags=re.S).strip()
        if msg:
            print(msg)
        if body and body != msg:
            print(body)
        print("---")
PY
}

function appendResultLine()
{
   local name=$1
   local status=$2
   local elapsed=$3
   local width=56

   [ ${#name} -gt "$width" ] && width=${#name}
   printf "===> %-${width}s [ %s ] %ss\n" "$name" "$status" "$elapsed" >>"$resultFile"
}

function relFromTestcaseRoot()
{
   local path=$1

   if [ "$path" = "$TESTCASE_ROOT" ]; then
      printf '.'
   elif [[ "$path" == "$TESTCASE_ROOT"/* ]]; then
      printf '%s' "${path#"$TESTCASE_ROOT"/}"
   else
      printf '%s' "$path"
   fi
}

function writeCaseOut()
{
   local name=$1
   local status=$2
   local elapsed=$3
   local outFile="$rawReportDir/${name}_out.txt"

   {
      printf 'CMD: bash testcase_new/tools/%s --no-up --no-compile --test --case-dir %s -f %s' \
         "$(basename "$0")" "$(relFromTestcaseRoot "$caseDir")" "$name"
      hookReproArgs
      printf '\n'
      printf 'case   : %s\n' "$name"
      printf 'status : %s\n' "$status"
      printf 'time   : %ss\n' "$elapsed"
      printf '\n--- stdout ---\n'
      [ -f "$stdoutFile" ] && cat "$stdoutFile"
      printf '\n--- stderr ---\n'
      [ -f "$stderrFile" ] && cat "$stderrFile"
      if [ -s "$xmlFile" ] && grep -Eq '<(failure|error)[[:space:]>]' "$xmlFile"; then
         printf '\n--- failures ---\n'
         extractXmlFailures "$xmlFile"
      fi
   } >"$outFile"
}

function writeHumanReport()
{
   appendResultLine "$1" "$2" "$3"
   writeCaseOut "$1" "$2" "$3"
}

function showTestResult()
{
   local useColor=${1:-0}
   local failedAll=$((programFail + programError))

   echo "***********************************************************"
   echo "                    ***test result***"
   echo " begin time : $runBeginTime"
   echo " end time   : $runEndTime"
   echo " use time   : ${runUseTime}(s)"
   echo " total      : $programTotal"
   if [ "$useColor" -eq 1 ] && [ -t 1 ]; then
      printf " succeed    : \033[32m%s\033[0m\n" "$programPass"
      if [ "$failedAll" -ne 0 ]; then
         printf " failed     : \033[31m%s\033[0m\n" "$failedAll"
      else
         echo " failed     : $failedAll"
      fi
   else
      echo " succeed    : $programPass"
      echo " failed     : $failedAll"
   fi
   echo " report     : $rawReportDir"
   echo "***********************************************************"
}

# 130/143 > 124 > 3 > 2 > 1 > 0。不把更严重的码盖掉
function mergeRunnerRC()
{
   local newRC=$1

   case $newRC in
      130 | 143 )
         runnerRC=$newRC
         ;;
      124 )
         if [ $runnerRC -ne 130 ] && [ $runnerRC -ne 143 ]; then
            runnerRC=124
         fi
         ;;
      3 )
         case $runnerRC in
            0 | 1 | 2 ) runnerRC=3 ;;
         esac
         ;;
      2 )
         case $runnerRC in
            0 | 1 ) runnerRC=2 ;;
         esac
         ;;
      1 )
         if [ $runnerRC -eq 0 ]; then
            runnerRC=1
         fi
         ;;
   esac
}

function contains()
{
   local expected=$1
   local item
   shift

   for item in "$@"
   do
      if [ "$item" = "$expected" ]; then
         return 0
      fi
   done
   return 1
}

function loadConfFile()
{
   local file=$1
   local line key value

   [ -f "$file" ] || reportError "configuration file not found: $file"
   while IFS= read -r line || [ -n "$line" ]
   do
      line=${line%$'\r'}
      line=${line#"${line%%[![:space:]]*}"}
      [ -n "$line" ] || continue
      [ "${line:0:1}" = "#" ] && continue
      [[ "$line" == *=* ]] || continue
      key=${line%%=*}
      value=${line#*=}
      key=${key%"${key##*[![:space:]]}"}
      value=${value#"${value%%[![:space:]]*}"}
      value=${value%"${value##*[![:space:]]}"}
      case $key in
         INSTALL_DIR | SVCNAME | RSRVNODEDIR | DB_HOSTNAME )
            printf -v "conf_$key" '%s' "$value"
            ;;
      esac
   done <"$file"
}

function loadTypeLists()
{
   local conf="$SRC_DIR/testcase.conf"
   local key line item
   local -a keys parts
   skipNames=()
   onlyNames=()
   case $testType in
      normal ) keys=(smoke_list large_data_list sync_list configure_list reliability_list) ;;
      smoke | sync | configure ) keys=("${testType}_list") ;;
      * ) return 0 ;;
   esac
   if [ ! -f "$conf" ]; then
      return 0
   fi
   for key in "${keys[@]}"
   do
      line=$(grep "^${key}=" "$conf" | head -1 || true)
      [ -n "$line" ] || continue
      line=${line#*=}
      IFS=',' read -r -a parts <<< "$line"
      for item in "${parts[@]}"
      do
         item=${item//[[:space:]]/}
         [ -n "$item" ] || continue
         if [ "$testType" = "normal" ]; then
            skipNames+=("$item")
         else
            onlyNames+=("$item")
         fi
      done
   done
}

function svnUp()
{
   (cd "$SRC_DIR" && svn up) || reportError "svn update failed"
}

function runScons()
{
   local -a cmd=(scons)

   [ $isRelease -eq 1 ] || cmd+=(--dd)
   cmd+=("$@")
   if [ $dryRun -eq 1 ]; then
      printf 'DRY-RUN: cd %q &&' "$SRC_DIR"
      printf ' %q' "${cmd[@]}"
      printf '\n'
      return 0
   fi
   (cd "$SRC_DIR" && "${cmd[@]}") || reportError "compile failed"
}

function loadSconsCases()
{
   local conf="$SRC_DIR/SConscript"
   local entryRe='^[[:space:]]*\[[[:space:]]*"'
   local strRe='"([^"]*)"'
   local line rest src first srcs

   sconsNames=()
   sconsSources=()
   [ -f "$conf" ] || return 0
   while IFS= read -r line || [ -n "$line" ]
   do
      [[ "$line" =~ $entryRe ]] || continue
      rest=$line
      first=""
      srcs=""
      while [[ "$rest" =~ $strRe ]]
      do
         src=${BASH_REMATCH[1]}
         rest=${rest#*"\"$src\""}
         src=${src#./}
         [ -n "$first" ] || first=$src
         srcs+="${srcs:+|}$src"
      done
      [ -n "$first" ] || continue
      first=${first##*/}
      sconsNames+=("${first%.*}")
      sconsSources+=("$srcs")
   done <"$conf"
}

function isSourceFile()
{
   case $1 in
      *.cpp | *.cc | *.cxx | *.c ) return 0 ;;
      * ) return 1 ;;
   esac
}

function addSelected()
{
   contains "$1" "${selected[@]}" || selected+=("$1")
}

function addProgram()
{
   addSelected "prog:$1"
   addSelected "prog:$1.static"
}

function matchSource()
{
   local path=$1
   local i src
   local -a srcs

   for i in "${!sconsNames[@]}"
   do
      IFS='|' read -r -a srcs <<< "${sconsSources[$i]}"
      for src in "${srcs[@]}"
      do
         if [ "$SRC_DIR/$src" = "$path" ]; then
            printf '%s\n' "${sconsNames[$i]}"
            break
         fi
      done
   done
}

function underSrc()
{
   local arg=$1

   if [[ "$arg" = /* ]]; then
      makeAbsolutePath "$arg"
   else
      makeAbsolutePath "$SRC_DIR/$arg"
   fi
}

function selectFile()
{
   local arg=$1
   local path name
   local -a matched

   if isSourceFile "$arg"; then
      path=$(underSrc "$arg")
      [ -f "$path" ] || reportError "-f not found: $path"
      mapfile -t matched < <(matchSource "$path")
      [ ${#matched[@]} -gt 0 ] || reportError "not an active testcase source in $SRC_DIR/SConscript: $path"
      addProgram "${matched[0]}"
      return
   fi
   if [[ "$arg" = */* ]]; then
      reportError "-f expects a source file such as crud/name.cpp, or a program name such as name"
   fi
   name=${arg%.static}
   if [ -f "$SRC_DIR/SConscript" ] && ! contains "$name" "${sconsNames[@]}"; then
      reportError "not an active testcase in $SRC_DIR/SConscript: $arg"
   fi
   if [[ "$arg" = *.static ]]; then
      addSelected "prog:$arg"
   else
      addProgram "$arg"
   fi
}

function selectDir()
{
   local arg=$1
   local path i src found=0
   local -a srcs

   path=$(underSrc "$arg")
   [ -d "$path" ] || reportError "-p is not a directory: $arg"
   if [ "$path" != "$SRC_DIR" ] && [[ "$path" != "$SRC_DIR"/* ]]; then
      reportError "-p must be under $SRC_DIR: $path"
   fi
   for i in "${!sconsNames[@]}"
   do
      IFS='|' read -r -a srcs <<< "${sconsSources[$i]}"
      for src in "${srcs[@]}"
      do
         if [[ "$SRC_DIR/$src" == "$path"/* ]]; then
            addProgram "${sconsNames[$i]}"
            found=1
            break
         fi
      done
   done
   [ $found -eq 1 ] || reportError "no active testcase in $SRC_DIR/SConscript under: $path"
}

function resolveSelection()
{
   local item

   [ ${#selectArgs[@]} -gt 0 ] || return 0
   loadSconsCases
   for item in "${selectArgs[@]}"
   do
      case $item in
         file:* ) selectFile "${item#file:}" ;;
         dir:* ) selectDir "${item#dir:}" ;;
      esac
   done
}

function compileSelected()
{
   local item name
   local -a targets=()

   for item in "${selected[@]}"
   do
      case $item in
         prog:* ) name=${item#prog:} ;;
         * ) continue ;;
      esac
      if ! contains "${name%.static}" "${sconsNames[@]}"; then
         reportError "cannot compile $name: not an active testcase in $SRC_DIR/SConscript"
      fi
      contains "build_test/$name" "${targets[@]}" || targets+=("build_test/$name")
   done
   [ ${#targets[@]} -gt 0 ] || return 0
   if [ $dryRun -eq 0 ]; then
      rm -rf -- "$SRC_DIR/build_test" "$SRC_DIR/report"
   fi
   runScons "${targets[@]}"
}

function compile()
{
   if [ ${#selected[@]} -gt 0 ]; then
      compileSelected
      return
   fi
   if [ $dryRun -eq 0 ]; then
      rm -rf -- "$SRC_DIR/build" "$SRC_DIR/build_test"
   fi
   runScons
}

function discoverTestcases()
{
   local item
   local testcase
   local testcaseName
   local base

   if [ ${#selected[@]} -gt 0 ]; then
      for item in "${selected[@]}"
      do
         case $item in
            exe:* ) testcase=${item#exe:} ;;
            prog:* )
               if [ $needCompile -eq 1 ]; then
                  testcase="$SRC_DIR/build_test/${item#prog:}"
               else
                  testcase="$caseDir/${item#prog:}"
               fi
               ;;
         esac
         if [ $dryRun -eq 0 ] && ! isExecutableCase "$testcase"; then
            reportError "test program not found or not executable: $testcase"
         fi
         testcases+=("$testcase")
      done
      return
   fi

   while IFS= read -r testcase
   do
      testcaseName=${testcase##*/}
      base=${testcaseName%.static}
      case $testType in
         normal )
            if [ $useSSL -eq 0 ] && contains "$testcaseName" "${sslFile[@]}"; then
               continue
            fi
            hookSkipCase "$testcaseName" && continue
            # SSL 程序也在 configure_list 里。-usessl 时不要被 normal 的排除名单再滤掉。
            if contains "$testcaseName" "${skipNames[@]}" || contains "$base" "${skipNames[@]}"; then
               if [ $useSSL -eq 1 ] && { contains "$testcaseName" "${sslFile[@]}" || contains "$base" "${sslFile[@]}"; }; then
                  :
               else
                  continue
               fi
            fi
            ;;
         smoke | sync | configure )
            if [ ${#onlyNames[@]} -gt 0 ]; then
               if ! contains "$testcaseName" "${onlyNames[@]}" && ! contains "$base" "${onlyNames[@]}"; then
                  continue
               fi
            fi
            ;;
      esac
      testcases+=("$testcase")
   done < <( [ -d "$caseDir" ] && listExecutableCases "$caseDir" || true )
}

function copyRunArtifacts()
{
   local dest=$1
   local copy_case=${2:-0}
   local reportFile

   mkdir -p -- "$dest" || return 1
   for reportFile in "${reportArtifacts[@]}"
   do
      [ -e "$reportFile" ] || continue
      cp -p -- "$reportFile" "$dest/" || return 1
      if [ "$copy_case" -eq 1 ] && [ -f "$reportFile.case" ]; then
         cp -p -- "$reportFile.case" "$dest/$(basename -- "$reportFile").case" || return 1
      fi
   done
}

function findJenkinsFormatter()
{
   first_file \
      "$SCRIPT_DIR/report-formatters/c_cpp_jenkins.sh" \
      "$SCRIPT_DIR/c_cpp_jenkins.sh" \
      "$TESTCASE_ROOT/tools/report-formatters/c_cpp_jenkins.sh"
}

function needChangeConf()
{
   case ${TEST_TYPE:-} in
      CONFIGURETEST | MVCCTEST ) return 0 ;;
      *) return 1 ;;
   esac
}

function findNodeConf()
{
   first_file "$caseDir/$1.node.conf" "$SRC_DIR/$1.node.conf"
}

function findChangeConfJs()
{
   first_file "$SCRIPT_DIR/changeConf.js" "$TESTCASE_ROOT/tools/changeConf.js"
}

function applyNodeConf()
{
   local conf=$1
   local mode=$2
   local js sdbbin rc=0

   confOutput=""
   js=$(findChangeConfJs) || {
      confOutput="changeConf.js not found"
      echo "[${RUNNER_TAG}][ERROR] $confOutput" >&2
      return 2
   }
   sdbbin=$(findSdb) || {
      confOutput="sdb not found"
      echo "[${RUNNER_TAG}][ERROR] $confOutput" >&2
      return 2
   }
   echo " [ Configure ] $mode $(basename -- "$conf")"
   confOutput=$(cd "$(dirname -- "$conf")" && timeout 600 "$sdbbin" -f "$(basename -- "$conf"),$js" \
      -e "var hostname='$hostname'; var svcname=$svcname; var mode='$mode';" 2>&1) || rc=$?
   printf '%s\n' "$confOutput"
   [ $rc -eq 0 ]
}

# after 用 before 写下的 node.conf.ini，用完删掉。没有 ini 也记一份恢复失败。
function restoreNodeConf()
{
   local ini=$1/node.conf.ini
   local restoreRC=0

   if [ ! -f "$ini" ]; then
      confOutput="node.conf.ini not found"
      writeCiFailXml "configure_after_${testcaseName}.xml" \
         "${TESTCASE_TYPE:-$CASE_TYPE}" "detect_env.${TESTCASE_TYPE:-$CASE_TYPE}" \
         "change_conf_after_${testcaseName}" "$confOutput"
      mergeRunnerRC 1
      [ "$stopFlag" -eq 1 ] && shouldBreak=1
      return 1
   fi
   applyNodeConf "$ini" after || restoreRC=$?
   if [ "$restoreRC" -ne 0 ]; then
      writeCiFailXml "configure_after_${testcaseName}.xml" \
         "${TESTCASE_TYPE:-$CASE_TYPE}" "detect_env.${TESTCASE_TYPE:-$CASE_TYPE}" \
         "change_conf_after_${testcaseName}" "$confOutput"
      rm -f -- "$ini"
      mergeRunnerRC "$restoreRC"
      [ "$stopFlag" -eq 1 ] && shouldBreak=1
      return "$restoreRC"
   fi
   rm -f -- "$ini"
}

function foreignStop()
{
   local f
   for f in "${failFlag:-${CI_WORK_DIR:+$CI_WORK_DIR/report/.TESTCASE_FAIL}}" "${reportDir:-}/.TESTCASE_FAIL"
   do
      if [ -n "$f" ] && [ -f "$f" ]; then
         return 0
      fi
   done
   return 1
}

function markTestLog()
{
   local kind=$1
   local name=$2
   local bin

   [ "$ciMode" -eq 1 ] || [ -n "${detectDir:-}" ] || [ "${reportFormat:-}" = "jenkins" ] || return 0
   bin="${INSTALL_DIR:-}/bin/sdb"
   [ -x "$bin" ] || bin=$(command -v sdb 2>/dev/null || true)
   [ -n "$bin" ] || return 0
   timeout 20 "$bin" -e "var db=new Sdb('${hostname}',${svcname}); db.msg('${kind} testcase[${name}]'); db.close();" \
      >/dev/null 2>&1 || true
}

function writeCiFailXml()
{
   local file=$1
   local suit=$2
   local classname=$3
   local name=$4
   local message=$5
   local dest="$rawReportDir/raw/$file"

   message=${message//]]>/]]]]><![CDATA[>}
   cat >"$dest" <<EOF
<testsuite name="${suit}">
<testcase classname="${classname}" name="${name}" time="0">
<failure><![CDATA[
EOF
   printf '%s\n' "$message" >>"$dest"
   cat >>"$dest" <<'EOF'
]]></failure>
</testcase>
</testsuite>
EOF
   reportArtifacts+=("$dest")
}

function detectSyncEnv()
{
   local name=$1
   local safe func js sdbbin out rc=0

   [ "${TEST_TYPE:-}" = "SYNCTEST" ] || return 0
   [ "$ciMode" -eq 1 ] || [ -n "${detectDir:-}" ] || [ "${reportFormat:-}" = "jenkins" ] || return 0
   if [ -n "$detectDir" ]; then
      func=$detectDir/func.js
      js=$detectDir/detect_cluster.js
   else
      func=${DETECT_FUNC_JS:-$SCRIPT_DIR/lib/func.js}
      js=${DETECT_CLUSTER_JS:-$SCRIPT_DIR/lib/detect_cluster.js}
   fi
   sdbbin=""
   if ! sdbbin=$(findSdb); then
      out="sdb not found"
      rc=2
   elif [ ! -f "$func" ] || [ ! -f "$js" ]; then
      out="detect_cluster.js not found: ${func:-<unset>} ${js:-<unset>}"
      rc=2
   else
      echo " [ Detect ] $name"
      out=$(timeout 1800 "$sdbbin" -f "$func,$js" \
         -e "var COORDSVCNAME='${svcname}';var COORDHOSTNAME='${hostname}'" 2>&1) || rc=$?
      printf '%s\n' "$out"
   fi
   [ $rc -eq 0 ] && return 0
   safe=${name//./_}
   writeCiFailXml "before_${TESTCASE_TYPE:-$CASE_TYPE}_${safe}.xml" \
      "detect_env" "${TESTCASE_TYPE:-$CASE_TYPE}.detect_env" "before_${safe}" "$out"
   [ "$printMode" != "full" ] && beginCaseLine "$name"
   markCase error "detectenv" Error 2
   return 1
}

function publishReports()
{
   local stage
   local rc=0
   local formatter

   if [ -z "$reportDir" ]; then
      return
   fi

   if [ "$reportFormat" = "jenkins" ]; then
      formatter=$(findJenkinsFormatter) || {
         echo "[${RUNNER_TAG}][ERROR] c_cpp_jenkins.sh not found" >&2
         return 1
      }
      stage=$(mktemp -d) || return 1
      if ! copyRunArtifacts "$stage" 1; then
         rm -rf -- "$stage"
         return 1
      fi
      TESTHOST_LIST="${TESTHOST_LIST:-}" DIAGLOG_URL="${DIAGLOG_URL:-}" \
         bash "$formatter" \
         "$stage" "$reportDir" "${TESTCASE_TYPE:-$CASE_TYPE}"
      rc=$?
      rm -rf -- "$stage"
      return $rc
   fi

   copyRunArtifacts "$reportDir"
}

function markCase()
{
   local kind=$1 extra=$2 word=$3 rc=$4

   if [ "$printMode" != "full" ]; then
      finishCaseLine "$testcaseName" "$kind" "$extra"
   fi
   writeHumanReport "$testcaseName" "$word" "${elapsed:-0}"
   mergeRunnerRC "$rc"
   case $kind in
      pass) programPass=$((programPass + 1)) ;;
      fail) programFail=$((programFail + 1)) ;;
      error) programError=$((programError + 1)) ;;
   esac
   if [ "$rc" -eq 130 ] || [ "$rc" -eq 143 ]; then
      shouldBreak=1
   elif [ $stopFlag -eq 1 ] && [ "$rc" -ne 0 ]; then
      shouldBreak=1
   fi
}

function autoTest()
{
   local runId
   local testcase
   local testcaseName
   local reportBase
   local xmlFile
   local stdoutFile
   local stderrFile
   local beginTime
   local endTime
   local elapsed
   local testcaseRC
   local confRC
   local tests
   local sequence=0
   local total=0
   local runnerRC=0
   local programPass=0
   local programFail=0
   local programError=0
   local programTotal=0
   local resultFile
   local runBeginEpoch
   local runEndEpoch
   local runBeginTime
   local runEndTime
   local runUseTime
   local foreignStopped=0
   local -a command

   rawReportDir="$reportDir"
   resultFile="$rawReportDir/result.txt"
   if [ $dryRun -eq 0 ]; then
      rm -rf -- "$rawReportDir"
      mkdir -p -- "$rawReportDir/raw" || reportError "create report directory failed"
      hookBeforeRun
      : >"$resultFile"
   fi

   runId="$(makeSafeName "$(hostname 2>/dev/null || echo host)")_$(date +%Y%m%d-%H%M%S)_$$"
   runBeginEpoch=$(date +%s.%N)
   runBeginTime=$(date '+%Y-%m-%d %H:%M:%S')
   echo " [ Execute ] ${#testcases[@]} $CASE_TYPE test program(s)"

   for testcase in "${testcases[@]}"
   do
      testcaseName=${testcase##*/}
      sequence=$((sequence + 1))
      reportBase="${testcaseName}_${runId}_$(printf '%04d' "$sequence")"
      xmlFile="$rawReportDir/raw/$reportBase.xml"
      stdoutFile="$rawReportDir/raw/$reportBase.stdout.log"
      stderrFile="$rawReportDir/raw/$reportBase.stderr.log"
      command=("$testcase" -n "$hostname" -s "$svcname" -d "$dbpath"
               -b "$rsrvPortBegin" -e "$rsrvPortEnd")
      hookCaseArgs
      command+=(--gtest_filter='*' "--gtest_output=xml:$xmlFile")

      if [ $dryRun -eq 1 ]; then
         printf 'DRY-RUN:'
         printf ' %q' "${command[@]}"
         printf '\n'
         continue
      fi

      if [ "$stopFlag" -eq 1 ] && foreignStop; then
         echo " [ Stop ] .TESTCASE_FAIL"
         foreignStopped=1
         break
      fi

      nodeConfFile=""
      needAfter=0
      shouldBreak=0
      if ! detectSyncEnv "$testcaseName"; then
         [ $shouldBreak -eq 1 ] && break
         continue
      fi
      if needChangeConf; then
         nodeConfFile=$(findNodeConf "$testcaseName" || true)
         [ -n "$nodeConfFile" ] || nodeConfFile="$caseDir/${testcaseName}.node.conf"
         confRC=0
         applyNodeConf "$nodeConfFile" before || confRC=$?
         if [ $confRC -ne 0 ]; then
            writeCiFailXml "configure_before_${testcaseName}.xml" \
               "${TESTCASE_TYPE:-$CASE_TYPE}" "detect_env.${TESTCASE_TYPE:-$CASE_TYPE}" \
               "change_conf_before_${testcaseName}" "$confOutput"
            [ "$printMode" != "full" ] && beginCaseLine "$testcaseName"
            if [ $confRC -eq 2 ]; then
               markCase error "changeconf before" Error 2
            else
               markCase error "changeconf before" Error 1
            fi
            restoreNodeConf "$(dirname -- "$nodeConfFile")"
            [ $shouldBreak -eq 1 ] && break
            continue
         fi
         needAfter=1
      fi

      markTestLog Begin "$testcase"
      [ "$printMode" != "full" ] && beginCaseLine "$testcaseName"
      beginTime=$(date +%s.%N)
      "${command[@]}" >"$stdoutFile" 2>"$stderrFile"
      testcaseRC=$?
      markTestLog End "$testcase"
      endTime=$(date +%s.%N)
      elapsed=$(awk -v begin="$beginTime" -v end="$endTime" \
         'BEGIN { printf "%.3f", end - begin }')
      reportArtifacts+=("$xmlFile" "$stdoutFile" "$stderrFile")
      printf '%s\n' "$testcaseName" >"$xmlFile.case"

      if [ $testcaseRC -eq 130 ] || [ $testcaseRC -eq 143 ]; then
         markCase error "exit=$testcaseRC" Error $testcaseRC
      elif [ $testcaseRC -eq 124 ]; then
         markCase error "exit=$testcaseRC" Error 124
      elif [ ! -s "$xmlFile" ]; then
         markCase error "missing report" Error 3
      else
         tests=$(grep -Eoh '<testcase[[:space:]>]' "$xmlFile" | wc -l || true)
         if [ "$tests" -eq 0 ]; then
            markCase error "no testcase" Error 3
         elif grep -Eq '<(failure|error)[[:space:]>]' "$xmlFile"; then
            markCase fail "$elapsed" Failed 1
            hookCaseFailed
            if [ "$printMode" = "failed" ]; then
               cat "$stdoutFile"
               cat "$stderrFile" >&2
               extractXmlFailures "$xmlFile"
            fi
         elif [ $testcaseRC -ne 0 ]; then
            markCase fail "$elapsed" Failed 1
            hookCaseFailed
            if [ "$printMode" = "failed" ]; then
               cat "$stdoutFile"
               cat "$stderrFile" >&2
            fi
         else
            markCase pass "$elapsed" Done 0
         fi
         if [ "$printMode" = "full" ]; then
            cat "$stdoutFile"
            cat "$stderrFile" >&2
         fi
         [ "${tests:-0}" -gt 0 ] && total=$((total + tests))
      fi

      if [ $needAfter -eq 1 ]; then
         restoreNodeConf "$(dirname -- "$nodeConfFile")"
      fi
      [ $shouldBreak -eq 1 ] && break
   done

   if [ $dryRun -eq 1 ]; then
      return 0
   fi
   programTotal=$((programPass + programFail + programError))
   runEndEpoch=$(date +%s.%N)
   runEndTime=$(date '+%Y-%m-%d %H:%M:%S')
   runUseTime=$(awk -v begin="$runBeginEpoch" -v end="$runEndEpoch" \
      'BEGIN { printf "%.3f", end - begin }')
   # 一个程序结果都没记下来时才是缺报告。有 XML 但没有 <testcase> 不升成 3。
   if [ $programTotal -eq 0 ] && [ $runnerRC -eq 0 ] && [ $foreignStopped -eq 0 ]; then
      runnerRC=3
   fi
   # 看到别的机器的 .TESTCASE_FAIL 而停下时，也要让 Ant 写下本类型的标记。
   if [ $foreignStopped -eq 1 ] && [ $runnerRC -eq 0 ]; then
      runnerRC=1
   fi
   echo
   showTestResult 1
   {
      echo
      showTestResult 0
   } >>"$resultFile"
   reportArtifacts+=("$resultFile")
   if ! publishReports; then
      mergeRunnerRC 2
   fi
   if ! hideRawXml; then
      mergeRunnerRC 2
   fi
   return $runnerRC
}

# raw/ 在报告目录内，Jenkins 的 report/**/*.xml 会把原始 XML 与发布后的 XML 重复收集。
# 发布完成后改名为 .xml.raw（与 PHP 一致），内容保留供排查。
function hideRawXml()
{
   local file

   [ -n "$rawReportDir" ] && [ -d "$rawReportDir/raw" ] || return 0
   for file in "$rawReportDir"/raw/*.xml
   do
      [ -f "$file" ] || continue
      mv -f -- "$file" "$file.raw" || return 1
   done
}

function runnerMain()
{
   trap 'exit 130' INT
   trap 'exit 143' TERM

   needUpdate=1
   needCompile=1
   isRelease=0
   hostname="localhost"
   user=""
   passwd=""
   dbpath="/opt/sequoiadb/database/"
   useSSL=0
   runTests=0
   caseDirInput="$SRC_DIR/build_test"
   caseDirGiven=0
   reportDir=""
   reportFormat=""
   printMode="normal"
   dryRun=0
   testType="normal"
   rsrvPortBegin="26000"
   rsrvPortEnd="27000"
   confFile=""
   ciMode=0
   stopFlagSet=0
   failFlag=""
   detectDir=""
   svcnameSet=0
   dbpathSet=0
   hostnameSet=0
   selectArgs=()
   selected=()
   sconsNames=()
   sconsSources=()
   testcases=()
   reportArtifacts=()
   skipNames=()
   onlyNames=()
   hookConsume=-1

   command -v realpath >/dev/null 2>&1 || reportError "realpath not found in PATH"
   command -v timeout >/dev/null 2>&1 || reportError "timeout not found in PATH"

   function applyReportInfo()
   {
      local item key value
      local IFS=';'
      for item in $1
      do
         [ -n "$item" ] || continue
         key=${item%%=*}
         value=${item#*=}
         case $key in
            execute_host) EXECUTE_HOST_REPORT=$value; export EXECUTE_HOST_REPORT ;;
            diaglog_url) DIAGLOG_URL=$value; export DIAGLOG_URL ;;
            diaglog_base) DIAGLOG_BASE=$value; export DIAGLOG_BASE ;;
            cluster_hosts) TESTHOST_LIST=$value; export TESTHOST_LIST ;;
            test_type) TEST_TYPE=$value; export TEST_TYPE ;;
            deploy_mode) DEPLOY_MODE=$value; export DEPLOY_MODE ;;
            build_number) BUILD_NUMBER=$value; export BUILD_NUMBER ;;
         esac
      done
   }

   while [ "$1" != "" ]
   do
      case $1 in
         --no-up | -noup )
            needUpdate=0
            ;;
         --no-compile | -nocompile )
            needCompile=0
            ;;
         --release | -release )
            isRelease=1
            ;;
         --conf | -conf )
            option=$1
            shift
            [ -n "$1" ] || reportError "missing value for $option"
            confFile=$1
            ;;
         --ci | -ci )
            ciMode=1
            ;;
         -h | -hostname )
            shift
            [ -n "${1:-}" ] || reportError "missing value for -h"
            hostname=$1
            hostnameSet=1
            ;;
         -n | -svcname )
            shift
            [ -n "${1:-}" ] || reportError "missing value for -n"
            svcname=$1
            svcnameSet=1
            ;;
         -user | -passwd )
            option=$1
            shift
            [ -n "${1:-}" ] || reportError "missing value for $option"
            case $option in
               -user ) user=$1 ;;
               -passwd ) passwd=$1 ;;
            esac
            ;;
         --db-path | -dbpath )
            shift
            [ -n "${1:-}" ] || reportError "missing value for --db-path"
            dbpath=$1
            dbpathSet=1
            ;;
         -s | -stopFlag )
            shift
            [ -n "${1:-}" ] || reportError "missing value for -s"
            stopFlag=$1
            stopFlagSet=1
            ;;
         --fail-flag | -fail-flag )
            shift
            [ -n "${1:-}" ] || reportError "missing value for $1"
            failFlag=$1
            ;;
         -m | --mode )
            shift
            [ -n "${1:-}" ] || reportError "missing value for -m"
            TEST_TYPE=$1
            export TEST_TYPE
            case ${1,,} in
               normal | normaltest ) testType="normal" ;;
               smoke | smoketest ) testType="smoke" ;;
               sync | synctest ) testType="sync" ;;
               configure | configuretest ) testType="configure" ;;
               all ) testType="all" ;;
            esac
            ;;
         --install-dir | -install-dir )
            shift
            [ -n "${1:-}" ] || reportError "missing value for --install-dir"
            INSTALL_DIR=$1
            export INSTALL_DIR
            ;;
         --testcase-type )
            shift
            [ -n "${1:-}" ] || reportError "missing value for --testcase-type"
            TESTCASE_TYPE=$1
            export TESTCASE_TYPE
            ;;
         --reportinfo )
            shift
            [ -n "${1:-}" ] || reportError "missing value for --reportinfo"
            applyReportInfo "$1"
            ;;
         --detect-dir | -detect-dir )
            shift
            [ -n "${1:-}" ] || reportError "missing value for $1"
            detectDir=$1
            ;;
         -f | -file )
            shift
            [ -n "${1:-}" ] || reportError "missing value for -f"
            selectArgs+=("file:$1")
            ;;
         -p | -dir )
            shift
            [ -n "${1:-}" ] || reportError "missing value for -p"
            selectArgs+=("dir:$1")
            ;;
         --case-dir | -case-dir )
            shift
            [ -n "${1:-}" ] || reportError "missing value for --case-dir"
            caseDirInput=$1
            caseDirGiven=1
            ;;
         --report-dir | -report-dir )
            shift
            [ -n "${1:-}" ] || reportError "missing value for --report-dir"
            reportDir=$1
            ;;
         --reportFormat | -reportFormat )
            shift
            [ -n "${1:-}" ] || reportError "missing value for --reportFormat"
            reportFormat=${1,,}
            ;;
         -print )
            if [ "${2:-}" = "failed" ]; then
               printMode=failed
               shift
            elif [ -n "${2:-}" ] && [[ "$2" != -* ]]; then
               reportError "-print supports failed or bare -print"
            else
               printMode=full
            fi
            ;;
         -test-type )
            shift
            [ -n "${1:-}" ] || reportError "missing value for -test-type"
            testType=$1
            ;;
         --use-ssl | -usessl )
            useSSL=1
            ;;
         --test | -test )
            runTests=1
            ;;
         --dry-run | -dry-run )
            dryRun=1
            ;;
         --help )
            display
            exit 0
            ;;
         * )
            hookParseOption "$1" "${2:-}"
            if [ "$hookConsume" -lt 0 ]; then
               reportError "unknown ${RUNNER_KIND} runner option: $1"
            fi
            if [ "$hookConsume" -eq 1 ]; then
               shift
            fi
            ;;
      esac
      shift
   done

   if [ "$stopFlag" != "0" ] && [ "$stopFlag" != "1" ]; then
      reportError "-s only supports 0 or 1"
   fi
   if [ "$printMode" != "normal" ] && [ "$printMode" != "failed" ] && [ "$printMode" != "full" ]; then
      reportError "-print supports failed or bare -print"
   fi
   if [ -n "$reportFormat" ] && [ "$reportFormat" != "jenkins" ]; then
      reportError "-reportFormat only supports jenkins"
   fi
   if [ -n "$confFile" ]; then
      loadConfFile "$confFile"
   fi
   if [ $ciMode -eq 1 ]; then
      needUpdate=0
      needCompile=0
      runTests=1   # CI 命令不带 -test，-ci 本身就是执行
      testType="all"
      useSSL=1
      reportFormat="jenkins"
      if [ $stopFlagSet -eq 0 ]; then
         stopFlag=0
      fi
      hookApplyCi
   fi
   if [ $hostnameSet -eq 0 ]; then
      hostname=${conf_DB_HOSTNAME:-$hostname}
   fi
   if [ $svcnameSet -eq 0 ]; then
      svcname=${SVCNAME:-${conf_SVCNAME:-$svcname}}
   fi
   if [ -z "${INSTALL_DIR:-}" ] && [ -n "${conf_INSTALL_DIR:-}" ]; then
      INSTALL_DIR=$conf_INSTALL_DIR
   fi
   if [ $dbpathSet -eq 0 ]; then
      if [ -n "${conf_RSRVNODEDIR:-}" ]; then
         dbpath=$conf_RSRVNODEDIR
      elif [ -n "${INSTALL_DIR:-}" ]; then
         dbpath="$INSTALL_DIR/database/"
      fi
   fi
   case $testType in
      normal | smoke | sync | configure | all ) ;;
      * ) reportError "-test-type supports normal, smoke, sync, configure or all" ;;
   esac

   if [ -n "${INSTALL_DIR:-}" ]; then
      prependLibPath "$INSTALL_DIR/lib"
   fi
   if [ -e "$PROJECT_ROOT/client/lib/$CLIENT_LIB" ]; then
      prependLibPath "$PROJECT_ROOT/client/lib"
   fi
   hookValidate
   local hasFile=0 hasDir=0 item
   for item in "${selectArgs[@]}"
   do
      case $item in
         file:* ) hasFile=1 ;;
         dir:* ) hasDir=1 ;;
      esac
   done
   if [ $hasFile -eq 1 ] && [ $hasDir -eq 1 ]; then
      reportError "-p and -f are mutually exclusive"
   fi
   if [ $dryRun -eq 1 ]; then
      needUpdate=0
   fi
   if [ $needUpdate -eq 1 ]; then
      svnUp
   fi
   resolveSelection
   hookSelect
   if [ $needCompile -eq 1 ]; then
      compile
   fi
   if [ $runTests -eq 0 ]; then
      echo " [ Execute ] skipped: pass -test to run test programs"
      exit 0
   fi
   if [ $caseDirGiven -eq 0 ] && [ ! -d "$SRC_DIR/build_test" ]; then
      if [ ${#selected[@]} -gt 0 ]; then
         caseDirInput="$SRC_DIR/build_test"
      elif [ -f "$SRC_DIR/SConstruct" ] && [ $dryRun -eq 0 ]; then
         reportError "build_test not found: compile first or pass -case-dir"
      elif [ $dryRun -eq 1 ]; then
         caseDirInput="$SRC_DIR/build_test"
      else
         caseDirInput="$SRC_DIR"
      fi
   fi

   caseDir=$(makeAbsolutePath "$caseDirInput")
   if [ -z "$reportDir" ]; then
      reportDir=$(defaultReportDir)
   fi
   reportDir=$(makeAbsolutePath "$reportDir")
   if [ ! -d "$caseDir" ] && [ ${#selected[@]} -eq 0 ] && [ $dryRun -eq 0 ]; then
      reportError "case directory not found: $caseDir"
   fi

   if [ ${#selected[@]} -eq 0 ]; then
      loadTypeLists
   fi
   discoverTestcases
   if [ ${#selected[@]} -gt 0 ]; then
      :
   elif [ "$testType" = "normal" ] && [ ${#skipNames[@]} -gt 0 ]; then
      echo " [ Filter ] test-type=normal, skip ${#skipNames[@]} names from testcase.conf (CI normal_test)"
   elif [ "$testType" != "normal" ] && [ "$testType" != "all" ]; then
      echo " [ Filter ] test-type=$testType, ${#onlyNames[@]} names from testcase.conf ${testType}_list"
   fi
   if [ ${#testcases[@]} -eq 0 ]; then
      echo " [ Execute ] 0 $CASE_TYPE test program(s)"
      exit 3
   fi
   autoTest
   exit $?
}
