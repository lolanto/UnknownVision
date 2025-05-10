import urllib.request
import os
import shutil
import zipfile
import subprocess

VS_DEVELOPER_COMMAND_PROMPT_PATH = None

GLFW_URL = "https://github.com/glfw/glfw/releases/download/3.3.6/glfw-3.3.6.bin.WIN64.zip"
DX_COMPILER_URL = "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.6.2112/dxc_2021_12_08.zip"
DX_TEX_URL = "https://github.com/microsoft/DirectXTex/archive/refs/tags/mar2022.zip"

TEMP_PATH = ".\\temp"
OUTPUT_PATH = ".\\3rdPart"
BIN_PATH = os.path.join(OUTPUT_PATH, 'bin')
LIB_PATH = os.path.join(OUTPUT_PATH, 'lib')
INCLUDE_PATH = os.path.join(OUTPUT_PATH, 'include')
PLATFORM = 'x64'
CONFIGURATION = 'Debug'

def MKDIR(path, remove_if_it_is_existed = False):
    folder = os.path.exists(path)
    if folder:
        if remove_if_it_is_existed:
            shutil.rmtree(path)
        else:
            print("path: ", path, " existed!")
            return
        
    os.makedirs(path)
    print("create path: ", path)


def CHECK_FILE(filePath):
    if os.path.exists(filePath):
        return True
    else:
        return False


def Download_File_Callback(downloadedBlocks, blockSizeInByte, totalSizeOfFile):
    if totalSizeOfFile <= 0:
        unit = 'B'
        downloadedFileSize = downloadedBlocks * blockSizeInByte
        if downloadedFileSize > 1024:
            unit = 'KB'
            downloadedFileSize /= 1024
        if downloadedFileSize > 1024:
            unit = 'MB'
            downloadedFileSize /= 1024
        print('\rDownloaded File Size: {0:.2f} {1}'.format(downloadedFileSize, unit), end='', flush=True)
    else:
        totalBlocks = totalSizeOfFile / blockSizeInByte
        print('\r{0:.2f}%'.format(downloadedBlocks / totalBlocks * 100), end='', flush=True)


'''
根据提供的URL，尝试下载对应的三方工具压缩包
target_url: 目标二进制下载地址
file_name: 下载之后的文件名字
'''
def Try_To_Get_Third_Party_Bin(target_url, file_name):
    # 检查本地是不是已经有对应的二进制内容
    download_file_path = os.path.join(TEMP_PATH, file_name)
    if os.path.exists(download_file_path):
        print(f'检查到路径: {download_file_path}已经存在，直接使用')
        return True
    print(f'尝试从: {target_url} 下载文件')
    try:
        urllib.request.urlretrieve(DXCompilerURL, download_file_path, Download_File_Callback)
    except:
        print(f'下载{file_name}失败!')
        print(f'你可以尝试手动下载: {target_url}')
        print(f'并将下载内容重命名后保存到：{download_file_path}')
        return False
    print(f'下载{file_name} 成功!')


