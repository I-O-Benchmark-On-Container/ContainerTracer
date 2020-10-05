import scons_compiledb
import platform
import subprocess
import os
import distro

SConscript("setting/SConscript")

Import("env")
scons_compiledb.enable(env)

if env["BUILD_UNIT_TEST"] == True:
    if os.getuid() != 0:
        print("[ERROR] `test` mode requires the sudo privileges.", file=sys.stderr)
        sys.exit()

    if env["TARGET_DEVICE"] == "":
        print(
            "[ERROR] you must specify the TARGET_DEVICE (e.g. nvme0n1)", file=sys.stderr
        )
        sys.exit()
    else:
        env.Append(CFLAGS=["-DTEST_DISK_PATH='\"{}\"'".format(env["TARGET_DEVICE"])])

env.Append(CFLAGS=["-O2"])
env["CC"] = ["gcc"]

is_redhat = (
    len(
        set(["fedora", "redhat", "centos"])
        & set([s.lower() for s in distro.linux_distribution()])
    )
    > 0
)
if is_redhat:
    env.Append(CFLAGS=["-DREDHAT_LINUX"])


if env["DEBUG"] == True:
    env.Append(CFLAGS=["-g", "-pg"])
    env.Append(CFLAGS=["-DLOG_INFO", "-DDEBUG"])
    env.Append(LINKFLAGS=["-g", "-pg"])
    env["PROGRAM_LOCATION"] = "#build/debug"


if env["BUILD_UNIT_TEST"]:
    SConscript("unity/SConscript")
SConscript("include/SConscript")

sub_project = {
    "trace-replay/SConscript": env["TRACE_REPLAY"],
    "runner/SConscript": env["RUNNER"],
}

# Program existence check.
dependency = ["clang-format", "cppcheck"]
cmd = "where" if platform.system() == "Windows" else "which"

current_check_program = "None"
try:
    for program in dependency:
        current_check_program = program
        subprocess.call([cmd, program], stdout=subprocess.DEVNULL)
except:
    print(current_check_program + "isn't properlly installed.", file=sys.stderr)

# Make a directory for log
os.makedirs(str(Dir(env["LOG_LOCATION"])), exist_ok=True)

# Execute the clang-format
clang_format_ouput_file = open(
    str(Dir(env["LOG_LOCATION"])) + os.sep + dependency[0] + ".log",
    "w",
)
clang_format_error_file = open(
    str(Dir(env["LOG_LOCATION"])) + os.sep + dependency[0] + ".error",
    "w",
)
proc = subprocess.Popen(
    "find ."
    + os.sep
    + " -iname '*.h' -o -iname '*.c' | xargs "
    + str(dependency[0])
    + " -i",
    shell=True,
    stdout=clang_format_ouput_file,
    stderr=clang_format_error_file,
)
proc.communicate()
clang_format_ouput_file.close()
clang_format_error_file.close()

for sub_project_root, do_make in sub_project.items():
    if do_make:
        SConscript(sub_project_root)

if env.GetOption("help"):
    env.PrintHelp()

env.CompileDb()

# Execute the cppcheck
cppcheck_ouput_file = open(
    str(Dir(env["LOG_LOCATION"])) + os.sep + dependency[1] + ".log",
    "w",
)
cppcheck_error_file = open(
    str(Dir(env["LOG_LOCATION"])) + os.sep + dependency[1] + ".error",
    "w",
)
subprocess.call(
    [
        dependency[1],
        "--check-config",
        "--enable=all",
        "--project=./compile_commands.json",
    ],
    stdout=cppcheck_ouput_file,
    stderr=cppcheck_error_file,
)
cppcheck_ouput_file.close()
cppcheck_error_file.close()

if env.GetOption("clean"):
    print("Delete All Object file manually")
    os.system('find ./ -name "*.o" -print0 | xargs -0 rm -f ')
    os.system('find ./ -name "*.os" -print0 | xargs -0 rm -f ')
    os.system("rm -rf ./build")
