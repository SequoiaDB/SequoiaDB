<template>
  <div class="app-container">
    <!-- 搜索部分 -->
    <div class="search-box">
      <el-row :gutter="20" type="flex" justify="end">
        <el-col :span="9">
          <el-input
            maxlength="50"
            size="small"
            placeholder="任务名称"
            v-model="searchParams.name"
            @keyup.enter.native="doSearch"
            clearable>
          </el-input>
        </el-col>
        <el-col :span="3">
          <el-button
            @click="doSearch"
            type="primary"
            size="small"
            icon="el-icon-search"
            style="width:100%">
            搜索
          </el-button>
        </el-col>
        <el-col :span="3">
          <el-button
            @click="resetSearch"
            size="small"
            icon="el-icon-circle-close"
            style="width:100%">
            重置
          </el-button>
        </el-col>
      </el-row>
    </div>
    <div>
      <el-button id="btn_schedule_showCreateDialog" @click="handleAddBtnClick" type="primary" size="small" icon="el-icon-plus"  style="margin-bottom:10px">创建调度任务</el-button>

    <schedule-dialog
    :visible.sync="dialogVisible"
      :task="currentTask"
      :dialog-title="dialogTitle"
      :submit-text="submitText"
      @submit="handleSubmitTask"
      @refreshList="fetchData"
      @create-data-switch="handleCreateDataSwitch"
    ></schedule-dialog>
    </div>

    <!-- 表格部分 -->
    <el-table
      v-loading="tableLoading"
      :data="tableData"
      border
      style="width: 100%; margin-top: 10px;">
      <el-table-column prop="name" label="任务名称" show-overflow-tooltip></el-table-column>
      <el-table-column prop="type" label="任务类型" width="120">
        <template slot-scope="scope">
          {{ getTypeLabel(scope.row.type) }}
        </template>
      </el-table-column>
      <el-table-column prop="desc" label="任务描述" show-overflow-tooltip></el-table-column>
      <el-table-column prop="cron" label="执行表达式" show-overflow-tooltip>
        <template slot-scope="scope">
          <el-tooltip
            class="item"
            effect="dark"
            :content="getCronDescription(scope.row.cron)"
            placement="top"
          >
            <span>{{ scope.row.cron }}</span>
          </el-tooltip>
        </template>
      </el-table-column>
      <el-table-column label="启用状态" width="100">
        <template slot-scope="scope">
          <el-switch
            v-model="scope.row.enable"
            active-color="#13ce66"
            inactive-color="#ff4949"
            @change="handleToggleEnable(scope.row)">
          </el-switch>
        </template>
      </el-table-column>
      <el-table-column label="操作" width="350" align="center">
        <template slot-scope="scope">
          <el-button-group>
            <el-button
              type="primary"
              size="mini"
              @click="handleShowDetail(scope.row)">
              查看
            </el-button>
            <el-button id="btn_schedule_showEditdeleteDialog" size="mini" @click="handleEditBtnClick(scope.row)">编辑</el-button>
            <el-button size="mini" type="success" @click="handleViewLogs(scope.row)">
              运行记录
            </el-button>
            <el-button id="btn_schedule_showDeleteDialog" size="mini" type="danger" @click="handleDeleteBtnClick(scope.row)">删除</el-button>
          </el-button-group>
        </template>
      </el-table-column>
    </el-table>

    <!-- 分页 -->
    <el-pagination
      class="pagination"
      background
      layout="total, prev, pager, next, jumper"
      :total="pagination.total"
      :current-page="pagination.current"
      @current-change="handleCurrentChange">
    </el-pagination>

    <!-- 任务详情弹窗 -->
    <el-dialog
      title="调度任务详情"
      :visible.sync="detailDialogVisible"
      width="700px"
      :close-on-click-modal="false"
      custom-class="task-detail-dialog"
    >
      <!-- 基本信息 -->
      <el-card shadow="hover" class="info-card">
        <h3 class="card-title">基本信息</h3>
        <el-row :gutter="20">
          <el-col :span="24"><strong>ID：</strong>{{ currentTask.id }}</el-col>
          <el-col :span="24"><strong>任务简介：</strong>{{ getScheduleIntroduction(currentTask.type) }}</el-col>
          <el-col :span="12"><strong>任务名称：</strong>{{ currentTask.name }}</el-col>
          <el-col :span="12"><strong>类型：</strong>{{ getTypeLabel(currentTask.type) }}</el-col>
          <el-col :span="12"><strong>描述：</strong>{{ currentTask.desc }}</el-col>
          <el-col :span="12"><strong>cron 表达式：</strong>{{ currentTask.cron }}</el-col>
          <el-col :span="12"><strong>启用：</strong>{{ currentTask.enable ? '是' : '否' }}</el-col>
          <el-col :span="12"><strong>创建时间：</strong>{{ formatDate(currentTask.create_time) }}</el-col>
          <el-col :span="12"><strong>更新时间：</strong>{{ formatDate(currentTask.update_time) }}</el-col>
        </el-row>
      </el-card>

      <!-- 任务内容 -->
      <el-card shadow="hover" class="content-card" v-if="currentTask.type !== 'clean'">
        <h3 class="card-title">任务内容</h3>
        <el-row :gutter="20">
          <el-col :span="12"><strong>源站点：</strong>{{ currentTask.content ? currentTask.content.source_site : '-' }}</el-col>
          <el-col :span="12"><strong>目标站点：</strong>{{ currentTask.content ? currentTask.content.target_site : '-' }}</el-col>
          <el-col :span="24" v-if="currentTask.type === 'transfer'">
            <strong>迁移条件：</strong>
            <div style="margin-left: 20px; display: flex; flex-direction: column; gap: 6px;">
              <div>
                <span>目标数据域：</span>
                {{ currentTask.content && currentTask.content.data_domain ? currentTask.content.data_domain : '不指定' }}
              </div>
              <div>
                集合超过 {{ currentTask.content && currentTask.content.no_write_time_threshold || 0 }} 天没数据写入才迁移记录
              </div>
              <div>
                迁移创建时间超过 {{ currentTask.content && currentTask.content.cl_create_time_threshold || 0 }} 天的集合
              </div>
              <div>
                删除目标站点多余的数据：
                {{ currentTask.content && currentTask.content.delete_more_lob_in_target ? '是' : '否' }}
              </div>
              <div>
                分区中断：
                {{ currentTask.content && currentTask.content.partition_interruption ? '是' : '否' }}
              </div>
            </div>
          </el-col>
          <el-col :span="24" v-else-if="currentTask.type === 'data_switch'">
            <strong>切换条件：</strong>
            <div style="margin-left: 20px; display: flex; flex-direction: column; gap: 6px;">
              <div>
                切换 {{ currentTask.content && currentTask.content.no_write_time_threshold || 0 }} 天没数据写入且已完成迁移的集合
              </div>
              <div>
                切换创建时间超过 {{ currentTask.content && currentTask.content.cl_create_time_threshold || 0 }} 天的集合
              </div>
              <div>
                分区中断：
                {{ currentTask.content && currentTask.content.partition_interruption ? '是' : '否' }}
              </div>
            </div>
          </el-col>
          <el-col :span="12">
            <strong>最大执行时间：</strong>
            {{ currentTask.content ? formatMaxExecTime(currentTask.content.max_exec_time) : '-' }}
          </el-col>
          <el-col :span="24">
            <el-tabs type="card" style="margin-top: 10px;">
              <el-tab-pane label="集合空间列表">
                <div class="list-box">
                  <el-tag
                    v-for="(item, idx) in (currentTask.content && currentTask.content.cs_list || [])"
                    :key="idx"
                    size="small"
                    type="info"
                    class="list-tag"
                  >
                    {{ item }}
                  </el-tag>
                  <div v-if="!currentTask.content || !currentTask.content.cs_list || !currentTask.content.cs_list.length" class="empty-text">
                    暂无数据
                  </div>
                </div>
              </el-tab-pane>

              <el-tab-pane label="集合空间模糊匹配">
                <div class="list-box">
                  <el-tag
                    v-for="(item, idx) in (currentTask.content && currentTask.content.cs_regex || [])"
                    :key="idx"
                    size="small"
                    type="warning"
                    class="list-tag"
                  >
                    {{ item }}
                  </el-tag>
                  <div v-if="!currentTask.content || !currentTask.content.cs_regex || !currentTask.content.cs_regex.length" class="empty-text">
                    暂无数据
                  </div>
                </div>
              </el-tab-pane>

              <el-tab-pane label="集合列表">
                <div class="list-box">
                  <el-tag
                    v-for="(item, idx) in (currentTask.content && currentTask.content.cl_list || [])"
                    :key="idx"
                    size="small"
                    type="success"
                    class="list-tag"
                  >
                    {{ item }}
                  </el-tag>
                  <div v-if="!currentTask.content || !currentTask.content.cl_list || !currentTask.content.cl_list.length" class="empty-text">
                    暂无数据
                  </div>
                </div>
              </el-tab-pane>

              <el-tab-pane label="集合模糊匹配">
                <div class="list-box">
                  <el-tag
                    v-for="(item, idx) in (currentTask.content && currentTask.content.cl_regex || [])"
                    :key="idx"
                    size="small"
                    type="danger"
                    class="list-tag"
                  >
                    {{ item }}
                  </el-tag>
                  <div v-if="!currentTask.content || !currentTask.content.cl_regex || !currentTask.content.cl_regex.length" class="empty-text">
                    暂无数据
                  </div>
                </div>
              </el-tab-pane>
            </el-tabs>
          </el-col>
        </el-row>
      </el-card>

      <el-card shadow="hover" class="content-card" v-if="currentTask.type === 'clean'">
        <h3 class="card-title">任务内容</h3>

        <el-row :gutter="20">
          <el-col :span="12">
            <strong>最大执行时间：</strong>
            {{ currentTask.content ? formatMaxExecTime(currentTask.content.max_exec_time) : '-' }}
          </el-col>
          <el-col :span="24">
            <el-collapse>
              <el-collapse-item
                v-for="(range, idx) in currentTask.content.clean_range"
                :key="idx"
                :title="'清理范围 ' + (idx + 1)"
                :name="idx"
              >
                <el-row :gutter="10" style="margin-top: 10px;">
                  <el-col :span="12"><strong>{{range.clean_site ? '指定站点: ' : '匹配站点: '}}</strong>{{ range.clean_site ? range.clean_site : range.clean_site_regex }}</el-col>
                  <el-col :span="12"><strong>最大保留天数：</strong>{{ range.max_retention_days }}</el-col>
                  <el-col :span="24" style="margin-top: 10px;">
                  <el-tabs type="card">
                    <el-tab-pane label="集合空间列表">
                      <div class="list-box">
                        <el-tag
                          v-for="(item, i) in range.cs_list || []"
                          :key="i"
                          size="small"
                          type="info"
                        >
                          {{ item }}
                        </el-tag>
                        <div v-if="!(range.cs_list && range.cs_list.length)" class="empty-text">暂无数据</div>
                      </div>
                    </el-tab-pane>

                    <el-tab-pane label="集合空间模糊匹配">
                      <div class="list-box">
                        <el-tag
                          v-for="(item, i) in range.cs_regex || []"
                          :key="i"
                          size="small"
                          type="warning"
                        >
                          {{ item }}
                        </el-tag>
                        <div v-if="!(range.cs_regex && range.cs_regex.length)" class="empty-text">暂无数据</div>
                      </div>
                    </el-tab-pane>

                    <el-tab-pane label="集合列表">
                      <div class="list-box">
                        <el-tag
                          v-for="(item, i) in range.cl_list || []"
                          :key="i"
                          size="small"
                          type="success"
                        >
                          {{ item }}
                        </el-tag>
                        <div v-if="!(range.cl_list && range.cl_list.length)" class="empty-text">暂无数据</div>
                      </div>
                    </el-tab-pane>

                    <el-tab-pane label="集合模糊匹配">
                      <div class="list-box">
                        <el-tag
                          v-for="(item, i) in range.cl_regex || []"
                          :key="i"
                          size="small"
                          type="danger"
                        >
                          {{ item }}
                        </el-tag>
                        <div v-if="!(range.cl_regex && range.cl_regex.length)" class="empty-text">暂无数据</div>
                      </div>
                    </el-tab-pane>
                  </el-tabs>
                </el-col>

                  <!-- 集合空间（精确匹配） -->

                </el-row>
              </el-collapse-item>
            </el-collapse>
          </el-col>
        </el-row>
      </el-card>

      <span slot="footer" class="dialog-footer">
        <el-button @click="detailDialogVisible = false">关闭</el-button>
      </span>
    </el-dialog>
  </div>
