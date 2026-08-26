<template>
  <el-dialog
  :title="dialogTitle"
  :visible.sync="visible"
  width="700px"
  :close-on-click-modal="false"
  :append-to-body="true"
  @close="handleDialogClose"
>
<el-form
    :model="taskData"
    :rules="rules"
    ref="taskForm"
    label-width="130px"
    size="small"
    :inline-message="true"
  >
      <!-- 基本信息 -->
      <el-form-item label="任务名称" prop="name">
        <el-input v-model="taskData.name"></el-input>
      </el-form-item>

      <el-form-item label="描述" prop="desc">
        <el-input v-model="taskData.desc"></el-input>
      </el-form-item>

      <el-form-item label="任务类型" prop="type">
        <el-select v-model="taskData.type" placeholder="请选择任务类型" :disabled="isEdit">
          <el-option
            v-for="item in taskTypeOptions"
            :key="item.value"
            :label="item.label"
            :value="item.value"
          ></el-option>
        </el-select>
      </el-form-item>

      <el-form-item label="任务简介" v-if="taskData.type">
        <span>{{getScheduleIntroduction(taskData.type)}}</span>
      </el-form-item>

      <el-form-item label="cron表达式" prop="cron">
        <el-input
          id="input_schedule_cron"
          v-model="taskData.cron"
          maxlength="100"
          placeholder="请输入cron表达式"
        >
          <template slot="suffix">
            <el-tooltip
              class="item"
              effect="dark"
              :content="'点击' + (showCronPicker ? '隐藏' : '显示') + 'Cron快捷生成工具'"
              placement="top"
            >
              <i
                class="el-icon-question pointer-cursor no-select"
                @click="showCronPicker = !showCronPicker"
              ></i>
            </el-tooltip>
          </template>
        </el-input>
      </el-form-item>

      <el-form-item label="cron快捷生成" v-if="showCronPicker">
        <cron-picker  ref="cronPicker" :cron="taskData.cron" @change="onCronChange" />
      </el-form-item>

      <el-form-item label="任务内容"></el-form-item>

      <!-- 任务内容卡片 -->
      <el-card shadow="hover" class="content-card" v-if="taskData.type && taskData.type !== 'clean'">
        <el-row :gutter="20">
          <!-- 源站点 -->
          <el-col :span="12">
            <el-form-item label="源站点">
              <el-select
                v-model="taskData.content.source_site"
                filterable
                remote
                reserve-keyword
                placeholder="输入搜索源站点"
                :remote-method="searchSites"
                :loading="loadingSites"
                @focus="loadAllSites"
              >
                <el-option
                  v-for="item in filteredSourceSites"
                  :key="item.id"
                  :label="item.name"
                  :value="item.name"
                ></el-option>
              </el-select>
            </el-form-item>
          </el-col>

          <!-- 目标站点 -->
          <el-col :span="12">
            <el-form-item label="目标站点">
              <el-select
                v-model="taskData.content.target_site"
                filterable
                remote
                reserve-keyword
                placeholder="输入搜索目标站点"
                :remote-method="searchSites"
                :loading="loadingSites"
                @focus="loadAllSites"
              >
                <el-option
                  v-for="item in filteredTargetSites"
                  :key="item.id"
                  :label="item.name"
                  :value="item.name"
                ></el-option>
              </el-select>
            </el-form-item>
          </el-col>

          <!-- 数组字段 -->
          <el-col :span="12">
            <el-form-item label="集合空间列表">
              <el-input
                type="textarea"
                v-model="csListStr"
                :autosize="{ minRows: 1, maxRows: 6 }"
                placeholder="eg: cs1,cs2"
              ></el-input>
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item>
              <template slot="label">
                <el-tooltip
                  content="支持 Perl 正则表达式（PCRE），如：doc_.*"
                  placement="top"
                >
                  <span style="cursor: help;">
                    集合空间模糊匹配
                  </span>
                </el-tooltip>
              </template>

              <el-input
                v-model="csRegexStr"
                type="textarea"
                :autosize="{ minRows: 1, maxRows: 6 }"
                placeholder="eg: doc_.*"
              />
            </el-form-item>
          </el-col>
          <el-col :span="12">
            <el-form-item label="集合列表">
              <el-input
                type="textarea"
                v-model="clListStr"
                :autosize="{ minRows: 1, maxRows: 6 }"
                placeholder="eg: cs.cl1,cs.cl2"
              ></el-input>
            </el-form-item>
          </el-col>

          <el-col :span="12">
            <!-- 集合模糊匹配 -->
            <el-form-item>
              <template slot="label">
                <el-tooltip
                  content="支持 Perl 正则表达式（PCRE），例如：cs.file_.*"
                  placement="top"
                >
                  <span style="cursor: help;">
                    集合模糊匹配
                  </span>
                </el-tooltip>
              </template>

              <el-input
                v-model="clRegexStr"
                type="textarea"
                :autosize="{ minRows: 1, maxRows: 6 }"
                placeholder="eg: cs.file_.*"
              />
            </el-form-item>
          </el-col>

          <el-col
            :span="24"
            style="text-align: right; margin-top: 10px;"
            v-if="csRegexStr.trim() !== '' || clRegexStr.trim() !== ''"
          >
            <el-button type="primary" size="mini" @click="previewMatch">
              预览匹配结果
            </el-button>
          </el-col>
          <!-- ✅ 动态条件（迁移 / 数据切换） -->
          <el-col :span="24" v-if="taskData.type === 'transfer'">
            <el-form-item label="迁移条件">
              <div style="display: flex; flex-direction: column; gap: 10px;">

                <!-- 迁移 xxx 天没有数据写入的集合 -->
                <div style="display: flex; align-items: center;">
                  <span>集合超过</span>
                  <el-input-number
                    v-model="taskData.content.no_write_time_threshold"
                    :min="0"
                    style="margin: 0 8px; width: 120px;"
                  ></el-input-number>
                  <span>天没数据写入才迁移记录</span>
                </div>

                <!-- 迁移创建时间超过 xxx 天的集合 -->
                <div style="display: flex; align-items: center;">
                  <span>迁移创建时间超过</span>
                  <el-input-number
                    v-model="taskData.content.cl_create_time_threshold"
                    :min="0"
                    style="margin: 0 8px; width: 120px;"
                  ></el-input-number>
                  <span>天的集合</span>
                </div>

                <div style="display: flex; align-items: center;">
                  <span>删除目标站点多余的数据：</span>
                  <el-switch
                    v-model="taskData.content.delete_more_lob_in_target"
                    style="margin-left: 10px;"
                  ></el-switch>
                </div>
                <div>
                  <span>分区中断</span>
                  <el-tooltip
                    content="分区中断是指当目标分区不满足迁移条件或迁移失败时，则该分区之后的分区将跳过迁移。"
                    placement="top"
                  >
                    <i
                      class="el-icon-info"
                      style="margin-left: 6px; cursor: pointer; color: #909399;"
                    ></i>
                  </el-tooltip>
                  <el-switch
                    v-model="taskData.content.partition_interruption"
                    style="margin-left: 10px;"
                  ></el-switch>
                </div>
                <div style="display: flex; align-items: center;">
                  <span style="width: 100px;">目标数据域：</span>
                  <el-input
                    v-model="taskData.content.data_domain" style="width: 120px;"
                  ></el-input>
                </div>

              </div>
            </el-form-item>
          </el-col>

          <el-col :span="24" v-else-if="taskData.type === 'data_switch'">
            <el-form-item label="切换条件">
              <div style="display: flex; flex-direction: column; gap: 10px;">

                <div style="display: flex; align-items: center;">
                  <span>切换</span>
                  <el-input-number
                    v-model="taskData.content.no_write_time_threshold"
                    :min="0"
                    style="margin: 0 8px; width: 120px;"
                  ></el-input-number>
                  <span>天没数据写入且已完成迁移的集合</span>
                </div>

                <div style="display: flex; align-items: center;">
                  <span>切换创建时间超过</span>
                  <el-input-number
                    v-model="taskData.content.cl_create_time_threshold"
                    :min="0"
                    style="margin: 0 8px; width: 120px;"
                  ></el-input-number>
                  <span>天的集合</span>
                </div>

                <div>
                  <span>分区中断</span>
                  <el-tooltip
                    content="分区中断是指当目标分区不满足数据切换条件或数据切换失败时，则该分区之后的分区将跳过数据切换。"
                    placement="top"
                  >
                    <i
                      class="el-icon-info"
                      style="margin-left: 6px; cursor: pointer; color: #909399;"
                    ></i>
                  </el-tooltip>
                  <el-switch
                    v-model="taskData.content.partition_interruption"
                    style="margin-left: 10px;"
                  ></el-switch>
                </div>
              </div>
            </el-form-item>
          </el-col>

          <el-col :span="24">
            <el-form-item label="最大执行时间">
              <el-input-number
                v-model="maxExecValue"
                :min="0"
                style="width: 200px;"
              ></el-input-number>
              <el-select v-model="maxExecUnit" placeholder="单位" style="width: 100px; margin-left: 10px;">
                <el-option label="毫秒" value="ms"></el-option>
                <el-option label="秒" value="s"></el-option>
                <el-option label="分钟" value="m"></el-option>
                <el-option label="小时" value="h"></el-option>
              </el-select>
              <span style="margin-left:8px;color:#909399;font-size:12px">
                0 表示不限制
              </span>
            </el-form-item>
          </el-col>
        </el-row>
        <el-form-item
          v-if="taskData.type === 'transfer' && !isEdit"
          label="后续操作"
        >
          <el-switch
            v-model="generateDataSwitchTask"
            active-text="创建成功后，生成数据切换任务"
          />
        </el-form-item>
      </el-card>


      <el-card shadow="hover" class="content-card" v-if="taskData.type === 'clean'">
        <div
          v-for="range in taskData.content.clean_range"
          :key="range.id"
          class="clean-range-card"
        >
          <h4>
            <span>清理范围 {{ taskData.content.clean_range.indexOf(range) + 1 }}</span>
            <el-button
              type="danger"
              size="mini"
              v-if="taskData.content.clean_range.length > 1"
              @click="removeCleanRange(range.id)"
            >
              删除此范围
            </el-button>
          </h4>

          <el-row :gutter="20">
            <!-- 清理站点 -->
            <el-col :span="24">
              <el-form-item label="站点匹配方式">
                <el-radio-group v-model="range.match_mode">
                  <el-radio label="site">精确匹配</el-radio>
                  <el-radio label="regex">模糊匹配</el-radio>
                </el-radio-group>
              </el-form-item>
            </el-col>

            <el-col :span="24" v-if="range.match_mode && range.match_mode === 'site'">
              <el-form-item label="清理站点">
                <el-select
                  v-model="range.clean_site"
                  filterable
                  remote
                  reserve-keyword
                  placeholder="请选择站点"
                  :remote-method="searchSites"
                  :loading="loadingSites"
                  @focus="loadAllSites"
                >
                  <el-option
                    v-for="item in allSites"
                    :key="item.id"
                    :label="item.name"
                    :value="item.name"
                  />
                </el-select>
              </el-form-item>
            </el-col>

            <el-col :span="15" v-if="range.match_mode && range.match_mode === 'regex'">
              <el-form-item label="站点名模糊匹配">
                <el-input
                  v-model="range.clean_site_regex"
                  placeholder="如：.* 表示匹配所有站点"
                ></el-input>
              </el-form-item>
            </el-col>

            <!-- 集合空间列表 -->
            <el-col :span="12">
              <el-form-item label="集合空间列表">
                <el-input
                  type="textarea"
                  v-model="range.csListStr"
                  :autosize="{ minRows:1, maxRows:6 }"
                  placeholder="eg: cs1,cs2"
                ></el-input>
              </el-form-item>
            </el-col>

            <!-- 集合空间正则 -->
            <el-col :span="12">
              <el-form-item>
                <template slot="label">
                  <el-tooltip content="支持正则匹配，如：cs_.*" placement="top">
                    <span style="cursor: help;">集合空间模糊匹配</span>
                  </el-tooltip>
                </template>
                <el-input
                  type="textarea"
                  v-model="range.csRegexStr"
                  :autosize="{ minRows:1, maxRows:6 }"
                  placeholder="eg: cs_.*"
                ></el-input>
              </el-form-item>
            </el-col>

            <!-- 集合列表 -->
            <el-col :span="12">
              <el-form-item label="集合列表">
                <el-input
                  type="textarea"
                  v-model="range.clListStr"
                  :autosize="{ minRows:1, maxRows:6 }"
                  placeholder="eg: cs.cl1,cs.cl2"
                ></el-input>
              </el-form-item>
            </el-col>

            <!-- 集合正则 -->
            <el-col :span="12">
              <el-form-item>
                <template slot="label">
                  <el-tooltip content="支持正则匹配，如：cs.file_.*" placement="top">
                    <span style="cursor: help;">集合模糊匹配</span>
                  </el-tooltip>
                </template>
                <el-input
                  type="textarea"
                  v-model="range.clRegexStr"
                  :autosize="{ minRows:1, maxRows:6 }"
                  placeholder="eg: cs.file_.*"
                ></el-input>
              </el-form-item>
            </el-col>

            <!-- 最大保留天数 -->
            <el-col :span="12">
              <el-form-item label="最大保留天数">
                <el-input-number
                  v-model="range.max_retention_days"
                ></el-input-number>
              </el-form-item>
            </el-col>

          </el-row>
        </div>

        <el-button type="primary" size="mini" @click="addCleanRange">
          + 添加清理范围
        </el-button>

        <el-form-item label="最大执行时间">
          <el-input-number
            v-model="maxExecValue"
            :min="0"
            style="width: 200px;"
          ></el-input-number>
          <el-select v-model="maxExecUnit" placeholder="单位" style="width: 100px; margin-left: 10px;">
            <el-option label="毫秒" value="ms"></el-option>
            <el-option label="秒" value="s"></el-option>
            <el-option label="分钟" value="m"></el-option>
            <el-option label="小时" value="h"></el-option>
          </el-select>
          <span style="margin-left:8px;color:#909399;font-size:12px">
            0 表示不限制
          </span>
        </el-form-item>
      </el-card>
    </el-form>

    <span slot="footer" class="dialog-footer">
      <el-button @click="$emit('update:visible', false)">取消</el-button>
      <el-button type="primary" @click="submitTask">{{ submitText }}</el-button>
    </span>

    <!-- 调用子组件 -->
    <match-result-drawer
      :visible.sync="matchDrawerVisible"
      :data="matchResultData"
    />
  </el-dialog>
