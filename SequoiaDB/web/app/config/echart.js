(function () {
   window.SdbSacManagerConf.recordEchart = {
      tooltip: {
         trigger: 'axis',
         formatter: '{a0}: {c0}'
      },
      animation: false,
      addDataAnimation: false,
      legend: {
         borderWidth: 1,
         padding: [5, 10, 5, 10],
         borderColor: '#727272',
         data: ['Record insert'],
         textStyle: {
            color: 'auto'
         },
         y: 'bottom'
      },
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30']
         }
      ],
      yAxis: [
         {
            type: 'value'
         }
      ],
      grid: {
         x: 55,
         y: 20,
         x2: 20,
         y2: 60
      },
      series: [
         {
            name: 'Record insert',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.DiskStorageEchart = {
      title: {
         show: true,
         text: '磁盘容量',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}%'
      },
      legend: {
         data: ['Used']
      },
      animation: false,
      addDataAnimation: false,
      color: ['#2EC7C9', '#DB7093'],
      xAxis: [
         {
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            max: 100,
            axisLabel: {
               formatter: '{value}%'
            }
         }
      ],
      grid: {
         x: 55,
         x2: 20,
         y: 40,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            itemStyle: {
               normal: {
                  color: '#9ECCB7',
                  lineStyle: { color: '#9ECCB7' },
                  areaStyle: { type: 'default' }
               }
            },
            smooth: true,
            name: 'Used',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.DiskIOEchart = {
      title: {
         show: true,
         text: '读取/写入',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}MB<br>{a1}: {c1}MB'
      },
      legend: {
         data: ['Read', 'Write']
      },
      animation: false,
      addDataAnimation: false,
      color: ['#6495ED', '#FF69B4'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            axisLabel: {
               formatter: '{value}MB'
            }
         }
      ],
      grid: {
            x: 55,
            x2: 20,
            y: 40,
            y2: 40,
            borderColor: '#000'
         },
      series: [
         {
            smooth: true,
            name: 'Read',
            stack: 'Read',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         },
         {
            smooth: true,
            name: 'Write',
            stack: 'Write',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.CpuEchart = {
      title: {
         show: true,
         text: 'CPU利用率',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}%'
      },
      grid: {
         x: 55,
         x2: 20,
         y: 40,
         y2: 40,
         borderColor: '#000'
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      legend: {
         data: [
            'Used'
         ]
      },
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            },
            data: ['0', '5', '10', '15', '20', '25', '30']
         }
      ],
      yAxis: [
         {
            type: 'value',
            max: 100,
            data: ['0', '20', '40', '60', '80', '100'],
            axisLabel: {
               formatter: '{value}%'
            }
         }
      ],
      series: [
         {
            type: 'line',
            smooth: true,
            name: 'Used',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   } ;

   window.SdbSacManagerConf.MemoryLessEchart = {
      addDataAnimation: false,
      tooltip: {
         show: false,
         trigger: 'axis',
         axisPointer: {
            type: 'shadow'
         }
      },
      toolbox: {
         show: false
      },
      grid: {
         x: 10,
         y: 10,
         y2: 0,
         x2: 0,
         borderColor: '#fff'
      },
      xAxis: [
         {
            show: false,
            type: 'value'
         }
      ],
      yAxis: [
         {
            show: false,
            type: 'category',
            data: ['Memory']
         }
      ],
      series: [
         {
            name: 'Used',
            type: 'bar',
            stack: 'All',
            itemStyle: {
               normal: {
                  label: {
                     show: true,
                     position: 'insideRight',
                     formatter: '{c0}%',
                     textStyle: {
                        color: '#FFF',
                        fontSize: 16
                     }
                  },
                  color: '#18bc9a'
               }
            },
            data: [0, 0]
         },
         {
            name: 'Free',
            type: 'bar',
            stack: 'All',
            itemStyle: {
               normal: {
                  label: {
                     show: true,
                     position: 'insideRight',
                     formatter: '{c0}%',
                     textStyle: {
                        color: '#FFF',
                        fontSize: 16
                     }
                  },
                  color: '#21bbee'
               }
            },
            data: [0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.MemoryEchart = {
      title: {
         show: true,
         text: '内存占用',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}%'
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      legend: {
         data: [
            'Used'
         ]
      },
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            max: 100,
            axisLabel: {
               formatter: '{value}%'
            }
         }
      ],
      grid: {
         x: 55,
         y: 40,
         x2: 20,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            itemStyle: {
               normal: {
                  color: '#9ECCB7',
                  lineStyle: { color: '#9ECCB7' },
                  areaStyle: { type: 'default' }
               }
            },
            smooth: true,
            name: 'Used',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.RamBarEchart = {
      tooltip: {
         show: false,
         trigger: 'axis',
         axisPointer: {            // 坐标轴指示器，坐标轴触发有效
            type: 'shadow'        // 默认为直线，可选为：'line' | 'shadow'
         },
         formatter: '{c0}Mb'
      },
      toolbox: {
         show: false
      },
      grid: {
         x: 0,
         y: 0,
         y2: 0,
         x2: 0,
         borderColor: '#fff'
      },
      xAxis: [
         {
            show: false,
            type: 'value'
         }
      ],
      yAxis: [
         {
            show: false,
            type: 'category',
            data: ['1']
         }
      ],
      series: [
      {

      },
      {
         name: '已使用',
         type: 'bar',
         stack: '总量',
         itemStyle: {
            normal: { label: { show: true, position: 'insideRight', formatter: '{c0}MB' }, color: '#21BBEE' }
         },
         data: [2516]
      },
         {
            name: '空闲',
            type: 'bar',
            stack: '总量',
            itemStyle: {
               normal: { color: '#DDD' }
            },
            data: [4096]
         }
      ]
   };

   window.SdbSacManagerConf.NetworkEchart = {
      title: {
         show: true,
         text: '网络',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}KB'
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            axisLabel: {
               formatter: '{value}KB'
            }
         }
      ],
      grid: {
         x: 55,
         y: 40,
         x2: 20,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Write',
            stack: 'Write',
            itemStyle: { normal: { areaStyle: { type: 'default' } } },
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   } ;
   
   window.SdbSacManagerConf.NetworkOutEchart = {
      title: {
         show: true,
         text: 'TXByts',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}KB'
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            axisLabel: {
               formatter: '{value}KB'
            }
         }
      ],
      grid: {
         x: 55,
         y: 40,
         x2: 20,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'TXByts',
            stack: 'TXByts',
            itemStyle: { normal: { areaStyle: { type: 'default' } } },
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   } ;

   window.SdbSacManagerConf.NetworkInEchart = {
      title: {
         show: true,
         text: 'RXBytes',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}KB'
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            axisLabel: {
               formatter: '{value}KB'
            }
         }
      ],
      grid: {
         x: 55,
         y: 40,
         x2: 20,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'RXBytes',
            stack: 'RXBytes',
            itemStyle: { normal: { areaStyle: { type: 'default' } } },
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   } ;
   
   window.SdbSacManagerConf.MonitorDataEchart = {
      title: {
         show: true,
         text: '',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         //显示百分比和占用大小
         formatter: '{a0}: {c0}/s<br>{a1}: {c1}/s<br>{a2}: {c2}/s<br>{a3}: {c3}/s'
      },
      legend: {
         data: ['Insert','Read','Delete','Update']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            axisLabel: {
               formatter: '{value}/s'
            }
         }
      ],
      grid: {
         y: 40,
         x2: 20,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Insert',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         },
         {
            smooth: true,
            name: 'Read',
            itemStyle: {
               normal: {
                  color: '#D9534F',
                  lineStyle: { color: '#D9534F' }
               }
            },
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         },
         {
            smooth: true,
            name: 'Delete',
            itemStyle: {
               normal: {
                  color: '#FFB86C',
                  lineStyle: { color: '#FFB86C' }
               }
            },
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         },
         {
            smooth: true,
            name: 'Update',
            itemStyle: {
               normal: {
                  color: '#225A22',
                  lineStyle: { color: '#225A22' }
               }
            },
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   } ;

   window.SdbSacManagerConf.NetworkIOEchart = {
      title: {
         show: true,
         text: '网络流量',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         //显示百分比和占用大小
         formatter: '{a0}: {c0}KB<br>{a1}: {c1}KB'
      },
      legend: {
         data: ['In', 'Out']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            axisLabel: {
               formatter: '{value}KB'
            }
         }
      ],
      grid: {
         x: 55,
         y: 40,
         x2: 20,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'In',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         },
         {
            smooth: true,
            name: 'Out',
            itemStyle: {
               normal: {
                  color: '#9ECCB7',
                  lineStyle: { color: '#9ECCB7' }
               }
            },
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.RecordInsertEchart = {
      title: {
         show: true,
         text: 'Insert',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}/s'
      },
      legend: {
         data: ['Record Insert']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            axisLabel: {
               formatter: '{value}/s'
            }
         }
      ],
      grid: {
         x: 70,
         y: 40,
         x2: 10,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Record Insert',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.RecordUpdateEchart = {
      title: {
         show: true,
         text: 'Update',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}/s'
      },
      legend: {
         data: ['Record Update']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            axisLabel: {
               formatter: '{value}/s'
            }
         }
      ],
      grid: {
         x: 70,
         y: 40,
         x2: 10,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Record Update',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.RecordDeleteEchart = {
      title: {
         show: true,
         text: 'Delete',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}/s'
      },
      legend: {
         data: ['Record Delete']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
      {
         type: 'value',
         axisLabel: {
            formatter: '{value}/s'
         }
      }
      ],
      grid: {
         x: 70,
         y: 40,
         x2: 10,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Record Delete',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.RecordReadEchart = {
      title: {
         show: true,
         text: 'Read',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}/s'
      },
      legend: {
         data: ['Record Read']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
      {
         type: 'category',
         boundaryGap: false,
         data: ['0', '5', '10', '15', '20', '25', '30'],
         splitLine: {
            lineStyle: {
               color: ['#fff']
            }
         }
      }
      ],
      yAxis: [
      {
         type: 'value',
         axisLabel: {
            formatter: '{value}/s'
         }
      }
      ],
      grid: {
         x: 70,
         y: 40,
         x2: 10,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Record Read',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.ModuleEchart = {
      title: {
         show: false,
         text: '',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis'
      },
      legend: {
         show: false,
         data: ['Record Read']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
         {
            type: 'value',
            axisLabel: {
               formatter: '{value}/s'
            }
         }
      ],
      grid: {
         x: 60,
         y: 30,
         x2: 15,
         y2: 30,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Record Read',
            type: 'line',
            label: {
               normal: {
                  show: true,
                  position: 'top'
               }
            },
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   };

   window.SdbSacManagerConf.StorageScaleEchart = {
      title: {
         show: true,
         text: '元数据比例',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}%<br>{a1}: {c1}%<br>{a2}: {c2}%'
      },
      animation: false,
      addDataAnimation: false,
      legend: {
         x: 'right',
         data: ['Index', 'Data', 'Lob']
      },
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30']
         }
      ],
      yAxis: [
         {
            type: 'value',
            max: 100,
            axisLabel: {
               formatter: '{value}%'
            }
         }
      ],
      grid: {
         x: 55,
         y: 40,
         x2: 20,
         y2: 20,
         borderColor: '#000'
      },
      series: [
         {
            itemStyle: { normal: { areaStyle: { type: 'default' } } },
            smooth: true,
            name: 'Index',
            type: 'line',
            stack: 'Sum',
            data: [0, 0, 0, 0, 0, 0, 0]
         },
         {
            itemStyle: { normal: { areaStyle: { type: 'default' } } },
            smooth: true,
            name: 'Data',
            type: 'line',
            stack: 'Sum',
            data: [0, 0, 0, 0, 0, 0, 0]
         },
         {
            itemStyle: { normal: { areaStyle: { type: 'default' } } },
            smooth: true,
            name: 'Lob',
            type: 'line',
            stack: 'Sum',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ],
      color: ['#9299fb',  '#02CCAA', '#87cefa' ]
   };

   window.SdbSacManagerConf.RedundancyChart = {
      'title': {
         'text': '容量信息',
         'x': 160
      },
      'tooltip': {
         'trigger': 'item',
         'formatter': '{a} <br/>{b} : {c}MB ( {d}% )',
         'textStyle': {
            'fontSize': 10
         }
      },
      'legend': {
         'orient': 'vertical',
         'x': 'left',
         'data': ['可用容量', '冗余容量']
      },
      'calculable': true,
      'series': [
         {
            'name': '总容量',
            'type': 'pie',
            'radius': '100px',
            'center': ['250px', '160px'],
            'data': [
               { 'value': 0, 'name': '可用容量' },
               { 'value': 0, 'name': '冗余容量' }
            ]
         }
      ]
   } ;

   window.SdbSacManagerConf.ExtendRedundancyChart = {
      'title': {
         'text': '容量信息',
         'x': 160
      },
      'tooltip': {
         'trigger': 'item',
         'formatter': '{a} <br/>{b} : {c}MB ( {d}% )',
         'textStyle': {
            'fontSize': 10
         }
      },
      'legend': {
         'orient': 'vertical',
         'x': 'left',
         'data': ['可用容量', '冗余容量']
      },
      'calculable': true,
      'series': [
         {
            'name': '总容量',
            'type': 'pie',
            'radius': '75%',
            'center': ['50%', '50%'],
            'data': [
               { 'value': 0, 'name': '可用容量' },
               { 'value': 0, 'name': '冗余容量' }
            ]
         }
      ]
   } ;
   
   window.SdbSacManagerConf.TotalSessionsEchart = {
      title: {
         show: true,
         text: ' Total Sessions',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}'
      },
      legend: {
         data: ['Total Sessions']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
      {
         type: 'value',
         axisLabel: {
            formatter: '{value}'
         }
      }
      ],
      grid: {
         x: 60,
         y: 40,
         x2: 30,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Total Sessions',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   } ;
   window.SdbSacManagerConf.TotalContextsEchart = {
      title: {
         show: true,
         text: 'Total Contexts',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}'
      },
      legend: {
         data: ['Total Contexts']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
      {
         type: 'value',
         axisLabel: {
            formatter: '{value}'
         }
      }
      ],
      grid: {
         x: 60,
         y: 40,
         x2: 30,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Total Contexts',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   } ;
   window.SdbSacManagerConf.TotalProceduresEchart = {
      title: {
         show: true,
         text: ' Total Procedures',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}'
      },
      legend: {
         data: ['Total Procedures']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
      {
         type: 'value',
         axisLabel: {
            formatter: '{value}'
         }
      }
      ],
      grid: {
         x: 60,
         y: 40,
         x2: 30,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Total Procedures',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   } ;
   window.SdbSacManagerConf.TotalTransactionsEchart = {
      title: {
         show: true,
         text: ' Total Transactions',
         textStyle: {
            color: '#666',
            fontFamily: 'Verdana,Georgia,宋体',
            fontSize: 14
         }
      },
      tooltip: {
         enterable: true,
         trigger: 'axis',
         formatter: '{a0}: {c0}'
      },
      legend: {
         data: ['Total Transactions']
      },
      animation: false,
      addDataAnimation: false,
      color: ['rgba(39,169,227,1)'],
      xAxis: [
         {
            type: 'category',
            boundaryGap: false,
            data: ['0', '5', '10', '15', '20', '25', '30'],
            splitLine: {
               lineStyle: {
                  color: ['#fff']
               }
            }
         }
      ],
      yAxis: [
      {
         type: 'value',
         axisLabel: {
            formatter: '{value}'
         }
      }
      ],
      grid: {
         x: 60,
         y: 40,
         x2: 30,
         y2: 40,
         borderColor: '#000'
      },
      series: [
         {
            smooth: true,
            name: 'Total Transactions',
            type: 'line',
            data: [0, 0, 0, 0, 0, 0, 0]
         }
      ]
   } ;
}());