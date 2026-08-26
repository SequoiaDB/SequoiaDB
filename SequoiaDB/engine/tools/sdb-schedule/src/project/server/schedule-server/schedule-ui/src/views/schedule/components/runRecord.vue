<template>
  <div class="app-container">
    <!-- 搜索部分 -->
    <div class="search-box">
      <el-row :gutter="2" type="flex" justify="end">
        <el-col :span="9">
          <el-input
            maxlength="50"
            size="small"
            placeholder="运行ID"
            v-model="searchParams.id"
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
        <el-col :span="3">
          <el-button
            @click="goBack"
            size="small"
            icon="el-icon-back"
            style="width:100%">
            返回
          </el-button>
        </el-col>
      </el-row>
    </div>

    <!-- 表格部分 -->
    <el-table
      v-loading="tableLoading"
      :data="tableData"
      border
      style="width: 100%; margin-top: 10px;">
      <el-table-column prop="id" label="运行ID" width="200" show-overflow-tooltip></el-table-column>
      <el-table-column prop="type" label="任务类型" width="120">
        <template slot-scope="scope">
          <span>
            {{ formatTaskType(scope.row.type) }}
          </span>
        </template>
      </el-table-column>
      <el-table-column prop="run_server" label="执行节点" width="220" show-overflow-tooltip></el-table-column>
      <el-table-column label="执行计划" width="150">
        <template slot-scope="scope">
          <el-tooltip
            effect="dark"
            placement="top"
            v-if="scope.row.plan"
          >
            <template #content>
              {{ scope.row.plan }}
            </template>
            <span class="ellipsis-text">{{ scope.row.plan }}</span>
          </el-tooltip>
          <span v-else>-</span>
        </template>
      </el-table-column>
      <el-table-column prop="process_cl_count" label="已处理的集合数" width="120" show-overflow-tooltip></el-table-column>
      <el-table-column prop="running_flag" label="状态" width="100">
        <template slot-scope="scope">
          <el-tag :type="getStatusTagType(scope.row.running_flag)">
            {{ getStatusText(scope.row.running_flag) }}
          </el-tag>
        </template>
      </el-table-column>
      <el-table-column label="开始时间" prop="start_time" width="150">
        <template slot-scope="scope">{{ formatDate(scope.row.start_time) }}</template>
      </el-table-column>
      <el-table-column label="结束时间" prop="stop_time" width="150">
        <template slot-scope="scope">{{ formatDate(scope.row.stop_time) }}</template>
      </el-table-column>
      <el-table-column prop="detail" label="详细信息" width="210" show-overflow-tooltip></el-table-column>

      <!-- 操作栏 -->
      <el-table-column label="操作" width="249" align="center">
        <template slot-scope="scope">
          <el-button
            type="primary"
            size="mini"
            @click="handleViewDetail(scope.row)">
            查看
          </el-button>
          <el-button
            type="success"
            size="mini"
            @click="handleRunDetail(scope.row)">
            执行情况
          </el-button>
          <el-button
            type="danger"
            size="mini"
            :disabled="scope.row.running_flag !== 2"
            @click="handleStopTask(scope.row)">
            停止
          </el-button>
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

    <!-- 运行详情弹窗 -->
    <el-dialog
      title="运行记录详情"
      :visible.sync="detailDialogVisible"
      width="800px"
      :close-on-click-modal="false"
      custom-class="record-detail-dialog"
    >
      <el-card shadow="hover" class="info-card">
        <h3 class="card-title">基本信息</h3>
        <el-row :gutter="20">
          <el-col :span="12"><strong>ID：</strong>{{ currentRecord.id }}</el-col>
          <el-col :span="12"><strong>调度ID：</strong>{{ currentRecord.schedule_id }}</el-col>
          <el-col :span="12"><strong>任务类型：</strong>{{ formatTaskType(currentRecord.type) }}</el-col>
          <el-col :span="12"><strong>运行服务器：</strong>{{ currentRecord.run_server }}</el-col>
          <el-col :span="12"><strong>状态：</strong>{{ currentRecord.running_flag === 1 ? '运行中' : '已停止' }}</el-col>
          <el-col :span="12"><strong>开始时间：</strong>{{ formatDate(currentRecord.start_time) }}</el-col>
          <el-col :span="12"><strong>结束时间：</strong>{{ formatDate(currentRecord.stop_time) }}</el-col>
          <el-col :span="12"><strong>创建时间：</strong>{{ formatDate(currentRecord.create_time) }}</el-col>
          <!-- 新增 detail 字段显示 -->
          <el-col :span="12">
            <strong>详细信息：</strong>
            <el-tooltip effect="dark" placement="top" :content="currentRecord.detail || '-'">
              <span class="ellipsis-text">{{ currentRecord.detail || '-' }}</span>
            </el-tooltip>
          </el-col>
        </el-row>
      </el-card>

      <el-card shadow="hover" class="content-card" v-if="currentRecord.type !== 3">
        <h3 class="card-title">运行内容</h3>
        <el-row :gutter="20">
          <el-col :span="12"><strong>源站点：</strong>{{ currentRecord.content ? currentRecord.content.source_site : '-' }}</el-col>
          <el-col :span="12"><strong>目标站点：</strong>{{ currentRecord.content ? currentRecord.content.target_site : '-' }}</el-col>
          <el-col :span="24" v-if="currentRecord.type === 1 || currentRecord.type === 2">
            <strong>{{ currentRecord.type === 1 ? '迁移条件' : '数据切换条件' }}：</strong>
            <div style="margin-left: 20px; display: flex; flex-direction: column; gap: 6px;">
              <div v-if="currentRecord.type === 1">
                <span>目标数据域：</span>
                {{ currentRecord.content && currentRecord.content.data_domain ? currentRecord.content.data_domain : '不指定' }}
              </div>
              <div>
                {{ currentRecord.type === 1 ? '集合超过' : '切换' }} {{ currentRecord.content && currentRecord.content.no_write_time_threshold || 0 }} {{ currentRecord.type === 1 ? '天没数据写入才迁移记录' : '天没数据写入且已完成迁移的集合' }}
              </div>
              <div>
                {{ currentRecord.type === 1 ? '迁移' : '切换' }}创建时间超过 {{ currentRecord.content && currentRecord.content.cl_create_time_threshold || 0 }} 天的集合
              </div>
              <div v-if="currentRecord.type === 1">
                删除目标站点多余的数据：{{ currentRecord.content && currentRecord.content.delete_more_lob_in_target ? '是' : '否' }}
              </div>
              <div>
                分区中断：{{ currentRecord.content && currentRecord.content.partition_interruption ? '是' : '否' }}
              </div>
            </div>
          </el-col>
          <el-col :span="12">
            <strong>最大执行时间：</strong>
            {{ currentRecord.content ? formatMaxExecTime(currentRecord.content.max_exec_time) : '-' }}
          </el-col>
          <el-col :span="24">
            <el-tabs type="card" style="margin-top: 10px;">
              <el-tab-pane label="集合空间列表">
                <div class="list-box">
                  <el-tag
                    v-for="(item, idx) in (currentRecord.content && currentRecord.content.cs_list || [])"
                    :key="idx"
                    size="small"
                    type="info"
                    class="list-tag"
                  >
                    {{ item }}
                  </el-tag>
                  <div v-if="!currentRecord.content || !currentRecord.content.cs_list || !currentRecord.content.cs_list.length" class="empty-text">
                    暂无数据
                  </div>
                </div>
              </el-tab-pane>

              <el-tab-pane label="集合空间模糊匹配">
                <div class="list-box">
                  <el-tag
                    v-for="(item, idx) in (currentRecord.content && currentRecord.content.cs_regex || [])"
                    :key="idx"
                    size="small"
                    type="warning"
                    class="list-tag"
                  >
                    {{ item }}
                  </el-tag>
                  <div v-if="!currentRecord.content || !currentRecord.content.cs_regex || !currentRecord.content.cs_regex.length" class="empty-text">
                    暂无数据
                  </div>
                </div>
              </el-tab-pane>

              <el-tab-pane label="集合列表">
                <div class="list-box">
                  <el-tag
                    v-for="(item, idx) in (currentRecord.content && currentRecord.content.cl_list || [])"
                    :key="idx"
                    size="small"
                    type="success"
                    class="list-tag"
                  >
                    {{ item }}
                  </el-tag>
                  <div v-if="!currentRecord.content || !currentRecord.content.cl_list || !currentRecord.content.cl_list.length" class="empty-text">
                    暂无数据
                  </div>
                </div>
              </el-tab-pane>

              <el-tab-pane label="集合模糊匹配">
                <div class="list-box">
                  <el-tag
                    v-for="(item, idx) in (currentRecord.content && currentRecord.content.cl_regex || [])"
                    :key="idx"
                    size="small"
                    type="danger"
                    class="list-tag"
                  >
                    {{ item }}
                  </el-tag>
                  <div v-if="!currentRecord.content || !currentRecord.content.cl_regex || !currentRecord.content.cl_regex.length" class="empty-text">
                    暂无数据
                  </div>
                </div>
              </el-tab-pane>
            </el-tabs>
          </el-col>
        </el-row>
      </el-card>

      <el-card shadow="hover" class="content-card" v-if="currentRecord.type === 3">
        <h3 class="card-title">任务内容</h3>

        <el-row :gutter="20">
          <el-col :span="12">
            <strong>最大执行时间：</strong>
            {{ currentRecord.content ? formatMaxExecTime(currentRecord.content.max_exec_time) : '-' }}
          </el-col>
          <el-col :span="24">
            <el-collapse>
              <el-collapse-item
                v-for="(range, idx) in currentRecord.content.clean_range"
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

      <el-card shadow="hover" class="plan-card">
        <h3 class="card-title">执行计划</h3>
        <el-collapse>
          <el-collapse-item title="点击查看计划内容" name="1">
            <pre>{{ currentRecord.plan ? JSON.stringify(currentRecord.plan, null, 2) : '-' }}</pre>
          </el-collapse-item>
        </el-collapse>
      </el-card>

      <span slot="footer" class="dialog-footer">
        <el-button @click="detailDialogVisible = false">关闭</el-button>
      </span>
    </el-dialog>


    <!-- 运行详情弹窗 -->
    <el-dialog
      title="任务执行情况"
      :visible.sync="runDetailDialogVisible"
      width="1200px"
      :close-on-click-modal="false"
    >
      <el-table :data="runDetailData" border style="width:100%" v-if="currentTaskType !== 3">
        <el-table-column prop="collection" label="集合"></el-table-column>
        <template v-if="currentTaskType === 2">
          <el-table-column label="可切换">
            <template slot-scope="scope">
              {{ scope.row.can_data_switch ? '是' : '否' }}
            </template>
          </el-table-column>
          <el-table-column label="已切换">
            <template slot-scope="scope">
              {{ scope.row.data_switched ? '是' : '否' }}
            </template>
          </el-table-column>
        </template>

        <!-- 其他类型时显示 -->
        <template v-else>
          <el-table-column prop="success_record_num" label="成功记录数"></el-table-column>
          <el-table-column prop="failed_record_num" label="失败记录数"></el-table-column>
          <el-table-column prop="transfer_record_size" label="迁移的记录大小(B)"></el-table-column>
          <el-table-column prop="success_lob_num" label="成功LOB数"></el-table-column>
          <el-table-column prop="failed_lob_num" label="失败LOB数"></el-table-column>
          <el-table-column prop="transfer_lob_size" label="迁移的LOB大小(B)"></el-table-column>
        </template>
      </el-table>

      <el-table :data="runDetailData" border style="width:100%" v-if="currentTaskType == 3">
        <el-table-column prop="clean_site" label="清理站点"></el-table-column>
        <el-table-column prop="source_cl" label="原集合名"></el-table-column>
        <el-table-column prop="rename_cl" label="清理的集合名"></el-table-column>
        <el-table-column label="清理成功">
          <template slot-scope="scope">
            {{ scope.row.success ? '是' : '否' }}
          </template>
        </el-table-column>
        <el-table-column prop="clean_time" label="清理时间">
          <template slot-scope="scope">
            {{ formatDate(scope.row.clean_time) }}
          </template>
        </el-table-column>
        <el-table-column prop="detail" label="清理详情"></el-table-column>
      </el-table>

      <span slot="footer" class="dialog-footer">
        <el-button @click="runDetailDialogVisible = false">关闭</el-button>
      </span>
    </el-dialog>
  </div>
