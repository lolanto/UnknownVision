import os
import sys

PLATFORM = ''
CONFIGURATION = ''

FILE_LIST = ['dxc.exe', 'dxcompiler.dll', 'dxil.dll']

def Link_Bin_File():
    trdPart_bin_path = os.path.join('..\\..\\..\\3rdPart\\bin', PLATFORM, CONFIGURATION)
    bin_path=  os.path.join('..\\', PLATFORM, CONFIGURATION)
    for x in FILE_LIST:
        cmd = 'mklink ' + os.path.join(bin_path, x) + ' ' + os.path.join(trdPart_bin_path, x)
        print(cmd)
        os.system(cmd)

if __name__ == '__main__':
    PLATFORM = sys.argv[1]
    CONFIGURATION = sys.argv[2]
    Link_Bin_File()