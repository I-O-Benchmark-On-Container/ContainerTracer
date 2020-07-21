import scons_compiledb

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

for sub_project_root, do_make in sub_project.items():
	if do_make:
		SConscript(sub_project_root)

if env.GetOption('help'):
	env.PrintHelp()

env.CompileDb()