</template>

<script>
import { queryTasks } from '@/api/schedule'
import { getTaskProgress, stopTask } from '@/api/task'

export default {
  name: 'RunRecord',
  data() {
    return {
      tableLoading: false,
      tableData: [],
      pagination: { current: 1, size: 10, total: 0 },
      searchParams: { id: '' },
      scheduleId: null,
      detailDialogVisible: false,
      currentRecord: {},
      runDetailDialogVisible: false,
      runDetailData: [],
      currentTaskType: null
    }
  },
  created() {
    this.scheduleId = this.$route.query.schedule_id
    if (!this.scheduleId) {
      this.$message.warning('未指定任务 ID')
      return
    }
    this.fetchData()
  },
  methods: {
    formatMaxExecTime(ms) {
      if (!ms && ms !== 0) return "-";
      if (ms < 60000) {
        return ms + " ms";
      }
      const minutes = Math.floor(ms / 60000);
      const seconds = Math.floor((ms % 60000) / 1000);
      return `${minutes} 分钟 ${seconds} 秒`;
    },
    getStatusText(flag) {
    switch (flag) {
      case 1:
        return '初始化'
      case 2:
        return '运行中'
      case 3:
        return '已完成'
      case 4:
        return '异常结束'
      case 5:
        return '超时结束'
      case 6:
        return '已停止'
      default:
        return '-'
    }
  },
  getStatusTagType(flag) {
    switch (flag) {
      case 1:
        return 'info'
      case 2:
        return 'success'
      case 3:
        return 'success'
      case 4:
        return 'danger'
      case 5:
        return 'warning'
      case 6:
        return 'warning'
      default:
        return ''
    }
  },
  handleStopTask(row){
    if (row.running_flag !== 2) {
      this.$message.warning('只有运行中的任务才能停止')
      return
    }
    // 2️⃣ 二次确认弹窗
  this.$confirm(
    `确认要停止任务吗？`,
    '停止确认',
    {
      confirmButtonText: '确定停止',
      cancelButtonText: '取消',
      type: 'warning'
    }
  )
    .then(() => {
      stopTask(row.id).then(() => {
          this.$message.success('任务停止成功')
          this.fetchData()
      })
    })
    .catch(() => {
    })
  },
    // 点击“运行详情”按钮
    handleRunDetail(row) {
      const taskId = row.id
      this.currentTaskType = row.type
      getTaskProgress(taskId)
        .then(res => {
          // 假设返回的数据是对象或数组，转换成数组用于表格渲染
          this.runDetailData = Array.isArray(res.data) ? res.data : [res.data]
          this.runDetailDialogVisible = true
        })
        .catch(err => {
          console.error(err)
          this.$message.error('获取运行详情失败')
        })
    },
    formatTaskType(type) {
      if (type === 1) return '迁移任务'
      if (type === 2) return '数据切换任务'
      if (type === 3) return '清理任务'
      return '-'
    },
    fetchData() {
      this.tableLoading = true
      queryTasks(
        this.scheduleId,
        this.searchParams.id ? { id: this.searchParams.id } : {},
        this.pagination.current,
        this.pagination.size,
        { create_time: -1 }
      )
        .then(res => {
          if (res && res.data) {
            this.tableData = res.data
          } else if (Array.isArray(res)) {
            this.tableData = res
          } else {
            this.tableData = []
          }
          this.pagination.total =
            (res.headers && Number(res.headers['x-record-count'])) ||
            res.total ||
            this.tableData.length
        })
        .catch(err => {
          console.error(err)
          this.$message.error('获取运行记录失败')
        })
        .finally(() => {
          this.tableLoading = false
        })
    },
    handleViewDetail(row) {
      this.currentRecord = JSON.parse(JSON.stringify(row))
      this.detailDialogVisible = true
    },
    doSearch() {
      this.pagination.current = 1
      this.fetchData()
    },
    resetSearch() {
      this.searchParams.id = ''
      this.pagination.current = 1
      this.fetchData()
    },
    handleCurrentChange(page) {
      this.pagination.current = page
      this.fetchData()
    },
    goBack() {
      this.$router.back()
    },
    joinArray(arr) {
      return arr && arr.length ? arr.join(', ') : '-'
    },
    formatDate(ts) {
      if (!ts) return '-'
      var d = new Date(ts)
      return d.toLocaleString()
    }
  }
}
</script>

<style scoped>
.plan-card {
  margin-bottom: 15px;
  padding: 15px 20px;
  border-radius: 8px;
}
.content-card pre {
  background: #f5f5f5;
  padding: 10px;
  border-radius: 4px;
  overflow-x: auto;    /* 横向滚动 */
  white-space: pre-wrap; /* 自动换行 */
  word-break: break-word;
  margin: 0;
}
.plan-card pre {
  background: #f5f5f5;
  padding: 10px;
  border-radius: 4px;
  overflow-x: auto; /* 横向溢出滚动 */
  white-space: pre-wrap; /* 自动换行 */
  word-break: break-word;
}
.ellipsis-text {
  display: inline-block;
  max-width: 200px;   /* 注意要小于 column 的宽度 */
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  vertical-align: middle;
}
.search-box {
  width: 50%;
  float: right;
  margin-bottom: 10px;
}
.pagination {
  text-align: right;
  margin-top: 15px;
}
.info-card,
.content-card {
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
