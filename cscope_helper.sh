#!/bin/bash

# List all header and C source files
find . -name \*.h -print -o -name \*.c -print > cscope.files

# Run cscope with the new file (assuming
# 'cscope' is defined in the env path.
cscope -b -q

# Let's just trust all went well
echo "cscope.files updated. Emacs can now do some work for you."

# References:
# http://cscope.sourceforge.net/large_projects.html
# https://techtooltip.wordpress.com/2012/01/06/how-to-integrate-emacs-cscope-to-browse-linux-kernel-source-code/
