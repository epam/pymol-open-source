import os
import sys
import re

class DependencyEntry:
    def __init__(self, path):
        self.path = path
        self.exists = os.path.exists(path)
        self.time = os.path.getmtime(path) if self.exists else None
        self.invalid = None
        self.deps = set()

    def depends_on(self, other):
        if not isinstance(other, DependencyEntry):
            raise TypeError('Expected a DependencyEntry')
        self.deps.add(other)

    def is_invalid(self):
        if self.invalid == None:
            self.invalid = (
                # file is missing
                not self.exists or
                # there are dependencies and
                len(self.deps) > 0 and (
                    # one of dependencies is invalid
                    any(dep.is_invalid() for dep in self.deps) or
                    # one of dependencies is newer
                    max(dep.time for dep in self.deps) > self.time
                )
            )
        return self.invalid

class ShowIncludes:
    prefix = 'Note: including file: '
    prefix_length = len(prefix)

    @staticmethod
    def match(line):
        return line.startswith(ShowIncludes.prefix)

    @staticmethod
    def extract(line):
        return os.path.abspath(line[ShowIncludes.prefix_length:].strip())

def strip_prefix(path, prefix):
    if isinstance(prefix, str):
        prefix = re.compile(re.escape(prefix), re.I)
    matched = re.match(prefix, path)
    if matched:
        return path[matched.end(0) + 1:]
    return path

class DependencyTracker:
    def __init__(self, outdir, cc, pp_opts, extra):
        self.root = os.path.dirname(os.path.abspath(__file__))
        self.root_re = re.compile(re.escape(self.root), re.I)
        self.outdir = os.path.abspath(outdir)
        self.cc = cc
        self.pp_opts = pp_opts
        self.extra = extra
        self.entries = {}
        self.sources = {}

    def add(self, obj, src):
        obj = os.path.abspath(obj)
        src = os.path.abspath(src)
        dep = obj + '.dep'
        self.sources[obj] = (src, dep)

        # expect it only once
        paths = (obj, src, dep)
        for path in paths:
            if path in self.entries:
                raise AssertionError('Already in dependency tracker: ' + path)
        
        # add files as entries
        entries = map(DependencyEntry, paths)
        for entry in entries:
            self.entries[entry.path] = entry

        # wire basic dependencies
        obj_entry, src_entry, dep_entry = entries
        obj_entry.depends_on(dep_entry)
        obj_entry.depends_on(src_entry)
        dep_entry.depends_on(src_entry)

        # read a list of dependencies
        if dep_entry.exists:
            with open(dep_entry.path) as f:
                for header in f.read().splitlines():
                    header = os.path.abspath(header)
                    if header not in self.entries:
                        self.entries[header] = DependencyEntry(header)
                    dep_entry.depends_on(self.entries[header])

    def is_invalid(self, path):
        path = os.path.abspath(path)
        entry = self.entries[path]
        return entry.is_invalid()

    def rebuild_deps(self, obj):
        obj = os.path.abspath(obj)
        src, dep = self.sources[obj]
        print 'Rebuilding deps for', strip_prefix(obj, self.outdir)

        # execute preprocessor on the source file
        import subprocess
        with open(os.devnull, 'wb') as devnull:
            proc = subprocess.Popen(
                [self.cc, '/E', src, '/showIncludes'] + self.pp_opts + self.extra,
                stdout=devnull, stderr=subprocess.PIPE)
            _, stderr = proc.communicate()

        # parse a printed list of includes
        includes = set(
            map(ShowIncludes.extract,
                filter(ShowIncludes.match, stderr.splitlines())
            )
        )

        # ignore system includes
        filtered_includes = set(
            map(lambda path: strip_prefix(path, self.root_re),
                filter(lambda path: re.match(self.root_re, path), includes)
            )
        )

        # save them to a dependency list file
        with open(dep, 'w') as f:
            f.write('\n'.join(sorted(filtered_includes)))

        # save raw stderr for debugging
        # with open(dep + '.raw', 'w') as f:
        #     f.write('root: ' + self.root + '\n\n\n')
        #     f.write('\n'.join(sorted(includes)) + '\n\n\n')
        #     f.write(stderr)

    def dump(self):
        with open(os.path.join(self.outdir, 'DependencyTracker.log'), 'w') as f:
            for key in sorted(self.entries.keys()):
                entry = self.entries[key]
                f.write('%s :: %s :: %s\n' % (entry.path, entry.time, entry.is_invalid()))
