#!/usr/bin/env python3
#
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import logging
import os

from .lib.hg_extension_test_base import EdenHgTestCase, hg_test


@hg_test
class UndoTest(EdenHgTestCase):
    def populate_backing_repo(self, repo):
        repo.write_file('src/common/foo/test.txt', 'testing\n')
        self.commit1 = repo.commit('Initial commit.')

    def edenfs_logging_settings(self):
        edenfs_log_levels = {}

        log = logging.getLogger('eden.test.undo')
        if log.getEffectiveLevel() >= logging.DEBUG:
            edenfs_log_levels['eden.fs.inodes.TreeInode'] = 'DBG5'

        return edenfs_log_levels

    def test_undo_commit_with_new_dir(self):
        log = logging.getLogger('eden.test.undo')

        # Add a new file in a new directory
        log.debug('=== commit 1: %s', self.commit1)
        base_dir = 'src/common/foo'
        new_dir = 'src/common/foo/newdir'
        new_file = 'src/common/foo/newdir/code.c'
        self.mkdir(new_dir)
        self.write_file(new_file, 'echo hello world\n')
        # Add the file and create a new commit
        log.debug('=== hg add')
        self.hg('add', new_file)
        log.debug('=== hg commit')
        commit2 = self.repo.commit('Added newdir\n')
        log.debug('=== commit 2: %s', commit2)
        self.assert_status_empty()
        self.assertNotEqual(self.repo.get_head_hash(), self.commit1)

        # Use 'hg undo' to revert the commit
        log.debug('=== hg undo')
        self.hg('undo')
        log.debug('=== undo done')
        self.assert_status_empty()
        log.debug('=== new head: %s', self.repo.get_head_hash())
        self.assertEqual(self.repo.get_head_hash(), self.commit1)

        # listdir() should only return test.txt now, and not newdir
        dir_entries = os.listdir(self.get_path(base_dir))
        self.assertEqual(dir_entries, ['test.txt'])

        # stat() calls should fail with ENOENT for newdir
        # This exercises a regression we have had in the past where we did not
        # flush the kernel inode cache entries properly, causing listdir()
        # to report the contents correctly but stat() to report that the
        # directory still existed.
        self.assertFalse(os.path.exists(self.get_path(new_dir)))
