#!/usr/bin/env python3

usage = \
'''Usage: %(me)s [--dry-run] [--verbose] [--dont-update]
  or:  %(me)s restore
  or:  %(me)s update
In the 1st form, run standard pre-commit procedure for Progressia.
In the 2nd form, attempt to restore workspace if the pre-commit hook failed.
In the 3rd form, only update git pre-commit hook

  --dry-run      do not change anything in git or in the working tree
  --verbose      print commands and diagnostics
  --dont-update  do not update git pre-commit hook
  --help         display this help and exit

Currently, the pre-commit procedure performs the following:
  1. format staged changes
  2. attempt to compile with staged changes only'''

import sys
import os
import subprocess

def fail(*args):
    print(my_name + ':', *args, file=sys.stderr)
    sys.exit(1)

def invoke(*cmd, result_when_dry=None, quiet=True):

    if verbose:
        print(my_name + ': command "' + '" "'.join(cmd) + '"')

    if dry_run and result_when_dry != None:
        print(my_name + ':   skipped: --dry-run')
        return result_when_dry

    output = ''
    popen = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE,
                             text=True,
                             universal_newlines=True)

    for line in popen.stdout:
        if (not quiet):
            print(line, end='')
        output += line

    popen.stdout.close()

    return_code = popen.wait()
    if return_code != 0:
        raise subprocess.CalledProcessError(return_code, cmd)

    return output

STASH_NAME = 'progressia_pre_commit_stash'

def run_safety_checks():
    if invoke('git', 'stash', 'list', '--grep', f"^{STASH_NAME}$") != '':
        fail(f"Cannot run pre-commit checks: stash {STASH_NAME} exists. " +
             f"Use `{my_name} restore` to restore workspace and repository " +
             f"state")

    # Let's hope there are no files with weird names
    indexed_changes = \
        set(invoke('git', 'diff', '--name-only', '--cached') \
            .strip().split('\n'))
    unindexed_changes = \
        set(invoke('git', 'diff', '--name-only') \
            .strip().split('\n'))

    both_changes = indexed_changes & unindexed_changes

    if len(both_changes) != 0:
        fail(f"Cannot run pre-commit checks: files with indexed and " +
             "unindexed changes exist:\n\n\t" +
             "\n\t".join(both_changes) +
             "\n")


if __name__ == '__main__':
    my_name = os.path.basename(sys.argv[0])
    verbose = False
    dry_run = False



    verbose = True
    dry_run = True
    run_safety_checks()
    #update()

    unindexed_changes = invoke('git', 'diff', '--name-status')
    if unindexed_changes != '':
        print('Unindexed changes found in files:')
        print(unindexed_changes)
        print('These changes will be ignored')

        invoke('git', 'stash', 'push',
               '--keep-index',
               '--include-untracked',
               '--message', STASH_NAME,
               result_when_dry='')

    # check that ORIGINAL does not exist
    # check that staged files & files with unstaged changes = 0
    # update pre-commit hook
    # if any unstaged changes:
    #   stash ORIGINAL
    #   remove unstaged changes
    # format staged files
    # compile
    # git add
    # if any unstaged changes:
    #   unstash ORIGINAL

