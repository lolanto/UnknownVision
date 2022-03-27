## 配置方法
1. 依赖项

    * python 3.10
    * visual studio 2019(16.11.5)

2. 修改脚本setup.py中的**VS_DEVELOPER_COMMAND_PROMPT_PATH**全局变量，使其目录指向“xxx\Common7\Tools\VsDevCmd.bat”

3. 运行setup.py，若中间下载出现异常(无法链接github)，可重复运行。

## 子项目配置方法

### ClientWithUI
* 需要添加Examples/ClientWithUI/UV_Grid_Sm.jpg
* 图片内容无所谓，仅需要保证名称一致