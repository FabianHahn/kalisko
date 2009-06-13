import os

# Build exclude list
exclude = []
for key, value in ARGLIST:
	if key == 'exclude':
		exclude = exclude + [value]

# Config section
ccflags = ['-std=gnu99', '-Wall', '-pipe']

# Start of actual makefile
coretpl = Environment(CCFLAGS = ccflags, LINKFLAGS = ['-Wl,--export-dynamic'], ENV = os.environ, CPPPATH = ['.'])
modtpl = Environment(CCFLAGS = ccflags, ENV = os.environ, CPPPATH = ['.','#src'])

# Add glib support
coretpl.ParseConfig('pkg-config --cflags --libs glib-2.0')
modtpl.ParseConfig('pkg-config --cflags --libs glib-2.0')

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

# Build core
core = coretpl.Clone()
if coretpl['PLATFORM'] == 'win32':
	core.Append(LINKFLAGS = ['-Wl,--out-implib,' + prefix + '/libkalisko.a'])
corefiles = [x for x in core.Glob(os.path.join(prefix, '*.c')) if not x.name == 'test.c']
SConscript(os.path.join(prefix, 'SConscript'), ['core', 'corefiles'])

# Build modules
modules = os.listdir('src/modules')

for moddir in modules:
	if os.path.isdir(os.path.join('src/modules', moddir)) and not moddir in exclude:
		if os.path.isfile(os.path.join('src/modules', moddir, 'SConscript')):
			# Build module
			module = modtpl.Clone()
			if modtpl['PLATFORM'] == 'win32':
				module.Append(LIBPATH = ['#' + prefix, '#' + prefix + '/modules'])
			SConscript(os.path.join(prefix, 'modules', moddir, 'SConscript'), 'module')
			if buildtests:
				module = modtpl.Clone()
				if modtpl['PLATFORM'] == 'win32':
					module.Append(LIBPATH = ['#bin/test', '#bin/test/modules'])
				SConscript(os.path.join('bin/test', 'modules', moddir, 'SConscript'), 'module')

# Build tests
if buildtests:
	# Build tester core
	core = coretpl.Clone()
	if coretpl['PLATFORM'] == 'win32':
		core.Append(LINKFLAGS = ['-Wl,--out-implib,bin/test/libkalisko.a'])	
	corefiles = [x for x in core.Glob('bin/test/*.c') if not x.name == 'kalisko.c']
	SConscript('bin/test/SConscript', ['core', 'corefiles'])

	# Build test cases
	tests = os.listdir('src/tests')
	
	for testdir in tests:
		if os.path.isdir(os.path.join('src/tests', testdir)) and not moddir in exclude:
			if os.path.isfile(os.path.join('src/tests', testdir, 'SConscript')):
				# Build test
				test = modtpl.Clone()
				if modtpl['PLATFORM'] == 'win32':
					test.Append(LIBPATH = ['#bin/test', '#bin/test/modules'])				
				SConscript(os.path.join('bin/test/tests', testdir, 'SConscript'), 'test')