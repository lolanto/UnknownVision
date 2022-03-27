from math import ceil, floor
from turtle import down
import urllib.request
import os
import shutil
from xml.etree.ElementInclude import include
import zipfile

VS_DEVELOPER_COMMAND_PROMPT_PATH = "xxx\Common7\Tools\VsDevCmd.bat"

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


def DownloadFileCallback(downloadedBlocks, blockSizeInByte, totalSizeOfFile):
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
    download_file = os.path.join(TEMP_PATH, 'dxc.zip')
    DXCompilerURL = "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.6.2112/dxc_2021_12_08.zip"
    urllib.request.urlretrieve(DXCompilerURL, download_file, DownloadFileCallback)
    dxc_zip_file = zipfile.ZipFile(download_file, 'r')
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

    DXTexURL = "https://github.com/microsoft/DirectXTex/archive/refs/tags/mar2022.zip"
    download_file = os.path.join(TEMP_PATH, 'DirectXTex.zip')
    urllib.request.urlretrieve(DXTexURL, download_file, DownloadFileCallback)
    dxTex_zip_file = zipfile.ZipFile(download_file, 'r')
    DX_TEX_PATH = os.path.join(TEMP_PATH, 'DXTEX')
    dxTex_zip_file.extractall(DX_TEX_PATH)
    cmd = VS_DEVELOPER_COMMAND_PROMPT_PATH + ''' & \
        cd temp\\DXTEX\\DirectXTex-mar2022 & \
        msbuild DirectXTex\\DirectXTex_Desktop_2019.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 & \
    '''
    ret = os.system(cmd)
    if ret != 0:
        raise Exception("Build DirectXTex Failed!")

    DX_TEX_PATH = os.path.join(DX_TEX_PATH, 'DirectXTex-mar2022', 'DirectXTex')    
    MKDIR(os.path.join(INCLUDE_PATH, 'DirectXTex'))
    for x in include_file_list:
        shutil.move(os.path.join(DX_TEX_PATH, x), os.path.join(INCLUDE_PATH, 'DirectXTex', x))

    DX_TEX_PATH = os.path.join(DX_TEX_PATH, 'Bin', 'Desktop_2019', 'x64', 'Debug')
    MKDIR(os.path.join(LIB_PATH, PLATFORM, CONFIGURATION))
    for x in lib_file_list:
        shutil.move(os.path.join(DX_TEX_PATH, x), os.path.join(LIB_PATH, PLATFORM, CONFIGURATION, x))
    
    shutil.rmtree(TEMP_PATH, 'DXTEX')
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
    GLFW_TEMP_FILE = os.path.join(TEMP_PATH, 'GLFW')
    download_file = os.path.join(TEMP_PATH, 'GLFW.zip')
    GLFWURL = "https://github.com/glfw/glfw/releases/download/3.3.6/glfw-3.3.6.bin.WIN64.zip"
    urllib.request.urlretrieve(GLFWURL, download_file, DownloadFileCallback)
    glfw_zip_file = zipfile.ZipFile(download_file, 'r')
    glfw_zip_file.extractall(GLFW_TEMP_FILE)

    GLFW_TEMP_FILE = os.path.join(GLFW_TEMP_FILE, 'glfw-3.3.6.bin.WIN64')
    MKDIR(os.path.join(INCLUDE_PATH, 'GLFW'), True)
    for x in include_file_list:
        shutil.move(os.path.join(GLFW_TEMP_FILE, 'include', 'GLFW', x), os.path.join(INCLUDE_PATH, 'GLFW', x))
    MKDIR(os.path.join(LIB_PATH, PLATFORM, CONFIGURATION))
    for x in lib_file_list:
        shutil.move(os.path.join(GLFW_TEMP_FILE, 'lib-vc2019', x), os.path.join(LIB_PATH, PLATFORM, CONFIGURATION, x))

    shutil.rmtree(TEMP_PATH, 'GLFW')
    print("Config GLFW Finished!")


if __name__ == '__main__':
    # 初始化文件夹
    MKDIR(TEMP_PATH)
    MKDIR(BIN_PATH)
    MKDIR(LIB_PATH)
    MKDIR(INCLUDE_PATH)
    Config_DXC()
    Config_DXTex()
    Config_GLFW()