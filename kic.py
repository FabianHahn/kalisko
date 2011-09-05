import glob
import os
import subprocess

def buildInterfaces():
	if not (os.path.exists('kic/kic') or os.path.exists('kic/kic.exe')):
		print 'kic not found, building...'
		os.chdir('kic')
		os.system('scons')
		os.chdir('..')

	os.chdir('src')
	interfaces = glob.glob('*.i') + glob.glob('modules/*/*.i')
	
	for interface in interfaces:
		header = interface[:-1] + 'h'
		imtime = os.stat(interface).st_mtime
		
		if os.path.exists(header):
			hmtime = os.stat(header).st_mtime
		else:
			hmtime = 0
	
		if imtime > hmtime:
			if subprocess.call(['../kic/kic', interface]) == 0:
				print 'Compiled ' + interface
			else:
				break
	
	os.chdir('..')
