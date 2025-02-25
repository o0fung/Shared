#!/usr/bin/env python3
from pathlib import Path
import argparse

# Print a tree of a directory path.
#  usage: tree.py [-h] path


class DisplayablePath(object):
    display_filename_prefix_middle = '├──'
    display_filename_prefix_last = '└──'
    display_parent_prefix_middle = '    '
    display_parent_prefix_last = '│   '

    def __init__(self, path, parent_path, is_last):
        self.path = Path(str(path))
        self.parent = parent_path
        self.is_last = is_last
        if self.parent:
            self.depth = self.parent.depth + 1
        else:
            self.depth = 0

    # all directory path would end with a '/'
    @property
    def displayname(self):
        if self.path.is_dir():
            return self.path.name + '/'
        return self.path.name

    # generator of directory tree
    #  each class instance represent one layer of folder
    #  another class instance would be called if more directory is found
    @classmethod
    def make_tree(cls, root, parent=None, is_last=False, criteria=None):
        root = Path(str(root))
        criteria = criteria or cls._default_criteria

        # return the top directory path of the current layer
        displayable_root = cls(root, parent, is_last)
        yield displayable_root

        # map the directory path of the current layer
        children = sorted(list(path
                               for path in root.iterdir()
                               if criteria(path)),
                          key=lambda s: str(s).lower())
        count = 1
        for path in children:
            is_last = count == len(children)
            if path.is_dir():
                # make another tree class instant if new directory is found
                yield from cls.make_tree(path,
                                         parent=displayable_root,
                                         is_last=is_last,
                                         criteria=criteria)
            else:
                # return the path of an item of the current layer
                yield cls(path, displayable_root, is_last)
            count += 1

    @classmethod
    def _default_criteria(cls, path):
        return True

    def displayable(self):
        # the top path is simply printed
        if self.parent is None:
            return self.displayname

        # construct the printed pattern for an item and
        #  the symbol preceding the item in one folder
        _filename_prefix = (self.display_filename_prefix_last
                            if self.is_last
                            else self.display_filename_prefix_middle)
        parts = ['{!s} {!s}'.format(_filename_prefix,
                                    self.displayname)]

        # construct the printed pattern for any number of folder layers
        parent = self.parent
        while parent and parent.parent is not None:
            parts.append(self.display_parent_prefix_middle
                         if parent.is_last
                         else self.display_parent_prefix_last)
            parent = parent.parent

        # the printed pattern was reversed, now output
        return ''.join(reversed(parts))


# parse from command line the target directory path
parser = argparse.ArgumentParser(description='Print a tree of a directory path.')
parser.add_argument('path', help='target directory path to print tree')
arg = vars(parser.parse_args())

# make the directory tree and print out
paths = DisplayablePath.make_tree(Path(arg['path']))
for path in paths:
    txt = path.displayable()
    if '.DS_Store' in txt:
        continue
    print(txt)
