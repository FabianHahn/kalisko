import os

# Config section
ccflags = ['-std=gnu99', '-Wall', '-pipe']
modsdir = 'modules'

# Start of actual makefile
core = Environment(CCFLAGS = ccflags, ENV = os.environ, CPPPATH = ['.'])
modtpl = Environment(CCFLAGS = ccflags, ENV = os.environ, CPPPATH = ['.'])

# Build modes
if ARGUMENTS.get('debug') == '0':
	core.Append(CCFLAGS = ['-O2'])
	modtpl.Append(CCFLAGS = ['-O2'])
else:
	core.Append(CCFLAGS = ['-g'])
	modtpl.Append(CCFLAGS = ['-g'])

if ARGUMENTS.get('verbose') != '1':
	core.Replace(CCCOMSTR = 'Compiling object: $TARGET')
	core.Replace(LINKCOMSTR = 'Linking executable: $TARGET')
	modtpl.Replace(SHCCCOMSTR = 'Compiling shared object: $TARGET')
	modtpl.Replace(SHLINKCOMSTR = 'Linking library: $TARGET')

# Add glib support to core
core.ParseConfig('pkg-config --cflags --libs glib-2.0')	

# Core configure checks
if not core.GetOption('clean'):
	conf = Configure(core)
	
	for depmod in conf.env['LIBS']:
		if not conf.CheckLib(depmod):
			print('Error: Could not find library ' + depmod + '!')
			Exit(1)

	if conf.env['PLATFORM'] == 'posix':
		conf.env.Append(LIBS = ['dl'])

		if not conf.CheckLibWithHeader('dl', 'dlfcn.h', 'c'):
			print('Error: Could not find libdl and the corresponding dlfcn.h header file!')
			Exit(1)
			
	elif conf.env['PLATFORM'] == 'win32':
		conf.env.Append(LIBS = ['ws2_32'])
		
		if not conf.CheckCHeader('windows.h'):
			print('Error: Could not find windows.h header file!')
			Exit(1)	
			
		if not conf.CheckLibWithHeader('ws2_32', 'winsock2.h', 'c'):
			print('Error: Could not find ws2_32 and the corresponding winsock.h header file!')
			Exit(1)
			
		if not conf.CheckCHeader('Ws2tcpip.h'):
			print('Error: Could not find Ws2tcpip.h header file!')
			Exit(1)		

	core = conf.Finish()

# Build core
core.Program('kalisko', Glob('*.c'))

# Build modules
modules = os.listdir(modsdir)

for moddir in modules:
	if os.path.isdir(os.path.join(modsdir, moddir)):
		if os.path.isfile(os.path.join(modsdir, moddir, 'SConscript')):
			# Build module
			module = modtpl.Clone()
			SConscript(os.path.join(modsdir, moddir, 'SConscript'), 'module')

