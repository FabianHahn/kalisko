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

FOO_CC_CONTENTS = """
MODULE_NAME("foo");
MODULE_DEPENDS(MODULE_DEPENDENCY("wiggle", 0, 7, 0), MODULE_DEPENDENCY("nabble", 0, 1, 2));
"""

BAR_CC_CONTENTS = """
MODULE_NAME("bar");
"""

class ModuleAnalyzerTest(unittest.TestCase):
	def setUp(self):
		self.test_dir = tempfile.mkdtemp()

		# Make the analyzer believe that socket and event exist.
		self._CreateFakeModule('wiggle')
		self._CreateFakeModule('nabble')
		self._CreateFakeModule('sibble')

		# Create a foo module with deps.
		os.mkdir(os.path.join(self.test_dir, 'foo'))
		with open(os.path.join(self.test_dir, 'foo', 'foo.cc'), 'w') as f:
			f.write(FOO_CC_CONTENTS)

		# Create a baz module without deps.
		os.mkdir(os.path.join(self.test_dir, 'bar'))
		with open(os.path.join(self.test_dir, 'bar', 'bar.cc'), 'w') as f:
			f.write(BAR_CC_CONTENTS)

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

	def _CreateFakeModule(self, module_name):
		os.mkdir(os.path.join(self.test_dir, module_name))
		with open(os.path.join(self.test_dir, module_name, '%s.cc' % module_name), 'w') as f:
			f.write('MODULE_NAME("%s");\n' % module_name)
