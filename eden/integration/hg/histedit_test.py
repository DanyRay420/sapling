#!/usr/bin/env python3
#
# Copyright (c) 2016-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import os
from textwrap import dedent

from .lib.hg_extension_test_base import EdenHgTestCase, hg_test
from .lib.histedit_command import HisteditCommand
from ..lib import hgrepo


@hg_test
class HisteditTest(EdenHgTestCase):
    def populate_backing_repo(self, repo):
        repo.write_file('first', '')
        self._commit1 = repo.commit('first commit')

        repo.write_file('second', '')
        self._commit2 = repo.commit('second commit')

        repo.write_file('third', '')
        self._commit3 = repo.commit('third commit')

    def test_stop_at_earlier_commit_in_the_stack_without_reordering(self):
        commits = self.repo.log()
        self.assertEqual([self._commit1, self._commit2, self._commit3], commits)

        # histedit, stopping in the middle of the stack.
        histedit = HisteditCommand()
        histedit.pick(self._commit1)
        histedit.stop(self._commit2)
        histedit.pick(self._commit3)

        # We expect histedit to terminate with a nonzero exit code in this case.
        with self.assertRaises(hgrepo.HgError) as context:
            histedit.run(self)
        head = self.repo.log(revset='.')[0]
        expected_msg = (
            'Changes committed as %s. '
            'You may amend the changeset now.' % head[:12]
        )
        self.assertIn(expected_msg, str(context.exception))

        # Verify the new commit stack and the histedit termination state.
        # Note that the hash of commit[0] is unpredictable because Hg gives it a
        # new hash in anticipation of the user amending it.
        parent = self.repo.log(revset='.^')[0]
        self.assertEqual(self._commit1, parent)
        self.assertEqual(
            ['first commit', 'second commit'], self.repo.log('{desc}')
        )

        # Make sure the working copy is in the expected state.
        self.assert_status_empty(op='histedit')
        self.assertSetEqual(
            set(['.eden', '.hg', 'first', 'second']),
            set(os.listdir(self.repo.get_canonical_root()))
        )

        self.hg('histedit', '--continue')
        self.assertEqual(
            ['first commit', 'second commit', 'third commit'],
            self.repo.log('{desc}')
        )
        self.assert_status_empty()
        self.assertSetEqual(
            set(['.eden', '.hg', 'first', 'second', 'third']),
            set(os.listdir(self.repo.get_canonical_root()))
        )

    def test_reordering_commits_without_merge_conflicts(self):
        self.assertEqual(
            ['first commit', 'second commit', 'third commit'],
            self.repo.log('{desc}')
        )

        # histedit, reordering the stack in a conflict-free way.
        histedit = HisteditCommand()
        histedit.pick(self._commit2)
        histedit.pick(self._commit3)
        histedit.pick(self._commit1)
        histedit.run(self)

        self.assertEqual(
            ['second commit', 'third commit', 'first commit'],
            self.repo.log('{desc}')
        )
        self.assert_status_empty()
        self.assertSetEqual(
            set(['.eden', '.hg', 'first', 'second', 'third']),
            set(os.listdir(self.repo.get_canonical_root()))
        )

    def test_drop_commit_without_merge_conflicts(self):
        self.assertEqual(
            ['first commit', 'second commit', 'third commit'],
            self.repo.log('{desc}')
        )

        # histedit, reordering the stack in a conflict-free way.
        histedit = HisteditCommand()
        histedit.pick(self._commit1)
        histedit.drop(self._commit2)
        histedit.pick(self._commit3)
        histedit.run(self)

        self.assertEqual(
            ['first commit', 'third commit'], self.repo.log('{desc}')
        )
        self.assert_status_empty()
        self.assertSetEqual(
            set(['.eden', '.hg', 'first', 'third']),
            set(os.listdir(self.repo.get_canonical_root()))
        )

    def test_roll_two_commits_into_parent(self):
        self.assertEqual(
            ['first commit', 'second commit', 'third commit'],
            self.repo.log('{desc}')
        )

        # histedit, reordering the stack in a conflict-free way.
        histedit = HisteditCommand()
        histedit.pick(self._commit1)
        histedit.roll(self._commit2)
        histedit.roll(self._commit3)
        histedit.run(self)

        self.assertEqual(['first commit'], self.repo.log('{desc}'))
        self.assert_status_empty()
        self.assertSetEqual(
            set(['.eden', '.hg', 'first', 'second', 'third']),
            set(os.listdir(self.repo.get_canonical_root()))
        )

    def test_abort_after_merge_conflict(self):
        self.write_file('will_have_confict.txt', 'original\n')
        self.hg('add', 'will_have_confict.txt')
        commit4 = self.repo.commit('commit4')
        self.write_file('will_have_confict.txt', '1\n')
        commit5 = self.repo.commit('commit5')
        self.write_file('will_have_confict.txt', '2\n')
        commit6 = self.repo.commit('commit6')

        histedit = HisteditCommand()
        histedit.pick(commit4)
        histedit.pick(commit6)
        histedit.pick(commit5)
        original_commits = self.repo.log()

        with self.assertRaises(hgrepo.HgError) as context:
            histedit.run(self, ancestor=commit4)
        expected_msg = (
            ('Fix up the change (pick %s)\n' % commit6[:12]) +
            '  (hg histedit --continue to resume)'
        )
        self.assertIn(expected_msg, str(context.exception))
        self.assert_status({
            'will_have_confict.txt': 'M',
        }, op='histedit')
        self.assert_file_regex('will_have_confict.txt', '''\
            <<<<<<< local: .*
            original
            =======
            2
            >>>>>>> histedit: .*
            ''')

        self.hg('histedit', '--abort')
        self.assertEqual('2\n', self.read_file('will_have_confict.txt'))
        self.assertListEqual(
            original_commits,
            self.repo.log(),
            msg='The original commit hashes should be restored by the abort.'
        )
        self.assert_status_empty()
