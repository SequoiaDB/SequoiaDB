MarkdownEditor.exe: Markdown 编辑器
testLink.bat: 1. 检查在 toc.json 配置的目录或文件是否实际存在
              2. 检查 md 文档链接（图片链接和跳转链接）是否有错，无法检测大小写错误，链接请一定要注意大小写
              3. 直接双击使用，执行完成后会在当前路径生成 output.log
checktoc.bat: 1. 检查没有在 toc.json 配置的 md 文档文件
              2. 直接双击使用


编辑文档
双击运行 MarkdownEditor.exe 即可

图片链接：
images/Architecture/Data_Model/intro_data_mode_architecture_diagram.png

跳转链接：
manual/Architecture/Data_Model/index.md

跳转到页面指定标题的链接：
manual/Architecture/Data_Model/index.md#xxx