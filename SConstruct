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

if ARGUMENTS.get('force_zero_revision') == '1':
	srcversion = '0'
else:
	# Fetch current hg revision
	cmd = os.popen('hg summary')
	srcversion = cmd.read()
	matches = re.search('^.*: (\d+):', srcversion)
	srcversion = matches.group(1)

# Build lists from given arguments
exclude = []
cppdefines = []
for key, value in ARGLIST:
	if key == 'exclude':
		exclude = exclude + value.split(',')
	elif key == 'define':
		keyvalue = value.split(':', 1)
		if len(keyvalue) == 2:
			cppdefines = cppdefines + [(keyvalue[0], keyvalue[1])]
		elif len(keyvalue) == 1:
			cppdefines = cppdefines + [keyvalue[0]]

# Config section
ccflags = ['-std=gnu99', '-Wall', '-pipe']

# Start of actual makefile
dummy = Environment(ENV = os.environ)

if dummy['PLATFORM'] == 'win32':
	coretpl = Environment(CPPDEFINES = [('SRC_REVISION', srcversion)] + cppdefines, CCFLAGS = ccflags, LINKFLAGS = ['-Wl,--export-dynamic'], ENV = os.environ, CPPPATH = ['.'], TOOLS = ['mingw', 'yacc'])
	modtpl = Environment(CPPDEFINES = [('SRC_REVISION', srcversion)] + cppdefines, CCFLAGS = ccflags, ENV = os.environ, CPPPATH = ['.','#src'], YACC = 'bison', YACCFLAGS = ['-d','-Wall','--report=all'], TOOLS = ['mingw', 'yacc'])	
else:
	coretpl = Environment(CPPDEFINES = [('SRC_REVISION', srcversion)] + cppdefines, CCFLAGS = ccflags, LINKFLAGS = ['-Wl,--export-dynamic'], ENV = os.environ, CPPPATH = ['.'])
	modtpl = Environment(CPPDEFINES = [('SRC_REVISION', srcversion)] + cppdefines, CCFLAGS = ccflags, ENV = os.environ, CPPPATH = ['.','#src'], YACC = 'bison', YACCFLAGS = ['-d','-Wall','--report=all'])

# Add glib support
coretpl.ParseConfig('pkg-config --cflags --libs glib-2.0 gthread-2.0')
modtpl.ParseConfig('pkg-config --cflags --libs glib-2.0 gthread-2.0')

buildtests = True

# Build modes
if ARGUMENTS.get('debug') == '0' or ARGUMENTS.get('release') == '1':
	prefix = 'bin/release'
	coretpl.Append(CCFLAGS = ['-O2'])
	coretpl.VariantDir(prefix, 'src', 0)
	modtpl.Append(CCFLAGS = ['-O2'])
	modtpl.VariantDir(prefix, 'src', 0)
	buildtests = False
else:
	prefix = 'bin/debug'
	coretpl.Append(CCFLAGS = ['-g'])
	coretpl.VariantDir(prefix, 'src', 0)
	coretpl.VariantDir('bin/test', 'src', 0)
	modtpl.Append(CCFLAGS = ['-g'])
	modtpl.VariantDir(prefix, 'src', 0)
	modtpl.VariantDir('bin/test', 'src', 0)
	
if ARGUMENTS.get('verbose') != '1':
	coretpl.Replace(CCCOMSTR = 'Compiling object: $TARGET')
	coretpl.Replace(LINKCOMSTR = 'Linking executable: $TARGET')
	modtpl.Replace(SHCCCOMSTR = 'Compiling shared object: $TARGET')
	modtpl.Replace(SHLINKCOMSTR = 'Linking library: $TARGET')
	modtpl.Replace(YACCCOMSTR = 'Generating parser: $TARGET')

# Build core
core = coretpl.Clone()
corefiles = [x for x in core.Glob(os.path.join(prefix, '*.c')) if not x.name == 'test.c']
SConscript(os.path.join(prefix, 'SConscript'), ['core', 'corefiles'])

# Build modules
modules = os.listdir('src/modules')

for moddir in modules:
	if os.path.isdir(os.path.join('src/modules', moddir)) and not moddir in exclude:
		if os.path.isfile(os.path.join('src/modules', moddir, 'SConscript')):
			# Build module
			module = modtpl.Clone()
			SConscript(os.path.join(prefix, 'modules', moddir, 'SConscript'), 'module')
			if buildtests:
				module = modtpl.Clone()
				SConscript(os.path.join('bin/test', 'modules', moddir, 'SConscript'), 'module')

# Build tests
if buildtests:
	# Build tester core
	core = coretpl.Clone()
	corefiles = [x for x in core.Glob('bin/test/*.c') if not x.name == 'kalisko.c']
	SConscript('bin/test/SConscript', ['core', 'corefiles'])

	# Build test cases
	tests = os.listdir('src/tests')
	
	for testdir in tests:
		if os.path.isdir(os.path.join('src/tests', testdir)) and not testdir in exclude:
			if os.path.isfile(os.path.join('src/tests', testdir, 'SConscript')):
				# Build test
				test = modtpl.Clone()		
				SConscript(os.path.join('bin/test/tests', testdir, 'SConscript'), 'test')