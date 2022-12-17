#!/usr/bin/env python3

usage = \
'''Usage: %(me)s run [OPTIONS...]
  or:  %(me)s restore [OPTIONS...]
  or:  %(me)s set-build-root CMAKE_BINARY_DIR
In the 1st form, run standard pre-commit procedure for Progressia.
In the 2nd form, attempt to restore workspace if the pre-commit hook failed.
In the 3rd form, only update git pre-commit hook.

  --dry-run      do not change anything in git or in the filesystem;
                   implies --verbose
  --verbose      print commands and diagnostics
  --help         display this help and exit
  --version      display version information and exit

Currently, the pre-commit procedure performs the following:
  1. format staged changes
  2. attempt to compile with staged changes only

pre-commit-settings.json values:
  build-root         CMake binary dir to use (filled in by CMake)
  parallelism        threads to use, default is 1
  git                git command, default is null
  cmake              cmake command, default is null
  clang-format-diff  clang-format-diff command, default is null

Use semicolons to separate arguments in git, cmake and clang-format-diff'''

# Script version. Increment when script logic changes significantly.
# Commit change separately.
version = 1

# Source directories to format
src_dirs = ['desktop', 'main']

# File extensions to format
exts = ['cpp', 'h', 'inl']


import sys
import os
import subprocess
import shutil
import json


STASH_NAME = 'progressia_pre_commit_stash'
# Paths are relative to this script's directory, tools/
SETTINGS_PATH = 'pre-commit-settings.json'
CLANG_FORMAT_PATH = 'clang-format/clang-format.json'
CLANG_TIDY_CHECK_MARKER = 'Clang-tidy is enabled ' \
                          '(this is a marker for pre-commit.py)'


def fail(*args, code=1):
    """Print an error message and exit with given code (default 1)"""
    print(my_name + ':', *args, file=sys.stderr)
    sys.exit(1)


def verbose(*args):
    """Print a message in verbose mode only."""
    if verbose_mode:
        print(my_name + ':', *args)


def long_print_iter(title, it):
    """Print contents of iterable titled as specified. If iterable is empty,
    print the string (nothing) instead.
    """
    print(title + ':')
    
    if len(it) > 0:
        print('\t' + '\n\t'.join(it) + '\n')
    else:
        print('\t(nothing)\n')


def invoke(*cmd, result_when_dry=None, quiet=True, text=True, stdin=None):
    """Execute given system command and return its stdout. If command fails,
    throw CalledProcessError.
    
    When in verbose mode, log command before execution. If in dry-run mode and
    result_when_dry is not None, skip execution and return result_when_dry
    instead.
    
    Keyword arguments:
    result_when_dry -- unless None (default), skip execution and return this
    quiet -- if False, print stdout (default True)
    text -- treat stdin and stdout as text rather than bytes (default False)
    stdin -- unless None (default), send this to stdin of spawned process
    """
    verbose('command', *(repr(c) for c in cmd))

    if dry_run and result_when_dry is not None:
        print(my_name + ':   skipped: --dry-run')
        return result_when_dry
    
    popen = subprocess.Popen(cmd,
        stdout=subprocess.PIPE,
        text=text,
        universal_newlines=text,
        stdin=subprocess.PIPE if stdin else subprocess.DEVNULL)
    
    stdout, _ = popen.communicate(input=stdin)
    
    if text and not quiet:
        print(stdout, end='')

    return_code = popen.wait()
    if return_code != 0:
        raise subprocess.CalledProcessError(return_code, cmd)

    return stdout


def get_file_sets():
    """Return sets of indexed and unindexed files according to Git"""
    def git_z(*cmd):
        raw = invoke(*git, *cmd, '-z', text=False)
        return set(f.decode() for f in raw.split(b'\x00') if len(f) != 0)
    
    indexed   = git_z('diff', '--name-only', '--cached')
    unindexed = git_z('diff', '--name-only') | \
                git_z('ls-files', '--other', '--exclude-standard')
    
    return indexed, unindexed


