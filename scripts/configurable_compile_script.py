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
    name_line = ""
    flags = flags_line.split(' ')
    # print flags

    #Calculate final line
    for fl in flags:
        if fl.find("DEFAULTS") != -1 :
            result_line += ' CFLAGS+=-DDEFAULTS_DEFINED CFLAGS+=-D' + fl + ' '
            temp_string = fl
            name_line += temp_string.replace('DEFAULTS','')
        elif fl.find("PWM_SPINDLE_PARAMS") != -1 :
            result_line += ' CFLAGS+=-DPWM_PARAMS_DEFINED CFLAGS+=-D' + fl + ' '
            temp_string = fl
            name_line += temp_string.replace('PWM_SPINDLE_PARAMS','')
        else:
            result_line += 'CFLAGS+=-D' + fl + ' '
            name_line += '_' + fl

    print result_line ##debug print
    print name_line ##debug print

    return [result_line, name_line] 

def get_files_by_name2(regex):
    list_paths = []
    list_subpaths = []
    for name in sorted(glob.glob(regex)):
        ##print os.path.abspath(name)
        ##print name.split(os.sep)[-3]
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
    p = subprocess.Popen(command, shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    return iter(p.stdout.readline, b'')



def main(argv):

    with open('flags_combos.txt', 'r') as myfile:
        flags_lines = [line.rstrip('\n') for line in myfile]

    #print "Flags combos list: "
    #print flags_lines

    if os.name == 'nt':
        ##Read the tool path list that has to be modified according to
        ##the locally installed versions of the used tools.
        with open('tools_paths_windows.txt', 'r') as myfile:
            pathlist = [line.rstrip('\n') for line in myfile]

        print "Tools paths list: "
        print pathlist
        ##print "Original PATH environment variable: "
        ##print os.environ["PATH"]
        os.environ["PATH"] = os.pathsep.join(pathlist) + os.pathsep + os.environ["PATH"]    
    elif os.name == 'posix':
        ##Read the tool path list that has to be modified according to
        ##the locally installed versions of the used tools.
        with open('tools_paths_unix.txt', 'r') as myfile:
            pathlist = [line.rstrip('\n') for line in myfile]

        print "Tools paths list: "
        print pathlist
        ##print "Original PATH environment variable: "
        ##print os.environ["PATH"]
        os.environ["PATH"] = os.pathsep.join(pathlist) + os.pathsep + os.environ["PATH"]

    print "Modified PATH environment variable: "
    print os.environ["PATH"]

    

    dirname = os.path.dirname(os.path.abspath(__file__))
    lib_dirname = os.path.join(dirname, ('..' + os.sep + 'libopencm3' + os.sep + 'lib'))
    print dirname
    print lib_dirname

    os.chdir('..')
    
    #Check if the libraries are already compiled
    #and choose if recompile just the application
    #or everything.
    check_lib_flag = 0
    for fname in os.listdir(lib_dirname):
        if fname.endswith('.a'):
            check_lib_flag = 1

    if check_lib_flag:
        ## Create Artifacts folder if it doesn't exist.
        artifacts_folder = os.path.join(dirname, 'artifacts_built')
        print artifacts_folder #debug print
        if (os.path.isdir(artifacts_folder)):
            print 'Artifacts folder exists already'
        else:
            print 'Artifacts folder did not exists'
            os.mkdir(artifacts_folder)

        ## Clean Artifacts folder removing previous compilation files.
        for file in glob.glob(artifacts_folder + os.sep + '*.bin'):
            os.remove(file)

        ## Execute various compilations passing defines
        for elem in flags_lines:

            [define_line, name_line]  = form_compile_flags(elem)

            # Clean ...
            externalCommand = 'make clean_grbl'
            result = subprocess.Popen(externalCommand,shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
            print "Executing command : " + externalCommand
            print result

            # ... and build
            externalCommand = 'make grbl '+define_line+' '
            print "Executing command : " + externalCommand
            for line in run_command(externalCommand):
                print line

            build_filepath = '..' + os.sep + 'grbl_port' + os.sep + 'stm32' + os.sep + '*' + os.sep + '*' + os.sep + '*' + os.sep + 'main*.bin'
            (old_file_names,file_name_subpaths)=get_files_by_name2(os.path.join(dirname, build_filepath))
            print old_file_names
            print file_name_subpaths

            ## Copy and rename compiled file into the artifacts folder
            for ofn,fns in zip(old_file_names,file_name_subpaths):
                #print artifacts_folder #debug print
                old_file_name = os.path.join(dirname, ofn)
                new_file_name = os.path.join(artifacts_folder,('gocm3_' + fns + name_line + '.bin'))
                print new_file_name #debug print
                shutil.copy(old_file_name, new_file_name)

    else:
        externalCommand = 'make clean'
        os.system(externalCommand)

        externalCommand = 'make'
        os.system(externalCommand)

    print "Compile script execution ended."

    pass

if __name__ == "__main__":
    main(sys.argv)

