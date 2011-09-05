import glob
import os
import subprocess

os.chdir('src')
interfaces = glob.glob('*.i') + glob.glob('modules/*/*.i')

for interface in interfaces:
	if subprocess.call(['../kic/kic', interface]) == 0:
		print 'Compiled ' + interface
	else:
		break