def Config_DXC():
    # 检查DXC是否配置过
    should_config_dxc = False
    bin_file_list = ['dxc.exe', 'dxcompiler.dll', 'dxil.dll']
    for x in bin_file_list:
        if CHECK_FILE(os.path.join(BIN_PATH, PLATFORM, CONFIGURATION, x)) is False:
            should_config_dxc = True
            break
    lib_file_list = ['dxcompiler.lib']
    for x in lib_file_list:
        if CHECK_FILE(os.path.join(LIB_PATH, PLATFORM, CONFIGURATION, x)) is False:
            should_config_dxc = True
            break
    include_file_list = ['d3d12shader.h', 'dxcapi.h']
    for x in include_file_list:
        if CHECK_FILE(os.path.join(INCLUDE_PATH, 'DXC', x)) is False:
            should_config_dxc = True
            break
    
    if should_config_dxc is False:
        print("DXC is Configured")
        return

    print("Start Config DXC")
    DXC_TEMP_PATH = os.path.join(TEMP_PATH, 'DXC')
    DXC_FILE_NAME = 'dxc.zip'
    if Try_To_Get_Third_Party_Bin(DX_COMPILER_URL, DXC_FILE_NAME) is False:
        raise Exception("运行失败!")
    download_file_path = os.path.join(TEMP_PATH, 'dxc.zip')
    dxc_zip_file = zipfile.ZipFile(download_file_path, 'r')
    dxc_zip_file.extractall(DXC_TEMP_PATH)
    MKDIR(os.path.join(BIN_PATH, PLATFORM, CONFIGURATION))
    for x in bin_file_list:
        shutil.move(os.path.join(DXC_TEMP_PATH, 'bin', 'x64', x), os.path.join(BIN_PATH, PLATFORM, CONFIGURATION, x))
    MKDIR(os.path.join(LIB_PATH, PLATFORM, CONFIGURATION))
    for x in lib_file_list:
        shutil.move(os.path.join(DXC_TEMP_PATH, 'lib', 'x64', x), os.path.join(LIB_PATH, PLATFORM, CONFIGURATION, x))
    MKDIR(os.path.join(INCLUDE_PATH, 'DXC'), True)
    for x in include_file_list:
        shutil.move(os.path.join(DXC_TEMP_PATH, 'inc', x), os.path.join(INCLUDE_PATH, 'DXC', x))

    shutil.rmtree(DXC_TEMP_PATH)
    print("Config DXC Finished!")

def Config_DXTex():
    # 检查DXTex是否配置过
    should_config_DXTex = False
    lib_file_list = ['DirectXTex.lib', 'DirectXTex.pch', 'DirectXTex.pdb']
    for x in lib_file_list:
        if CHECK_FILE(os.path.join(LIB_PATH, PLATFORM, CONFIGURATION, x)) is False:
            should_config_DXTex = True
            break
    include_file_list = ['DirectXTex.h', 'DirectXTex.inl']
    for x in include_file_list:
        if CHECK_FILE(os.path.join(INCLUDE_PATH, 'DirectXTex', x)) is False:
            should_config_DXTex = True
            break

    if should_config_DXTex is False:
        print("DXTex is Configured")
        return

    DX_TEX_FILE_NAME = 'DirectXTex.zip'
    if Try_To_Get_Third_Party_Bin(DX_TEX_URL, DX_TEX_FILE_NAME) is False:
        raise Exception("运行失败!")
    download_file_path = os.path.join(TEMP_PATH, DX_TEX_FILE_NAME)
    dxTex_zip_file = zipfile.ZipFile(download_file_path, 'r')
    DX_TEX_PATH = os.path.join(TEMP_PATH, 'DXTEX')
    dxTex_zip_file.extractall(DX_TEX_PATH)
    cmd = VS_DEVELOPER_COMMAND_PROMPT_PATH + ''' & \
        cd temp\\DXTEX\\DirectXTex-mar2022 & \
        msbuild DirectXTex\\DirectXTex_Desktop_2022.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 & \
    '''
    ret = os.system(cmd)
    if ret != 0:
        raise Exception("Build DirectXTex Failed!")

    DX_TEX_PATH = os.path.join(DX_TEX_PATH, 'DirectXTex-mar2022', 'DirectXTex')    
    MKDIR(os.path.join(INCLUDE_PATH, 'DirectXTex'))
    for x in include_file_list:
        shutil.move(os.path.join(DX_TEX_PATH, x), os.path.join(INCLUDE_PATH, 'DirectXTex', x))

    DX_TEX_PATH = os.path.join(DX_TEX_PATH, 'Bin', 'Desktop_2022', 'x64', 'Debug')
    MKDIR(os.path.join(LIB_PATH, PLATFORM, CONFIGURATION))
    for x in lib_file_list:
        shutil.move(os.path.join(DX_TEX_PATH, x), os.path.join(LIB_PATH, PLATFORM, CONFIGURATION, x))
    
    shutil.rmtree(DX_TEX_PATH)
    print("Config DXTex Finished!")

