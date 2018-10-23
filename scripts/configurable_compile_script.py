#!/usr/bin/env python2
__author__ = 'Angelo Di Chello'
__copyright__ = "Copyright 2018"
__license__ = "GPL3"
__version__ = "0.1"
__status__ = "Prototype"

import sys
import os
import subprocess
import shutil
import glob

def form_compile_flags(flags_line):
    result_line = ""
    flags = flags_line.split(' ')
    # print flags

    #Calculate final line
    for fl in flags:
        if fl.find("DEFAULTS") != -1 :
            result_line += ' CFLAGS+=-DDEFAULTS_DEFINED CFLAGS+=-D' + fl + ' '
        elif fl.find("PWM_SPINDLE_PARAMS") != -1 :
            result_line += ' CFLAGS+=-PWM_PARAMS_DEFINED CFLAGS+=-D' + fl + ' '
        else:
            result_line += 'CFLAGS+=-D' + fl + ' '

    return result_line

def copy_rename(old_file_name, new_file_name):
        src_dir= os.curdir
        dst_dir= os.path.join(os.curdir , "subfolder")
        src_file = os.path.join(src_dir, old_file_name)
        shutil.copy(src_file,dst_dir)

        dst_file = os.path.join(dst_dir, old_file_name)
        new_dst_file_name = os.path.join(dst_dir, new_file_name)
        os.rename(dst_file, new_dst_file_name)

def get_files_by_name2(regex):
    list_paths = []
    list_subpaths = []
    for name in sorted(glob.glob(regex)):
        #print os.path.abspath(name)
        #print name.split(os.sep)[-3]
        list_paths.append(os.path.abspath(name))
        list_subpaths.append(name.split(os.sep)[-3])

    return (list_paths,list_subpaths)

def runProcess(exe):    
    p = subprocess.Popen(exe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    while(True):
      retcode = p.poll() #returns None while subprocess is running
      line = p.stdout.readline()
      yield line
      if(retcode is not None):
        break

def run_command(command):
    p = subprocess.Popen(command,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    return iter(p.stdout.readline, b'')

def main(argv):
    ##Read the tool path list that has to be modified according to
    ##the locally installed versions of the used tools.
    with open('tools_paths.txt', 'r') as myfile:
        pathlist = [line.rstrip('\n') for line in myfile]

    print "Tools paths list: "
    print pathlist

    with open('flags_combos.txt', 'r') as myfile:
        flags_lines = [line.rstrip('\n') for line in myfile]

    #print "Flags combos list: "
    #print flags_lines

    os.chdir('..')

    ##print "Original PATH environment variable: "
    ##print os.environ["PATH"]
    os.environ["PATH"] = os.pathsep.join(pathlist) + os.pathsep + os.environ["PATH"]
    print "Modified PATH environment variable: "
    print os.environ["PATH"]

    dirname = os.path.dirname(__file__)
    lib_dirname = os.path.join(dirname, '..\libopencm3\lib')
    ##print dirname
    ##print lib_dirname

    #Check if the libraries are already compiled
    #and choose if recompile just the application
    #or everything.
    check_lib_flag = 0
    for fname in os.listdir(lib_dirname):
        if fname.endswith('.a'):
            check_lib_flag = 1

    if check_lib_flag:
        ## Execute various compilations passing defines
        for elem in flags_lines:

            define_line = form_compile_flags(elem)

            # Clean ...
            externalCommand = 'make clean_grbl'
            result = subprocess.Popen(externalCommand, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
            print "Executing command : " + externalCommand
            print result

            # ... and build
            externalCommand = 'make grbl '+define_line+' '
            print "Executing command : " + externalCommand
            for line in run_command(externalCommand):
                print line

            (old_file_names,file_name_subpaths)=get_files_by_name2(os.path.join(dirname,'../grbl_port/stm32/*/*/*/main*.bin'))
            print old_file_names
            print file_name_subpaths
            old_file_name = os.path.join(dirname, '../grbl_port/stm32/f4/nucleo-f401re/build_dir/main.bin')
            new_file_name = os.path.join(dirname,'main_copied.bin')
            #copy_rename(old_file_name, new_file_name)

    else:
        externalCommand = 'make clean'
        os.system(externalCommand)

        externalCommand = 'make'
        os.system(externalCommand)

    print "Compile script execution ended."

    pass

if __name__ == "__main__":
    main(sys.argv)

