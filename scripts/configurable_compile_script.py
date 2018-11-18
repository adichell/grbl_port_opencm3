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
import logging
import argparse

def form_compile_flags(flags_line):
    result_line = ""
    name_line = ""
    flags = flags_line.split(' ')
    logging.debug(flags)

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

    logging.debug(result_line) ##debug print
    logging.debug(name_line) ##debug print

    return [result_line, name_line] 

def get_files_by_name2(regex):
    list_paths = []
    list_subpaths = []
    for name in sorted(glob.glob(regex)):
        logging.debug(os.path.abspath(name))
        logging.debug(name.split(os.sep)[-3])
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



#def main(argv):
def compile(flagsFile, toolPathsFile, verbosity, versionString):
    result_of_operations = 0

    print "Compile script execution started."
    
    with open(flagsFile, 'r') as myfile:
        flags_lines = [line.rstrip('\n') for line in myfile]

    logging.debug("Flags combos list:")
    logging.debug(flags_lines)

    ##Read the tool path list that has to be modified according to
    ##the locally installed versions of the used tools.
    with open(toolPathsFile, 'r') as myfile:
        pathlist = [line.rstrip('\n') for line in myfile]

    logging.debug("Tools paths list: ")
    logging.debug(pathlist)
    logging.debug("Original PATH environment variable: ")
    logging.debug(os.environ["PATH"])

    os.environ["PATH"] = os.pathsep.join(pathlist) + os.pathsep + os.environ["PATH"]    
    
    logging.debug("Modified PATH environment variable: ")
    logging.debug(os.environ["PATH"])

    dirname = os.path.dirname(os.path.abspath(__file__))
    lib_dirname = os.path.join(dirname, ('..' + os.sep + 'libopencm3' + os.sep + 'lib'))
    logging.debug(dirname)
    logging.debug(lib_dirname)

    os.chdir('..')

    ## Create Artifacts folder if it doesn't exist.
    artifacts_folder = os.path.join(dirname, 'artifacts_built')
    logging.debug(artifacts_folder)
    if (os.path.isdir(artifacts_folder)):
        logging.debug('Artifacts folder exists already')
    else:
        logging.debug('Artifacts folder did not exists')
        os.mkdir(artifacts_folder)
        logging.debug('Artifacts folder created')

    ## Clean Artifacts folder removing previous compilation files.
    for file in glob.glob(artifacts_folder + os.sep + '*.bin'):
        os.remove(file)

    ## Check if the libraries are already compiled
    ## and choose if recompile just the application
    ## or everything.
    check_lib_flag = 0
    for fname in os.listdir(lib_dirname):
        if fname.endswith('.a'):
            check_lib_flag = 1

    if check_lib_flag == 0:
        externalCommand = 'make clean'
        print "Executing command : " + externalCommand
        for line in run_command(externalCommand):
            if "error" in line:
                result_of_operations = -1
            print line

        externalCommand = 'make lib'
        print "Executing command : " + externalCommand
        for line in run_command(externalCommand):
            if "error" in line:
                result_of_operations = -1
            print line

    ## Execute various compilations passing defines
    for elem in flags_lines:

        [define_line, name_line]  = form_compile_flags(elem)

        ## Make compilation verbose if there is a DEBUG logging level ##
        define_line += verbosity

        ## Make compilation verbose if there is a DEBUG logging level ##
        define_line += versionString

        # Clean ...
        externalCommand = 'make clean_grbl'
        result = subprocess.Popen(externalCommand,shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[0]
        print "Executing command : " + externalCommand
        print result
        if "error" in result:
            result_of_operations = -1


        # ... and build
        externalCommand = 'make grbl '+define_line+' '
        print "Executing command : " + externalCommand
        for line in run_command(externalCommand):
            if "error" in line:
                result_of_operations = -1
            print line

        build_filepath = '..' + os.sep + 'grbl_port' + os.sep + 'stm32' + os.sep + '*' + os.sep + '*' + os.sep + '*' + os.sep + 'main*.bin'
        (old_file_names,file_name_subpaths)=get_files_by_name2(os.path.join(dirname, build_filepath))
        logging.info(old_file_names)
        logging.info(file_name_subpaths)

        ## Copy and rename compiled file into the artifacts folder
        for ofn,fns in zip(old_file_names,file_name_subpaths):
            logging.debug(artifacts_folder)
            old_file_name = os.path.join(dirname, ofn)
            new_file_name = os.path.join(artifacts_folder,('gocm3_' + fns + name_line + '.bin'))
            logging.info(new_file_name)
            shutil.copy(old_file_name, new_file_name)

    print "Compile script execution ended."

    sys.exit(result_of_operations)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Configurable compilation script start.')
    parser.add_argument('-f', '--flagsFile', help='File that contains the compilation flags to be used. Every line is a build type.', default='flags_combos.txt')
    parser.add_argument('-t', '--toolPathsFile', help='File that contains the paths of the tools to use to build the application. Every line is a tool path.')
    parser.add_argument('-maj', '--majorVersion', type=int, default=0, help='Version MAJOR number')
    parser.add_argument('-min', '--minorVersion', type=int, default=1, help='Version minor number')
    parser.add_argument('-bui', '--buildVersion', type=int, default=0, help='Version build number')
    parser.add_argument('-l', '--logLevel', help='Logging level.')

    args = parser.parse_args()

    if os.name == 'nt':
        toolPathsFile = 'tools_paths_windows.txt'
    elif os.name == 'posix':
        toolPathsFile = 'tools_paths_unix.txt'

    if(args.toolPathsFile):
        toolPathsFile = args.toolPathsFile

    versionString = 'VER=' + str(args.majorVersion) + '.' + str(args.minorVersion) + '.' + str(args.buildVersion)

    verbosity = ''

    # assuming loglevel is bound to the string value obtained from the
    # command line argument. Convert to upper case to allow the user to
    # specify --log=DEBUG or --log=debug
    if (args.logLevel):
        numeric_level = getattr(logging, args.logLevel.upper(), None)
        if not isinstance(numeric_level, int):
            raise ValueError('Invalid log level: %s' % loglevel)
        logging.basicConfig(level=numeric_level)
        if numeric_level < 20 :
            verbosity_string = ' V=1'

    compile(args.flagsFile, toolPathsFile, verbosity, versionString)
