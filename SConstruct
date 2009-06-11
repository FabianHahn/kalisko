import os

# Config section
ccflags = ['-std=gnu99', '-Wall', '-pipe']

# Start of actual makefile
core = Environment(CCFLAGS = ccflags, LINKFLAGS = ['-Wl,--export-dynamic'], ENV = os.environ, CPPPATH = ['.'])
modtpl = Environment(CCFLAGS = ccflags, ENV = os.environ, CPPPATH = ['.','#src'])

# Add glib support
core.ParseConfig('pkg-config --cflags --libs glib-2.0')
modtpl.ParseConfig('pkg-config --cflags --libs glib-2.0')

buildtests = True

# Build modes
if ARGUMENTS.get('debug') == '0' or ARGUMENTS.get('release') == '1':
	prefix = 'bin/release'
	core.Append(CCFLAGS = ['-O2'])
	core.VariantDir(prefix, 'src', 0)
	modtpl.Append(CCFLAGS = ['-O2'])
	modtpl.VariantDir(prefix, 'src', 0)
	if modtpl['PLATFORM'] == 'win32':
		modtpl.Append(LIBPATH = ['#bin/release', '#bin/release/modules'])
	buildtests = False
else:
	prefix = 'bin/debug'
	core.Append(CCFLAGS = ['-g'])
	core.VariantDir(prefix, 'src', 0)
	modtpl.Append(CCFLAGS = ['-g'])
	testtpl = modtpl.Clone()
	testtpl.VariantDir('bin/test', 'src', 0)
	testtpl.Append(CPPDEFINES = ['NO_DLL_IMPORT'])
	modtpl.VariantDir(prefix, 'src', 0)
	if modtpl['PLATFORM'] == 'win32':
		modtpl.Append(LIBPATH = ['#bin/debug', '#bin/debug/modules'])	
	
if ARGUMENTS.get('verbose') != '1':
	core.Replace(CCCOMSTR = 'Compiling object: $TARGET')
	core.Replace(LINKCOMSTR = 'Linking executable: $TARGET')
	modtpl.Replace(SHCCCOMSTR = 'Compiling shared object: $TARGET')
	modtpl.Replace(SHLINKCOMSTR = 'Linking library: $TARGET')
	if buildtests:
		testtpl.Replace(CCCOMSTR = 'Compiling object: $TARGET')
		testtpl.Replace(LINKCOMSTR = 'Linking executable: $TARGET')

# OS-specific global options
if core['PLATFORM'] == 'posix':
	if buildtests:
		testtpl.Append(LIBS = ['dl'])
elif core['PLATFORM'] == 'win32':
	core.Append(LINKFLAGS = ['-Wl,--out-implib,' + prefix + '/libkalisko.a'])	

# Build core
SConscript(os.path.join(prefix, 'SConscript'), 'core')

# Build modules
modules = os.listdir('src/modules')

for moddir in modules:
	if os.path.isdir(os.path.join('src/modules', moddir)):
		if os.path.isfile(os.path.join('src/modules', moddir, 'SConscript')):
			# Build module
			module = modtpl.Clone()
			SConscript(os.path.join(prefix, 'modules', moddir, 'SConscript'), 'module')

# Build tests
if buildtests:
	corefiles = [x for x in testtpl.Glob('bin/test/*.c') if not x.name == 'kalisko.c']

	tests = os.listdir('src/tests')
	
	for testdir in tests:
		if os.path.isdir(os.path.join('src/tests', testdir)):
			if os.path.isfile(os.path.join('src/tests', testdir, 'SConscript')):
				# Build test
				test = testtpl.Clone()
				SConscript(os.path.join('bin/test/tests', testdir, 'SConscript'), ['test','corefiles'])