def Config_GLFW():
    # 检查glfw是否配置
    should_config_glfw = False
    lib_file_list = ['glfw3.lib']
    for x in lib_file_list:
        if CHECK_FILE(os.path.join(LIB_PATH, PLATFORM, CONFIGURATION, x)) is False:
            should_config_glfw = True
            break

    include_file_list = ['glfw3.h', 'glfw3native.h']
    for x in include_file_list:
        if CHECK_FILE(os.path.join(INCLUDE_PATH, PLATFORM, CONFIGURATION, x)) is False:
            should_config_glfw = True
            break
    
    if should_config_glfw is False:
        print("GLFW is configured!")
        return
    
    print("Start Config GLFW")
    GLFW_TEMP_PATH = os.path.join(TEMP_PATH, 'GLFW')
    GLFW_FILE_NAME = 'GLFW.zip'
    
    if Try_To_Get_Third_Party_Bin(GLFW_URL, GLFW_FILE_NAME) is False:
        raise Exception("运行失败!")
    
    download_file_path = os.path.join(TEMP_PATH, 'GLFW.zip')
    glfw_zip_file = zipfile.ZipFile(download_file_path, 'r')
    glfw_zip_file.extractall(GLFW_TEMP_PATH)

    GLFW_TEMP_FILE = os.path.join(GLFW_TEMP_PATH, 'glfw-3.3.6.bin.WIN64')
    MKDIR(os.path.join(INCLUDE_PATH, 'GLFW'), True)
    for x in include_file_list:
        shutil.move(os.path.join(GLFW_TEMP_FILE, 'include', 'GLFW', x), os.path.join(INCLUDE_PATH, 'GLFW', x))
    MKDIR(os.path.join(LIB_PATH, PLATFORM, CONFIGURATION))
    for x in lib_file_list:
        shutil.move(os.path.join(GLFW_TEMP_FILE, 'lib-vc2019', x), os.path.join(LIB_PATH, PLATFORM, CONFIGURATION, x))

    shutil.rmtree(GLFW_TEMP_PATH)
    print("Config GLFW Finished!")


'''
配置VS开发者命令行工具路径，之后一些驱动VS对三方库的构建需要用到
'''
def Config_VS_Cmd_Env():
    global VS_DEVELOPER_COMMAND_PROMPT_PATH
    cmd = f'{os.environ.get("ProgramFiles(x86)")}\\Microsoft Visual Studio\\Installer\\vswhere.exe'
    result = subprocess.run(
        [cmd, '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
        capture_output=True,  # 捕获 stdout 和 stderr
        text=True              # 输出是字符串而不是字节
    )
    if result.returncode != 0:
        raise Exception(f'配置VS开发者命令行工具失败!')

    vs_dev_toop_path = result.stdout.strip()
    if os.path.exists(vs_dev_toop_path) is False:
        raise Exception(f'获取的VS安装路径错误{vs_dev_toop_path}')
    
    VS_DEVELOPER_COMMAND_PROMPT_PATH = os.path.join(vs_dev_toop_path, 'Common7', 'Tools', 'VsDevCmd.bat')
    if os.path.exists(VS_DEVELOPER_COMMAND_PROMPT_PATH) is False:
        raise Exception(f'VS开发者命令行工具{VS_DEVELOPER_COMMAND_PROMPT_PATH}不存在！')

if __name__ == '__main__':
    # 初始化文件夹
    MKDIR(TEMP_PATH)
    MKDIR(BIN_PATH)
    MKDIR(LIB_PATH)
    MKDIR(INCLUDE_PATH)
    Config_VS_Cmd_Env()
    Config_DXC()
    Config_DXTex()
    Config_GLFW()