</template>

<script>
import { querySiteList } from '@/api/site'
import { createSchedule, updateSchedule, previewCSCLMatch } from '@/api/schedule'
import debounce from 'lodash/debounce'
import MatchResultDrawer from './matchResultDrawer.vue'
import CronPicker from '@/components/common/CronPicker/index.vue'
export default {
  name: "ScheduleDialog",
  props: {
    visible: { type: Boolean, default: false },
    task: { type: Object, default: () => ({}) },
    dialogTitle: { type: String, default: "任务" },
    submitText: { type: String, default: "确定" }
  },
  components: {
    CronPicker,
    MatchResultDrawer,
  },
  data() {
    return {
      taskData: this.cloneTask(this.task),
      showCronPicker: true,
      csListStr: "",
      clListStr: "",
      csRegexStr: "",
      clRegexStr: "",
      allSites: [],
      loadingSites: false,
      maxExecValue: 1,
      maxExecUnit: 'h',
      taskTypeOptions: [
        { label: '迁移任务', value: 'transfer' },
        { label: '数据切换任务', value: 'data_switch' },
        { label: '清理任务', value: 'clean' }
      ],
      rules: {
        name: [{ required: true, message: "任务名称必填", trigger: "blur" }],
        type: [{ required: true, message: "请选择任务类型", trigger: "change" }],
        cron: [{ required: true, message: "cron表达式必填", trigger: "blur" }]
      },
      matchDrawerVisible: false,
      matchResultData: {
        cs: [],
        cl: []
      },
      generateDataSwitchTask: true
    };
  },
  computed: {
    isEdit() {
      return !!this.task.id;
    },
    filteredSourceSites() {
      return this.allSites.filter(
        s => s.name !== this.taskData.content.target_site
      );
    },
    filteredTargetSites() {
      return this.allSites.filter(
        s => s.name !== this.taskData.content.source_site
      );
    }
  },
  watch: {
    visible(val) {
      if (!val) return;

      if (!this.task.id && !this.task.__fromTransfer) {
        const empty = this.cloneTask({});

        this.taskData = empty;

        this.csListStr = "";
        this.clListStr = "";
        this.csRegexStr = "";
        this.clRegexStr = "";

        // 最大执行时间 reset
        this.maxExecValue = 1;
        this.maxExecUnit = 'h';

        // 后续操作开关恢复默认
        this.generateDataSwitchTask = true;

        return;
      }

      const newTask = this.cloneTask(this.task);

      this.taskData = newTask;

      if (
        newTask.type === 'transfer' ||
        newTask.type === 'data_switch'
      ) {
        this.initArrayStrings();
      }

      if (newTask.type === 'clean') {
        this.initCleanRangeStrings();
      }

      this.convertMaxExecTime(newTask.content.max_exec_time);
    }
  },
  created() {
    this.searchSites = debounce(this.searchSites, 300);
  },
  methods: {
    buildDataSwitchDraft(transferTask) {
      const draft = JSON.parse(JSON.stringify(transferTask));

      draft.id = undefined;
      draft.type = 'data_switch';
      draft.name = `${transferTask.name}_数据切换任务`;
      draft.desc = `由迁移任务【${transferTask.name}】生成`;

      // data_switch 不需要的字段
      delete draft.content.delete_more_lob_in_target;
      delete draft.content.data_domain;
      draft.__fromTransfer = true;

      return draft;
    },
    initCleanRangeStrings() {
      if (!this.taskData.content.clean_range) return;
      this.taskData.content.clean_range = this.taskData.content.clean_range.map(item => {
        return {
          ...item,
          csListStr: (item.cs_list || []).join(','),
          clListStr: (item.cl_list || []).join(','),
          csRegexStr: (item.cs_regex || []).join(','),
          clRegexStr: (item.cl_regex || []).join(','),
          match_mode: item.clean_site ? 'site' :
                  (item.clean_site_regex ? 'regex' : 'site')  // 默认 site
        }
      })
    },
    addCleanRange() {
      if(this.taskData.content.clean_range === undefined) {
        this.taskData.content.clean_range = []
      }
      this.taskData.content.clean_range.push({
        id: Date.now(),  // 唯一标识
        match_mode: "site",
        clean_site: "",
        clean_site_regex: "",
        clListStr: "",
        csListStr: "",
        csRegexStr: "",
        clRegexStr: "",
        max_retention_days: 7
      });
    },

    removeCleanRange(id) {
      console.log(this.taskData.content.clean_range)
      this.taskData.content.clean_range = this.taskData.content.clean_range.filter(
        range => range.id !== id
      );
    },
    getScheduleIntroduction(type) {
      switch (type) {
        case 'transfer':
          return '迁移任务是将集合的数据从源站点迁移到目标站点(数据源集群)上，支持增量迁移 LOB 数据；不支持增量迁移记录数据，每次变更记录数据，都将全量迁移记录数据！！！';
        case 'data_switch':
          return '数据切换任务是将已经迁移到目标站点(数据源集群)的集合，通过数据源关联的方式，将源集合的数据访问切换到数据源集群上；在这个过程中，源集合将被重命名，重命名的格式为 {clName}_data_switch_bak_{time}';
        case 'clean':
          return '清理任务是将已经完成数据切换，并且被数据切换任务重名名的源集合删除，重命名的格式为 {clName}_data_switch_bak_{time}';
        default:
          return type || '-';
      }
    },
    async previewMatch() {
      if (!this.taskData.content.source_site) {
        this.$message.warning("请先选择源站点");
        return;
      }

      try {
        const filter = {
          site: this.taskData.content.source_site,
          cs_regex: this.csRegexStr.split(",").map(s => s.trim()).filter(Boolean),
          cl_regex: this.clRegexStr.split(",").map(s => s.trim()).filter(Boolean)
        };

        const res = await previewCSCLMatch(filter);
        this.matchResultData = {
          cs: res.data.cs || [],
          cl: res.data.cl || []
        };
        this.matchDrawerVisible = true;

      } catch (err) {
        console.error("❌ 匹配失败:", err);
        this.$message.error("查询失败，请检查正则表达式");
      }
    },
    convertMaxExecTime(ms) {
      if (!ms || ms === 0) {
        this.maxExecUnit = 'ms';
        this.maxExecValue = 0;
        return;
      }

      // 能整除 1 小时
      if (ms % (3600 * 1000) === 0) {
        this.maxExecUnit = 'h';
        this.maxExecValue = ms / (3600 * 1000);
      }
      // 能整除 1 分钟
      else if (ms % (60 * 1000) === 0) {
        this.maxExecUnit = 'm';
        this.maxExecValue = ms / (60 * 1000);
      }
      // 能整除 1 秒
      else if (ms % 1000 === 0) {
        this.maxExecUnit = 's';
        this.maxExecValue = ms / 1000;
      }
      // 默认毫秒
      else {
        this.maxExecUnit = 'ms';
        this.maxExecValue = ms;
      }
    },
    onCronChange(res) {
      this.taskData.cron = res.cron
    },
    handleDialogClose() {
    // 清空源站点和目标站点
    this.taskData.content.source_site = '';
    this.taskData.content.target_site = '';
    this.$emit('update:visible', false); // ✅ 通知父组件关闭
  },
    cloneTask(task) {
      const defaultContent = {
        source_site: "",
        target_site: "",
        cs_list: [],
        cs_regex: [],
        cl_list: [],
        cl_regex: [],
        no_write_time_threshold: 30,
        delete_more_lob_in_target: false,
        partition_interruption: true,
        cl_create_time_threshold: 30,
        max_exec_time: 0,
        clean_range: [
        {
          id: Date.now(),
          clean_site: "",
          clean_site_regex: ".*",
          cl_list: [],
          cs_list: [],
          cs_regex: [],
          cl_regex: [],
          max_retention_days: 7
        }]
      };
      return Object.assign(
        { name: "", desc: "", type: "", cron: "", enable: true, content: defaultContent },
        JSON.parse(JSON.stringify(task || {}))
      );
    },

    initArrayStrings() {
      this.csListStr = (this.taskData.content.cs_list || []).join(",");
      this.clListStr = (this.taskData.content.cl_list || []).join(",");
      this.csRegexStr = (this.taskData.content.cs_regex || []).join(",");
      this.clRegexStr = (this.taskData.content.cl_regex || []).join(",");
    },

    async loadAllSites() {
      if (this.allSites.length) return;
      await this.fetchSites('');
    },

    async searchSites(query) {
      await this.fetchSites(query);
    },

    async fetchSites(keyword) {
      this.loadingSites = true;
      try {
        const res = await querySiteList(1, 20, keyword);
        const sites = res.data
        this.allSites = sites.map(s => ({ id: s.name, name: s.name }));
      } catch (e) {
        console.error('❌ 获取站点失败', e);
        this.allSites = [];
      } finally {
        this.loadingSites = false;
      }
    },

    submitTask() {
      if (this.taskData.type === 'clean') {

        if (!this.taskData.content.clean_range || this.taskData.content.clean_range.length === 0) {
          this.$message.warning("清理任务必须至少包含一个清理范围");
          return;
        }

        for (const r of this.taskData.content.clean_range) {
          // --- 1. 校验匹配方式 ---
          if (!r.match_mode) {
            this.$message.warning("请选择站点匹配方式（精确匹配 / 正则匹配）");
            return;
          }

          // --- 2. 根据匹配方式校验字段 ---
          if (r.match_mode === "site") {
            if (!r.clean_site) {
              this.$message.warning("精确匹配时必须填写“清理站点”");
              return;
            }
            r.clean_site_regex = null;
          }

          if (r.match_mode === "regex") {
            if (!r.clean_site_regex) {
              this.$message.warning("正则匹配时必须填写“站点正则”");
              return;
            }
            r.clean_site = null;
          }

          // --- 3. 转化列表类输入 ---
          r.cs_list = r.csListStr
            ? r.csListStr.split(",").map(s => s.trim()).filter(Boolean)
            : [];

          r.cl_list = r.clListStr
            ? r.clListStr.split(",").map(s => s.trim()).filter(Boolean)
            : [];

          r.cs_regex = r.csRegexStr
            ? r.csRegexStr.split(",").map(s => s.trim()).filter(Boolean)
            : [];

          r.cl_regex = r.clRegexStr
            ? r.clRegexStr.split(",").map(s => s.trim()).filter(Boolean)
            : [];

          // --- 4. 至少要指定集合范围 ---
          const noCollection =
            (!r.cs_list.length) &&
            (!r.cl_list.length) &&
            (!r.cs_regex.length) &&
            (!r.cl_regex.length);

          if (noCollection) {
            this.$message.warning(
              "每个清理范围至少需要填写一个：集合空间列表、集合列表、集合空间正则、集合正则"
            );
            return;
          }

          // --- 5. 默认值保护 ---
          if (r.max_retention_days < 0) {
            this.$message.warning(
              "最大保留时间不能小于 0 天"
            );
            return;
          }

          delete r.csListStr;
          delete r.clListStr;
          delete r.csRegexStr;
          delete r.clRegexStr;
          delete r.match_mode;
          delete r.id;
        }
        delete this.taskData.content.cs_list;
        delete this.taskData.content.cl_list;
        delete this.taskData.content.cs_regex;
        delete this.taskData.content.cl_regex;
        delete this.taskData.content.source_site;
        delete this.taskData.content.target_site;
        delete this.taskData.content.cl_create_time_threshold;
        delete this.taskData.content.no_write_time_threshold;
        delete this.taskData.content.delete_more_lob_in_target;
        delete this.taskData.content.partition_interruption;
      }
      else {
        delete this.taskData.content.clean_range;
        this.taskData.content.cs_list = this.csListStr.split(",").map(s => s.trim()).filter(Boolean);
        this.taskData.content.cl_list = this.clListStr.split(",").map(s => s.trim()).filter(Boolean);
        this.taskData.content.cs_regex = this.csRegexStr.split(",").map(s => s.trim()).filter(Boolean);
        this.taskData.content.cl_regex = this.clRegexStr.split(",").map(s => s.trim()).filter(Boolean);
        if (!this.taskData.content.source_site) {
          this.$message.warning("请先选择源站点");
          return;
        }
        if (!this.taskData.content.target_site) {
          this.$message.warning("请先选择目标站点");
          return;
        }

        const { cs_list, cl_list, cs_regex, cl_regex } = this.taskData.content;
        const allEmpty =
          (!cs_list || cs_list.length === 0) &&
          (!cl_list || cl_list.length === 0) &&
          (!cs_regex || cs_regex.length === 0) &&
          (!cl_regex || cl_regex.length === 0);

        if (allEmpty) {
          this.$message.warning("以下项请至少填写一个：集合空间列表、集合列表、集合空间模糊匹配 或 集合模糊匹配");
          return;
        }
      }

      // 最大执行时间单位转换成毫秒
      let multiplier = 1;
      if (this.maxExecUnit === 's') multiplier = 1000;
      else if (this.maxExecUnit === 'm') multiplier = 60 * 1000;
      else if (this.maxExecUnit === 'h') multiplier = 60 * 60 * 1000;
      this.taskData.content.max_exec_time = this.maxExecValue * multiplier;

      this.$refs.taskForm.validate(valid => {
        if (!valid) {
          this.$message.warning("请填写完整任务信息");
          return;
        }
        var apiCall;

        // 判断是否编辑
        if (this.taskData.id) {
          apiCall = updateSchedule(this.taskData.id, this.taskData);
        } else {
          apiCall = createSchedule(this.taskData);
        }

        apiCall
          .then(() => {
            if (this.taskData.id) {
              this.$message.success("任务修改成功");
            } else {
              this.$message.success("任务创建成功");
              if (
                this.taskData.type === 'transfer' &&
                this.generateDataSwitchTask
              ) {
                const draft = this.buildDataSwitchDraft(this.taskData);
                this.$emit('create-data-switch', draft);
              }
            }
            this.$emit("update:visible", false);
            this.$emit("refreshList");
          })
          .catch(err => {
            console.error("❌ 提交任务失败:", err);
            this.$message.error("任务提交失败，请重试");
          });
      });
    }
  }
};
</script>

