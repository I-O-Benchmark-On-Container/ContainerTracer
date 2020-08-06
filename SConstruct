import scons_compiledb
import platform
import subprocess
import os

SConscript('setting/SConscript')

Import('env')
scons_compiledb.enable(env)

if env["BUILD_UNIT_TEST"]:
	SConscript("unity/SConscript")
SConscript("include/SConscript")

sub_project = {
	'trace-replay/SConscript': env['TRACE_REPLAY'],
	'runner/SConscript': env['RUNNER'],
}

# 프로그램 존재 여부 확인
dependency = ["clang-format", "cppcheck"]
cmd = "where" if platform.system() == "Windows" else "which"

current_check_program = "None"
try:
    for program in dependency:
        current_check_program = program
        subprocess.call([cmd, program], stdout=subprocess.DEVNULL)
except:
    print(current_check_program+"이 설치되지 않았습니다.", file=sys.stderr)

# log 기록을 위한 디렉터리의 생성
os.makedirs(str(Dir(env["LOG_LOCATION"])), exist_ok=True)

# clang-format 수행
clang_format_ouput_file = open(
    str(Dir(env["LOG_LOCATION"]))
    + os.sep
    + dependency[0]
    + ".log",
    "w",
)
clang_format_error_file = open(
    str(Dir(env["LOG_LOCATION"]))
    + os.sep
    + dependency[0]
    + ".error",
    "w",
)
proc = subprocess.Popen(
    "find ."+os.sep+" -iname '*.h' -o -iname '*.c' | xargs "+ str(dependency[0]) +' -i',
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

if env.GetOption('help'):
	env.PrintHelp()

env.CompileDb()

# cppcheck 수행
cppcheck_ouput_file = open(
    str(Dir(env["LOG_LOCATION"]))
    + os.sep
    + dependency[1]
    + ".log",
    "w",
)
cppcheck_error_file = open(
    str(Dir(env["LOG_LOCATION"]))
    + os.sep
    + dependency[1]
    + ".error",
    "w",
)
subprocess.call(
    [dependency[1], "--check-config","--enable=all", "--project=./compile_commands.json"],
    stdout=cppcheck_ouput_file,
    stderr=cppcheck_error_file,
)
cppcheck_ouput_file.close()
cppcheck_error_file.close()


