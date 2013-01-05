"""
Copyright (c) 2008, Kalisko Project Leaders
Copyright (c) 2012, Google Inc.
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

import glob
import os
import subprocess

def buildInterfaces(kic_binary_path, interfaces_root_path):
	elf_file = os.path.join(kic_binary_path, 'kic')
	exe_file = os.path.join(kic_binary_path, 'kic.exe')
	if not (os.path.exists(elf_file) or os.path.exists(exe_file)):
		print 'kic not found, building...'
		old_path = os.getcwd()
		os.chdir(kic_binary_path)
		os.system('scons')
		os.chdir(old_path)

	os.chdir(interfaces_root_path)
	interfaces = glob.glob('*.i') + glob.glob('modules/*/*.i')

	for interface in interfaces:
		header = interface[:-1] + 'h'
		imtime = os.stat(interface).st_mtime

		if os.path.exists(header):
			hmtime = os.stat(header).st_mtime
		else:
			hmtime = 0

		if imtime > hmtime:
			if subprocess.call([os.path.join(kic_binary_path, 'kic'), interface]) == 0:
				print 'Compiled ' + interface
			else:
				break

	os.chdir('..')