<style scoped>
.scrollable-content {
  max-height: 300px;
  overflow-y: auto;
  padding-right: 10px;
}
.content-card {
  margin-top: 15px;
  padding: 15px 20px;
  border-radius: 8px;
}
.el-form-item {
  margin-bottom: 8px;
}

/* 清理范围卡片容器 */
.clean-range-card {
  border: 1px solid #e5e5e5;
  border-radius: 10px;
  padding: 20px;
  margin-bottom: 20px;
  background: #ffffff;
  transition: all 0.3s ease;
  box-shadow: 0 2px 5px rgba(0,0,0,0.05);
}

/* 悬停高亮效果 */
.clean-range-card:hover {
  box-shadow: 0 4px 12px rgba(0,0,0,0.1);
  border-color: #dcdfe6;
}

/* 标题样式 */
.clean-range-card h4 {
  margin: 0 0 15px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-weight: 600;
  font-size: 16px;
  color: #409EFF; /* 使用 Element UI 蓝色主色 */
}

/* 删除按钮 */
.clean-range-card h4 .el-button {
  font-size: 12px;
}

/* 内部表单项间距 */
.clean-range-card .el-row {
  margin-bottom: 0;
}

.clean-range-card .el-col {
  margin-bottom: 15px;
}

/* 统一文本框和数字输入高度 */
.clean-range-card .el-input,
.clean-range-card .el-input-number {
  width: 100%;
}

.section-title {
  font-size: 14px;       /* 原来可能是 18px 或更大，可以调小 */
  font-weight: 500;      /* 不用太粗 */
  color: #303133;        /* Element UI 默认文字颜色 */
  margin-bottom: 15px;
}

/* 清理范围卡片内的标题，如“清理范围 1” */
.clean-range-card h4 {
  font-size: 12px;       /* 小一号字体 */
  font-weight: 500;      /* 中等粗细 */
  color: #409EFF;        /* 蓝色强调，可根据主题调整 */
  margin-bottom: 10px;
}

/* 卡片内 label 对齐和间距 */
.clean-range-card .el-form-item__label {
  font-size: 12px;       /* label 字体更小 */
  color: #606266;
}
</style>