</template>

<script>
import cronstrue from 'cronstrue';
import 'cronstrue/locales/zh_CN';
import ScheduleDialog from "./components/scheduleDialog";
import { queryScheduleList, deleteSchedule, switchSchedule } from '@/api/schedule'

export default {
  components: { ScheduleDialog },
  data() {
    return {
      dialogVisible: false,
      tableLoading: false,
      detailDialogVisible: false,
      currentTask: {},
      searchParams: { name: '' },
      dialogTitle: "",
      submitText: "",
      pagination: { current: 1, size: 10, total: 0 },
      tableData: []
    };
  },
  activated() {
    this.resetPageState()
  },
  methods: {
    resetPageState() {
      this.searchParams.name = ''
      this.pagination.current = 1
      this.fetchData()
    },
    handleCreateDataSwitch(dataSwitchDraft) {
      this.$nextTick(() => {
        this.currentTask = dataSwitchDraft;
        this.dialogTitle = '创建数据切换任务';
        this.submitText = '创建';
        this.dialogVisible = true;
      });
    },
    formatMaxExecTime(ms) {
      if (!ms && ms !== 0) return "-";
      if (ms < 60000) {
        return ms + " ms";
      }
      const minutes = Math.floor(ms / 60000);
      const seconds = Math.floor((ms % 60000) / 1000);
      return `${minutes} 分钟 ${seconds} 秒`;
    },
    getCronDescription(cron) {
      try {
        return cronstrue.toString(cron, { locale: 'zh_CN' }) + "触发一次";
      } catch (e) {
        return '无效的 Cron 表达式';
      }
    },
    getTypeLabel(type) {
      switch (type) {
        case 'transfer':
          return '迁移任务';
        case 'data_switch':
          return '数据切换任务';
        case 'clean':
          return '清理任务';
        default:
          return type || '-';
      }
    },
    getScheduleIntroduction(type) {
      switch (type) {
        case 'transfer':
          return '迁移任务是将集合的数据从源站点迁移到目标站点(数据源集群)上，支持增量迁移 LOB 数据；不支持增量迁移记录数据，每次变更记录数据，都将全量迁移记录数据！！！';
        case 'data_switch':
          return '数据切换任务是将已经迁移到目标站点(数据源集群)的集合，通过数据源关联的方式，将源集合的数据访问切换到数据源集群上；在这个过程中，源集合将被重命名，重命名的格式为 {clName}_data_switch_bak_{time}';
        case 'clean':
          return '清理任务是将已经完成数据切换，并且被数据切换任务重名名的源集合删除，重命名的命名格式为 {clName}_data_switch_bak_{time}';
        default:
          return type || '-';
      }
    },
    handleViewLogs(row) {
      // 跳转到运行记录页并携带任务 ID
      this.$router.push({
        name: 'RunRecord',
        query: { schedule_id: row.id }
      })
    },
    handleDeleteBtnClick(row) {
      this.$confirm(`您确认删除任务【${row.name}】吗`, '提示', {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        confirmButtonClass: 'btn_schedule_confirmDelete',
        type: 'warning'
      }).then(() => {
        deleteSchedule(row.id).then(res => {
          this.$message.success(`【${row.name}】删除成功`)
          this.fetchData()
        })

      }).catch(() => {
      });
    },
    handleShowDetail(row) {
      this.currentTask = JSON.parse(JSON.stringify(row));
      this.detailDialogVisible = true;
    },
    handleAddBtnClick() {
      this.currentTask = this.getEmptyTask();
      this.dialogTitle = "创建调度任务";
      this.submitText = "创建";
      this.dialogVisible = true;
    },
    handleEditBtnClick(task) {
      this.currentTask = JSON.parse(JSON.stringify(task));
      this.dialogTitle = "编辑调度任务";
      this.submitText = "保存";
      this.dialogVisible = true;
    },
    handleToggleEnable(task) {
      const newStatus = task.enable;
      switchSchedule(task.id, newStatus)
        .then(() => {
          this.$message.success(`任务 "${task.name}" 已${newStatus ? '启用' : '禁用'}`);
        })
        .catch(() => {
          // 接口失败时回滚状态
          task.enable = !newStatus;
          this.$message.error(`任务 "${task.name}" 状态更新失败`);
        });
    },
    handleSubmitTask(taskData) {
      if (this.submitText === "创建") {
        taskData.id = Date.now();
        taskData.createTime = Date.now();
        taskData.updateTime = Date.now();
        this.allTasks.push(taskData);
        this.$message.success(`任务 "${taskData.name}" 创建成功`);
      } else {
        taskData.updateTime = Date.now();
        this.$message.success(`任务 "${taskData.name}" 修改成功`);
      }
    },
    doSearch() {
      this.pagination.current = 1;
      this.fetchData();
    },
    resetSearch() {
      this.searchParams.name = '';
      this.pagination.current = 1;
      this.fetchData();
    },
    handleCurrentChange(page) {
      this.pagination.current = page;
      this.fetchData();
    },
    fetchData() {
      this.tableLoading = true;
      let filter = {};
      if (this.searchParams.name && this.searchParams.name.trim() !== '') {
        filter.name = this.searchParams.name.trim();
      }
      queryScheduleList(
        this.pagination.current,
        this.pagination.size,
        filter
      ).then(res => {
        // 如果接口返回数组
        this.tableData = res.data;
        this.pagination.total = Number(res.headers['x-record-count']); // 若后端返回总数，可以改成 res.total
        this.tableLoading = false;
      }).catch(() => {
        this.tableLoading = false;
      });
    },
    getEmptyTask() {
      return {
        name: "",
        desc: "",
        type: "",
        cron: "",
        enable: true,
        content: {}
      }
    },
    formatDate(timestamp) {
      if (!timestamp) return '-';
      var d = new Date(timestamp);
      return d.toLocaleString();
    }
  },
  mounted() {
    this.fetchData();
  }
};
</script>

<style  scoped>
.list-box {
  padding: 10px 5px;
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}
.list-tag {
  cursor: default;
  user-select: text;
}
.empty-text {
  color: #999;
  font-size: 13px;
  padding: 6px 0;
}

.search-box {
  width: 50%;
  float: right;
  margin-bottom: 10px;
}
.task-detail-dialog .el-dialog__body {
  padding: 15px 20px;
}

.info-card, .content-card {
  margin-bottom: 15px;
  padding: 15px 20px;
  border-radius: 8px;
}
.card-title {
  font-size: 16px;
  font-weight: 600;
  margin-bottom: 10px;
  color: #409EFF;
}
.content-card pre {
  background: #f5f5f5;
  padding: 10px;
  border-radius: 4px;
  overflow-x: auto;
  white-space: pre-wrap;
  word-break: break-word;
  margin: 0;
}
.el-row {
  margin-bottom: 6px;
}
.el-col {
  margin-bottom: 6px;
}

.el-col strong {
  color: #606266;
}
</style>
