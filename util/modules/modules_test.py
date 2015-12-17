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
import shutil
import tempfile
import unittest

from .modules import ModuleAnalyzer

class ModuleAnalyzerTest(unittest.TestCase):
	def setUp(self):
		self.test_dir = tempfile.mkdtemp()

		self._CreateFakeModule('wiggle')
		self._CreateFakeModule('nabble')
		self._CreateFakeModule('sibble')

		self._CreateFakeModule('foo', ['wiggle', 'nabble'])
		self._CreateFakeModule('bar')

		self.analyzer = ModuleAnalyzer(self.test_dir)

	def tearDown(self):
		shutil.rmtree(self.test_dir)

	def testAll(self):
		self.assertItemsEqual(['foo', 'bar', 'wiggle', 'nabble', 'sibble'], self.analyzer.All())

	def testExpandRuntimeDeps(self):
		self.assertItemsEqual(['foo', 'wiggle', 'nabble'], self.analyzer.ExpandRuntimeDeps(['foo']))
		self.assertItemsEqual(['bar'], self.analyzer.ExpandRuntimeDeps(['bar']))
		self.assertItemsEqual([], self.analyzer.ExpandRuntimeDeps([]))

	def testExists(self):
		self.assertTrue(self.analyzer.Exists('foo'))
		self.assertFalse(self.analyzer.Exists('asdf'))

	def _CreateFakeModule(self, module_name, module_deps = []):
		os.mkdir(os.path.join(self.test_dir, module_name))
		with open(os.path.join(self.test_dir, module_name, '%s.cc' % module_name), 'w') as f:
			f.write('MODULE_NAME("%s");\n' % module_name)
			if len(module_deps) > 0:
				dep_declarations = map(lambda d: 'MODULE_DEPENDENCY("%s", 0, 7, 0)' % d, module_deps)
				f.write('MODULE_DEPENDS(%s)' % ','.join(dep_declarations))

