
from sys import argv
from sys import exit
from subprocess import run
import getopt
import os.path
import shutil


def read_packages(location):
    from os import listdir
    from os.path import isfile, join

    available_packages = {}

    # load packages
    entries = listdir(location)
    for entry in entries:
        full_path = join(location, entry)
        if entry[-3:] == ".py" and isfile(full_path):
            file_content = open(full_path).read()
            exec(file_content)

    return available_packages


def usage(avail_packages):

    name = os.path.basename(argv[0])

    print("Usage:")
    print(name, "[options]", "destination dir")
    print("")
    print("Possible options:")
    print("-p <name>      Download nad install package.")
    print("               Possible packages:")
    print("               " + ", ".join(avail_packages))
    print("               Option can be reapeted.")
    print("-g <generator> Generator to be used by CMake.")
    print("               See cmake --help, 'Generators' section for more details")


def is_exe(path):
    return os.path.isfile(path) and os.access(path, os.X_OK)


def main(argv):

    # read available packages
    packages = read_packages("./dependencies")
    packages_names = packages.keys()

    # no arguments? Show usage
    if len(argv) == 1:
        usage(packages_names)
        exit()

    libraries = []
    destination_dir = ""
    generator = ""

    # parse arguments
    try:
        opts, args = getopt.getopt(argv[1:], "g:hp:", ["help"])
    except getopt.GetoptError:
        print("Invalid arguments provided")
        print("See -h for help.")
        exit(2)

    # collect data
    for opt, arg in opts:
        if opt in ['-h', '--help']:
            usage(packages_names)
            exit()
        elif opt == "-g":
            generator = arg
        elif opt == "-p":
            if (arg in packages_names):
                libraries.append(arg)
            else:
                print("'" + arg + "' is not valid package name.")
                print("See -h for help.")
                exit(2)

    # we expect one argument (destination dir) in args
    if len(args) == 0:
        print("No destination dir was provided.")
        print("See -h for help.")
        exit(2)

    if len(args) > 1:
        print("Too many desination dirs. Only one is expected.")
        print("See -h for help.")
        exit(2)

    # construct CMakeLists.txt
    destination_dir = args[0]
    cmake_lists_path = os.path.join(destination_dir, "CMakeLists.txt")

    cmake_lists_header = open('./templates/dependencies_header.cmake').read()

    cmake_lists_file = open(cmake_lists_path, 'w')
    cmake_lists_file.write(cmake_lists_header)

    current_dir = os.getcwd()
    os.chdir('./dependencies')
    for lib in libraries:
        print("Building " + lib)
        if packages[lib](cmake_lists_file) != 0:
            break

    os.chdir(current_dir)

    cmake_lists_file.close()

    # create build dir
    build_dir = os.path.join(destination_dir, "build")
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    # run cmake
    cmake_executable = shutil.which('cmake')

    if is_exe(cmake_executable):

        cmake_args = [cmake_executable, destination_dir]

        if generator != "":
            cmake_args.append("-G")
            cmake_args.append(generator)

        result = run(cmake_args, cwd = build_dir)
        result = run([cmake_executable, "--build", "."], cwd = build_dir)
    else:
        print("Could not find 'cmake' in PATH")
        exit(2)


if __name__ == "__main__":
   main(argv[0:])