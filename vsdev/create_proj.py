import os
import sys
import argparse
import uuid

# 全局变量，用于存储项目名称和模板文件内容
PROJECT_NAME = None
TEMPLATE_VCXPROJ_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'template_vcxproj')
TEMPLATE_VCXPROJ_SRC = None

TEMPLATE_VCXPROJ_FILTER_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'template_vcxproj_filters')


def load_template_vcxproj():
    """
    加载模板文件内容到全局变量 TEMPLATE_VCXPROJ_SRC 中。
    如果模板文件存在，则读取其内容；否则打印错误信息。
    """
    global TEMPLATE_VCXPROJ_SRC
    if os.path.exists(TEMPLATE_VCXPROJ_PATH):
        with open(TEMPLATE_VCXPROJ_PATH, 'r', encoding='utf-8') as file:
            TEMPLATE_VCXPROJ_SRC = file.read()
            print("Template loaded successfully.")
            return True
    else:
        print(f"Template file does not exist: {TEMPLATE_VCXPROJ_PATH}")
    
    return False

def modify_vcproj():
    """
    修改模板内容：
    1. 替换模板中的 {#PROJECT_NAME#} 占位符为 PROJECT_NAME。
    2. 生成一个 GUID 并替换模板中的 {#GUID#} 占位符。
    """
    global TEMPLATE_VCXPROJ_SRC, PROJECT_NAME

    if TEMPLATE_VCXPROJ_SRC is None:
        print("Error: TEMPLATE_VCXPROJ_SRC is not loaded.")
        return False

    if PROJECT_NAME is None:
        print("Error: PROJECT_NAME is not set.")
        return False

    # 替换 {#PROJECT_NAME#} 占位符
    TEMPLATE_VCXPROJ_SRC = TEMPLATE_VCXPROJ_SRC.replace('{#PROJECT_NAME#}', PROJECT_NAME)
    print("Template modified successfully.")

    # 生成 GUID 并替换 {#GUID#} 占位符
    generated_guid = str(uuid.uuid4()).upper()  # 生成大写的 GUID
    TEMPLATE_VCXPROJ_SRC = TEMPLATE_VCXPROJ_SRC.replace('{#GUID#}', generated_guid)
    print(f"Generated GUID: {generated_guid}")

    return True

def setup_folders_and_files():
    """
    创建项目文件夹和 .vcxproj 文件：
    1. 在当前脚本所在文件夹下创建一个以 PROJECT_NAME 命名的文件夹。
    2. 在该文件夹中创建一个以 PROJECT_NAME 命名的 .vcxproj 文件，内容为 TEMPLATE_VCXPROJ_SRC。
    3. 将 TEMPLATE_VCXPROJ_FILTER_PATH 文件改名为 PROJECT_NAME.vcxproj.filters，并存储到同一目录下。
    """
    global PROJECT_NAME, TEMPLATE_VCXPROJ_SRC, TEMPLATE_VCXPROJ_FILTER_PATH

    if PROJECT_NAME is None:
        print("Error: PROJECT_NAME is not set.")
        return False

    if TEMPLATE_VCXPROJ_SRC is None:
        print("Error: TEMPLATE_VCXPROJ_SRC is not loaded.")
        return False

    # 获取当前脚本所在文件夹路径
    current_dir = os.path.dirname(os.path.abspath(__file__))

    # 创建以 PROJECT_NAME 命名的文件夹
    project_folder_path = os.path.join(current_dir, PROJECT_NAME)
    os.makedirs(project_folder_path, exist_ok=True)
    print(f"Created project folder: {project_folder_path}")

    # 创建以 PROJECT_NAME 命名的 .vcxproj 文件
    vcxproj_file_path = os.path.join(project_folder_path, f"{PROJECT_NAME}.vcxproj")
    with open(vcxproj_file_path, 'w', encoding='utf-8') as vcxproj_file:
        vcxproj_file.write(TEMPLATE_VCXPROJ_SRC)
    print(f"Created .vcxproj file: {vcxproj_file_path}")

    # 检查 TEMPLATE_VCXPROJ_FILTER_PATH 文件是否存在
    if os.path.exists(TEMPLATE_VCXPROJ_FILTER_PATH):
        # 读取 TEMPLATE_VCXPROJ_FILTER_PATH 文件内容
        with open(TEMPLATE_VCXPROJ_FILTER_PATH, 'r', encoding='utf-8') as filter_file:
            filter_content = filter_file.read()

        # 创建以 PROJECT_NAME 命名的 .vcxproj.filters 文件
        vcxproj_filters_file_path = os.path.join(project_folder_path, f"{PROJECT_NAME}.vcxproj.filters")
        with open(vcxproj_filters_file_path, 'w', encoding='utf-8') as filters_file:
            filters_file.write(filter_content)
        print(f"Created .vcxproj.filters file: {vcxproj_filters_file_path}")
    else:
        print(f"Filter template file does not exist: {TEMPLATE_VCXPROJ_FILTER_PATH}")

    return True

def setup_env(args):
    """
    使用 argparse 解析用户输入的命令行参数，并将项目名称赋值给全局变量 PROJECT_NAME。
    参数：
    - args: 命令行参数列表。
    """
    global PROJECT_NAME

    parser = argparse.ArgumentParser(description="Setup the environment for the project.")
    parser.add_argument('--project-name', type=str, required=True, help="Name of the project.")
    parsed_args = parser.parse_args(args)

    PROJECT_NAME = parsed_args.project_name
    print(f"Project name set to: {PROJECT_NAME}")

    # 获取当前脚本所在文件夹路径
    current_dir = os.path.dirname(os.path.abspath(__file__))

    # 检查是否已经存在与 PROJECT_NAME 同名的文件夹
    project_folder_path = os.path.join(current_dir, PROJECT_NAME)
    if os.path.exists(project_folder_path):
        print(f"Error: A folder named '{PROJECT_NAME}' already exists in the current directory.")
        return False
    return True

if __name__ == '__main__':
    """
    主函数：
    1. 解析命令行参数，设置项目名称。
    2. 加载模板文件内容。
    3. 修改模板内容。
    4. 创建项目文件夹和 .vcxproj 文件。
    """
    if setup_env(sys.argv[1:]) is False:  # 传递命令行参数（跳过脚本名）
        raise RuntimeError("Failed to setup env")
    if load_template_vcxproj() is False:
        raise RuntimeError("Failed to load template file.")
    if modify_vcproj() is False:
        raise RuntimeError("Failed to modify template content.")
    if setup_folders_and_files() is False:
        raise RuntimeError("Failed to set up folders and files.")