def run_safety_checks(indexed, unindexed):
    if invoke(*git, 'stash', 'list', '--grep', f"\\b{STASH_NAME}$") != '':
        fail(f"Cannot run pre-commit checks: stash {STASH_NAME} exists. "
             f"Use `{my_name} restore` to restore workspace and repository "
             f"state")

    both_changes = indexed & unindexed

    if len(both_changes) != 0:
        fail(f"Cannot run pre-commit checks: files with indexed and unindexed "
             'changes exist:\n\n\t' +
             '\n\t'.join(both_changes) +
             '\n')


def do_restore():
    """Restore repository and filesystem. Fail if stash not found."""
    print('Redoing rolled back changes')
    
    git_list = invoke(*git, 'stash', 'list', '--grep', f"\\b{STASH_NAME}$")
    
    if len(git_list) == 0:
        if dry_run:
            stash_name = 'stash@{0}'
        else:
            fail(f"Cannot restore repository: stash {STASH_NAME} not found")
    else:
        stash_name, _, _ = git_list.partition(':')
    
    invoke(*git, 'stash', 'pop', '--index', '--quiet', stash_name,
           result_when_dry='', quiet=False)


def format_project():
    """Format staged files with clang-format-diff."""
    print('Formatting code')
    format_file = os.path.join(os.path.dirname(__file__),
                               CLANG_FORMAT_PATH)
    
    with open(format_file, encoding='utf-8') as f:
        style = f.read()
    
    diff = invoke(*git, 'diff', '-U0', '--no-color', '--relative', 'HEAD',
                  *(f"{d}/*.{e}" for d in src_dirs for e in exts))
    
    invoke(*clang_format_diff, '-p1', '-i', '--verbose', '-style=' + style,
           stdin=diff, result_when_dry='', quiet=False)


def unformat_project(indexed):
    """Undo formatting changes introduced by format_project()."""
    print('Undoing formatting changes')
    invoke(*git, 'restore', '--', *indexed)


def build_project():
    """Build project with cmake."""
    print('Building project')
    build_log = invoke(*cmake, '--build', build_root, '--parallel', parallelism,
           result_when_dry=CLANG_TIDY_CHECK_MARKER,
           quiet=False)
    
    if CLANG_TIDY_CHECK_MARKER not in build_log.splitlines():
        fail('Project build was successful, but clang-tidy did not run. '
             'Please make sure DEV_MODE is ON and regenerate CMake cache.')
    
    print('Success')
    

def pre_commit():
    """Run pre-commit checks."""
    
    if build_root is None:
        fail(f"build-root is not set in {SETTINGS_PATH}. Compile project "
             'manually to set this variable properly.')
    if not os.path.exists(build_root):
        fail(f"build-root {build_root} does not exist. Compile project "
             'manually to set this variable properly.')
        
    cmakeCache = os.path.join(build_root, 'CMakeCache.txt')
    if not os.path.exists(cmakeCache):
        fail(f"{cmakeCache} does not exist. build-root is likely invalid. "
             'Compile project manually to set this variable properly.')
    
    indexed, unindexed = get_file_sets()
    if verbose_mode:
        long_print_iter('Indexed changes', indexed)
        long_print_iter('Unindexed changes', unindexed)
    run_safety_checks(indexed, unindexed)

    undo_formatting = False
    try:
        if len(unindexed) != 0:
            long_print_iter('Unindexed changes found in files', unindexed)
            print('These changes will be rolled back temporarily')

            invoke(*git, 'stash', 'push',
                   '--keep-index',
                   '--include-untracked',
                   '--message', STASH_NAME,
                   result_when_dry='', quiet=False)
            restore = True
        
        format_project()
        undo_formatting = True
        build_project()
        undo_formatting = False
        
    finally:
        if undo_formatting:
            unformat_project(indexed)
        if restore:
            do_restore()
    
    print('Staging formatting changes')
    invoke(*git, 'add', '--', *indexed, result_when_dry='', quiet=False)
            
            
def get_settings_path():
    return os.path.abspath(os.path.join(os.path.dirname(__file__),
                                        SETTINGS_PATH))


