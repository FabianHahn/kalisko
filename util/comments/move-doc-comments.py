"""
Copyright (c) 2012, Kalisko Project Leaders
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

# A utility script which can be used to move doc comments of functions from .c-files to .i-files.

import argparse
import os
import re
import sys


FUNCTION_REGEXP = re.compile(r'^API[ ]+(\w+)(\**)[ ]+(\**)(\w+)\(.*')


class Function(object):
	@classmethod
	def ForLine(cls, raw_line, comment=None):
		"""Parses the provided line into a FunctionSignature instance. Returns the new instance if successful and None otherwise."""
		match = re.search(FUNCTION_REGEXP, raw_line)
		if match:
			return_type = match.group(1)
			left_stars = match.group(2)
			right_stars = match.group(3)
			name = match.group(4)
			return Function(raw_line, return_type + left_stars + right_stars, name, comment)
		return None

	def __init__(self, raw_line, return_type, name, comment=None):
		self.raw_line = raw_line
		self.return_type = return_type
		self.name = name
		self.comment = comment

	def __eq__(self, other):
		"""Note that for this to return true, the raw_line attributes needn't match."""
		return self.return_type == other.return_type and self.name == other.name

	def __str__(self):
		return '[name: ' + self.name + ', returns: ' + self.return_type + ']'


def ParseFile(path):
	"""Reads the content of the passed file and extracts all function signatures and comments. Returns a list of Function objects."""
	result = []
	comment = None

	for line in open(path, 'r'):
		stripped_line = line.strip()
		if not comment and stripped_line.startswith('/**'):
			comment = line
			continue
		elif comment and stripped_line.startswith('*'):
			comment += line
			continue

		func = Function.ForLine(line, comment)
		if func:
			result.append(func)
		comment = None

	return result


def WriteFile(path, content):
	"""Writes the string content to the specified file. If the file already exists, the contents are replaced."""
	print 'Writing file:', path
	f = open(path, 'w')
	f.write(content)
	f.close()


def ProcessFilePair(cfile, ifile, write_to_disk):
	"""For every function whose signature appears in both cfile and ifile, moves the doc comment from cfile to ifile. Only actually commits to disk if write_to_disk is True."""
	print 'Processing file pair: (', cfile, ',', ifile, ')'

	# Get functions from both files, including comments if any
	cfunctions = ParseFile(cfile)
	ifunctions = ParseFile(ifile)

	# Used to remember the functions whose comments have been moved to the i-file
	moved_functions = []

	# Go through the ifile and add comments which are present in the cfile
	new_ifile = ''
	for line in open(ifile, 'r'):
		# Get the function for the current line, note that it will not contain any comments
		func = Function.ForLine(line)
		if not func:
			new_ifile = new_ifile + line
			continue

		# Get the corresponding functions with comment information
		assert func in ifunctions
		ifunc = ifunctions[ifunctions.index(func)]
		if func in cfunctions:
			cfunc = cfunctions[cfunctions.index(func)]
			if cfunc.comment and not ifunc.comment:
				# Move the comment from cfile to ifile
				print 'Moving doc comment for function:', func
				moved_functions.append(func)
				new_ifile = new_ifile + '\n' + cfunc.comment

		# Dump the current line in any case
		new_ifile = new_ifile + line

	# Go through the cfile and remove the comments which have been added to the ifile
	new_cfile = ''
	comment = None
	for line in open(cfile, 'r'):
		stripped_line = line.strip()
		if not comment and stripped_line.startswith('/**'):
			comment = line
			continue
		elif comment and stripped_line.startswith('*'):
			comment += line
			continue

		func = Function.ForLine(line)
		if comment and not (func and func in moved_functions):
			# This is not a function line or it has not been moved, re-add the comment
			new_cfile = new_cfile + comment

		# Definitely re-add the current line
		new_cfile = new_cfile + line
		comment = None

	# Dump the contents to disk
	if write_to_disk:
		WriteFile(cfile, new_cfile)
		WriteFile(ifile, new_ifile)


def FindFilePairs(directory, recursive):
	"""Returns a list of tuples of the form (file.c, file.i). If recursive is True, also searches subdirectories."""
	result = []
	for root, dirs, files in os.walk(directory):
		for f in files:
			name, extension = os.path.splitext(os.path.join(root, f))
			if extension == '.c' and os.path.isfile(name + '.i'):
				result.append((name + '.c', name + '.i'))
		if recursive == False:
			break
	return result


def main():
	parser = argparse.ArgumentParser(description='Moves function doc comments from c-files to the corresponding i-files')
	parser.add_argument('directory', help='The root directory in which to search for matching c-files and i-files')
	parser.add_argument('-n', '--nowrite', action='store_true', help='Execute a dry-run of the script without writing to disk')
	parser.add_argument('-r', '--recursive', action='store_true', help='Inspect subdirectories as well')
	args = parser.parse_args()

	print ''
	if args.nowrite:
		print '******************************************************'
		print 'Executing a dry-run. Disk contents will not be changed'
		print '******************************************************'
		print ''

	for cfile, ifile in FindFilePairs(args.directory, args.recursive):
		ProcessFilePair(cfile, ifile, not args.nowrite)
		print ''


if __name__ == '__main__':
	main()
