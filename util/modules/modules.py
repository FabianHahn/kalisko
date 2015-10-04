"""
Copyright (c) 2008, Kalisko Project Leaders
Copyright (c) 2015, Google Inc.
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

class ModuleAnalyzer(object):
	"""An object capable of analyzing a module source directory."""

	"""Used to find lines where dependencies are declared."""
	_MODULE_DEPENDS_LINE_PREFIX = 'MODULE_DEPENDS'

	"""Used to extract the actual dependencies."""
	_MODULE_DEPENDENCY_REGEX = r'MODULE_DEPENDENCY\("(\S+)"'

	def __init__(self, module_root):
		"""Creates an analyzer instance operating on the provided directory."""
		self.module_root = module_root

	def ExpandRuntimeDeps(self, modules):
		"""Returns a new set containing all the transitive runtime dependency
		   modules of the provided modules (including the modules themselves)."""
		result = set()
		for module in modules:
			result.update(self._TransitiveDeps(module))
		return result

	def All(self):
		"""Returns the names of all modules."""
		return os.listdir(self.module_root)

	def Exists(self, module_name):
		"""Returns whether or not a module of the given name exists."""
		return os.path.isdir(os.path.join(self.module_root, module_name))

	def _CheckExists(self, module_name):
		"""Utility method which throws is the provided module doesn't exist."""
		if not self.Exists(module_name):
			raise ValueError('Could not find module ' + module_name + ' under ' + self.module_root)

	def _TransitiveDeps(self, module_name):
		"""Returns the set of modules the provided module depends on transitively."""
		result = set([module_name])
		for dep in self._Deps(module_name):
			result.update(self._TransitiveDeps(dep))
		return result

	def _Deps(self, module_name):
		"""Returns the set of modules the provided module depends on directly."""
		self._CheckExists(module_name)

		# Extract all the lines where dependencies are declared.
		depends_lines = []
		with open(self._MainModuleSourceFile(module_name)) as f:
			for line in f:
				if line.startswith(self._MODULE_DEPENDS_LINE_PREFIX):
					depends_lines.append(line)

		# Extract all the dependencies from each line.
		result = set()
		for line in depends_lines:
			matches = [m.group(1) for m in re.finditer(self._MODULE_DEPENDENCY_REGEX, line)]
			result.update(set(matches))
		return result

	def _MainModuleSourceFile(self, module_name):
		"""Returns the path to the source file where the module is declared."""
		self._CheckExists(module_name)

		line = 'MODULE_NAME("' + module_name + '");\n'

		module_dir = os.path.join(self.module_root, module_name)
		for root, dirs, files in os.walk(module_dir):
			for name in files:
				path = os.path.join(root, name)
				with open(path) as f:
					if line in f.readlines():
						return path
		raise ValueError('Unable to find main module source file for module: ', module_name)