def save_settings():
    """Save tools/pre-commit-settings.json."""
    path = get_settings_path()
    verbose(f"Saving settings into {path}")
    if not dry_run:
        with open(path, mode='w') as f:
            json.dump(settings, f, indent=4)
    else:
        verbose('  skipped: --dry-run')


def set_build_root():
    """Set last build root in tools/pre-commit-settings.json."""
    settings['build_root'] = arg_build_root
    save_settings()


def parse_args():
    """Parse sys.argv and environment variables; set corresponding globals.
    Return (action, argument for set-build-root).
    """
    global action
    global verbose_mode
    global dry_run
    global allow_update
    
    consider_options = True
    action = None
    arg_build_root = None
    
    for arg in sys.argv[1:]:
        if arg == 'restore' or arg == 'set-build-root' or arg == 'run':
            if action is not None:
                fail(f"Cannot use '{arg}' and '{action}' together")
            action = arg
        elif consider_options and arg.startswith('-'):
            if arg == '-h' or arg == '--help' or arg == 'help' or arg == '/?':
                print(usage % {'me': my_name})
                sys.exit(0)
            elif arg == '--version':
                print(f"Progressia pre-commit script, version {version}")
                sys.exit(0)
            elif arg == '--verbose':
                verbose_mode = True
            elif arg == '--dry-run':
                dry_run = True
                verbose_mode = True
            elif arg == '--':
                consider_options = False
            else:
                fail(f"Unknown option '{arg}'")
        elif action == 'set-build-root' and arg_build_root is None:
            arg_build_root = arg
        else:
            fail(f"Unknown or unexpected argument '{arg}'")
    
    if not allow_update and action == 'update':
        fail("Cannot use '--dont-update' and 'update' together")
    
    if action is None:
        fail('No action specified')
    
    if action == 'set-build-root' and arg_build_root is None:
        fail('No path given')
    
    return action, arg_build_root


def load_settings():
    """Load values from pre-commit-settings.json."""
    global settings
    global build_root
    global git
    global cmake
    global clang_format_diff
    global parallelism
    
    path = get_settings_path()
    if os.path.exists(path):
        with open(path, mode='r') as f:
            settings = json.load(f)
    else:
        verbose(f"{path} not found, using defaults")
        settings = {
            "__comment": "See `pre-commit.py --help` for documentation",
            "build_root": None,
            "git": None,
            "cmake": None,
            "clang_format_diff": None,
            "parallelism": 1
        }
        save_settings()
    
    build_root = settings['build_root']
    parallelism = settings['parallelism']
    
    def find_command(hints, settings_name):
        if settings[settings_name] is not None:
            hints = [settings[settings_name]]
        
        cmds = (hint.split(';') for hint in hints)
        res = next((cmd for cmd in cmds if shutil.which(cmd[0])), None) \
            or fail(f"Command {hints[0]} not found. Set {settings_name} " +
                    f"in {path} or check PATH")
        
        verbose(f"Found command {hints[0]}:", *(repr(c) for c in res))
        return res
    
    git = find_command(['git'], 'git')
    cmake = find_command(['cmake'], 'cmake')
    clang_format_diff = find_command(['clang-format-diff-13',
                                      'clang-format-diff',
                                      'clang-format-diff.py'],
                                     'clang_format_diff')


if __name__ == '__main__':
    my_name = os.path.basename(sys.argv[0])
    verbose_mode = False
    dry_run = False
    allow_update = True
    
    action, arg_build_root = parse_args()
    load_settings()
    
    if dry_run:
        print('Running in dry mode: no changes to filesystem or git will '
              'actually be performed')
    
    try:
        if action == 'set-build-root':
            set_build_root()
        elif action == 'restore':
            do_restore()
            indexed, unindexed = get_file_sets()
            if indexed & unindexed:
                unformat_project(indexed)
        else:
            pre_commit()
    except subprocess.CalledProcessError as e:
        fail('Command', *(repr(c) for c in e.cmd),
             f"exited with code {e.returncode}")
