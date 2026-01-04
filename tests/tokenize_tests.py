#!/usr/bin/env python3

from unittest import TestCase

import unittest

import os.path
import sys
import subprocess
import random
import re

from shell_test_helpers import *

TOKENIZE = "./tokenize"
SHELL = "./shell"

class ShellTests(ShellTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(SHELL, *args, **kwargs)

    def test01(self):
        """Recognizes a simple non-special token"""
        self.assertEqual(sh('echo a | ./tokenize'), 'a')

    def test02(self):
        """Recognizes two non-special tokens"""
        self.assertEqual(sh("echo 'a b' | ./tokenize"), "a\nb")
    
    def test03(self):
        """Recognizes three non-special multi-char tokens"""
        self.assertEqual(
                sh("echo 'foo_bar baz      hello' | ./tokenize"), 
                "foo_bar\nbaz\nhello")
  
    def test04(self):
        """Recognizes special characters as tokens"""
        self.assertEqual(sh("echo '(;|)<>' | ./tokenize"), "(\n;\n|\n)\n<\n>")

    def test05(self):
        """Recognizes a string"""
        self.assertEqual(sh("echo '\"hello world\"' | ./tokenize"), "hello world")

    def test06(self):
        """Recognizes strings mixed with other tokens"""
        self.assertEqual(
                sh("echo 'foo \"Lorem ipsum dolor sit amet\" < bar \"consectetur (adipiscing; >elit\"' | ./tokenize"), 
                "foo\nLorem ipsum dolor sit amet\n<\nbar\nconsectetur (adipiscing; >elit")

    # Custom Unit Testing
    def test07(self):
        """Recognizes special symbols with tokens in a shell command"""
        self.assertEqual(
            sh("echo 'sort < names | head' | ./tokenize"),
            "sort\n<\nnames\n|\nhead")

    def test08(self):
        """Recongnizes file path with a mix of symbols and characters"""
        self.assertEqual(
            sh("echo 'ls -la /home/bob/secret_folder/secrets3 > output.txt' | ./tokenize"),
            "ls\n-la\n/home/bob/secret_folder/secrets3\n>\noutput.txt"
        )

    def test09(self):
        """Recognizes dash arguments"""
        self.assertEqual(
            sh("echo 'whoami --help' | ./tokenize"),
            "whoami\n--help"
        )

if __name__ == '__main__':
    print(f"-= {YELLOW}Running tests for {TOKENIZE}{RESET} =-")
    unittest.main(testRunner = unittest.TextTestRunner(resultclass = PrettierTextTestResult))


