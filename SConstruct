"""
Copyright (c) 2008, Kalisko Project Leaders
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
      in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""
import os
import re
import kic

kic.buildInterfaces()

modules = os.listdir('src/modules')

def CheckPKGConfig(context, name):
     context.Message( 'Checking pkg-config for %s... ' % name )
     ret = context.TryAction('pkg-config --exists %s' % name)[0]
     context.Result( ret )
     return ret

custom_tests = {'CheckPKGConfig' : CheckPKGConfig}

vars = Variables()
vars.Add(BoolVariable('verbose', 'Show command line invocations', 0))
vars.Add(BoolVariable('release', 'Set to build release target', 1))
vars.Add(BoolVariable('debug', 'Set to build debug target', 0))
vars.Add(BoolVariable('test', 'Set to build test target', 0))
vars.Add(BoolVariable('force_zero_revision', 'Force Mercurial revision to zero to prevent rebuilds after committing when developing', 0))
vars.Add(ListVariable('modules', 'Attempts to build the selected modules', [], modules))
vars.Add(ListVariable('exclude', 'Prevents the selected modules from being built', [], modules))
vars.Add('define', 'Custom defines to be passed to the compiler as DEFINE1[:VALUE1],DEFINE2[:VALUE2],...', '')
vars.Add('cflags_release', 'Flags to be passed to the compiler when compiling the release target', '-pipe -Os')
vars.Add('cflags_debug', 'Flags to be passed to the compiler when compiling the debug target', '-pipe -g')
vars.Add('cflags_test', 'Flags to be passed to the compiler when compiling the test target', '-pipe -g')

testenv = Environment(ENV = os.environ)
if testenv['PLATFORM'] == 'win32':
	tools = ['mingw', 'yacc']
else:
	tools = testenv['TOOLS']

env = Environment(variables = vars, CCFLAGS = ['-Wall'], CFLAGS = ['-std=gnu99'], ENV = os.environ, CPPPATH = ['.','#src'], YACC = 'bison', YACCFLAGS = ['-d','-Wall','--report=all'], TOOLS = tools)
Help(vars.GenerateHelpText(env))

if env['force_zero_revision']:
	srcversion = '0'
else:
	# Fetch current hg revision
	cmd = os.popen('hg summary')
	srcversion = cmd.read()
	matches = re.search('^.*: (\d+):', srcversion)
	srcversion = matches.group(1)
	
env.Append(CPPDEFINES = [('SRC_REVISION', srcversion)])

cppdefines = []
defines = env['define'].split(',')
for value in defines:
	keyvalue = value.split(':', 1)
	if len(keyvalue) == 2:
		cppdefines = cppdefines + [(keyvalue[0], keyvalue[1])]
	elif len(keyvalue) == 1:
		cppdefines = cppdefines + [keyvalue[0]]
		
env.Append(CPPDEFINES = cppdefines)

if not env['verbose']:
	env.Replace(CCCOMSTR = 'Compiling object: $TARGET')
	env.Replace(CXXCOMSTR = 'Compiling C++ object: $TARGET')
	env.Replace(LINKCOMSTR = 'Linking executable: $TARGET')
	env.Replace(SHCCCOMSTR = 'Compiling shared object: $TARGET')
	env.Replace(SHCXXCOMSTR = 'Compiling shared C++ object: $TARGET')
	env.Replace(SHLINKCOMSTR = 'Linking library: $TARGET')
	env.Replace(YACCCOMSTR = 'Generating parser: $TARGET')

if env['PLATFORM'] == 'win32':
	env.Append(LINKFLAGS = ['-Wl,--export-all-symbols', '-Wl,--enable-auto-import'])
	env['WINDOWS_INSERT_DEF'] = True
else:
	env.Append(LINKFLAGS = ['-Wl,--export-dynamic'])

# Add glib support
env.ParseConfig('pkg-config --cflags --libs glib-2.0 gthread-2.0')

# Build modes
targets = []

if env['release']:
	tenv = env.Clone()
	tenv.MergeFlags(env['cflags_release'])
	tdir = 'bin/release'
	tenv.VariantDir(tdir, 'src', 0)
	targets = targets + [(tdir, tenv, False)]

if env['debug']:
	tenv = env.Clone()
	tenv.MergeFlags(env['cflags_debug'])
	tdir = 'bin/debug'
	tenv.VariantDir(tdir, 'src', 0)
	targets = targets + [(tdir, tenv, False)]
	
if env['test']:
	tenv = env.Clone()
	tenv.MergeFlags(env['cflags_test'])
	tdir = 'bin/test'
	tenv.VariantDir(tdir, 'src', 0)
	targets = targets + [(tdir, tenv, True)]
	
for prefix, env, buildtests in targets:
	# Build modules
	for moddir in modules:
		if os.path.isdir(os.path.join('src/modules', moddir)) and moddir in env['modules'] and not moddir in env['exclude']:
			if os.path.isfile(os.path.join('src/modules', moddir, 'SConscript')):
				# Build module
				module = env.Clone()
				module.Append(CPPDEFINES = [('KALISKO_MODULE', moddir)])
				SConscript(os.path.join(prefix, 'modules', moddir, 'SConscript'), ['module', 'custom_tests'])
	
	# Build tests
	if buildtests:
		# Build tester core
		core = env.Clone()
		core.Append(CPPDEFINES = ['DISABLE_LOG_DEFAULT'])
		corefiles = [x for x in core.Glob('bin/test/*.c') if not x.name == 'kalisko.c']
		SConscript(os.path.join(prefix, 'SConscript'), ['core', 'corefiles'])
	
		# Build test suites
		tests = os.listdir('src/tests')
		
		for testdir in tests:
			if os.path.isdir(os.path.join('src/tests', testdir)) and moddir in env['modules'] and not testdir in env['exclude']:
				if os.path.isfile(os.path.join('src/tests', testdir, 'SConscript')):
					# Build test
					test = env.Clone()		
					SConscript(os.path.join(prefix, 'tests', testdir, 'SConscript'), ['test', 'custom_tests'])
					
	else:
		# Build core
		core = env.Clone()
		corefiles = [x for x in core.Glob(os.path.join(prefix, '*.c')) if not x.name == 'test.c']
		SConscript(os.path.join(prefix, 'SConscript'), ['core', 'corefiles'])
					
