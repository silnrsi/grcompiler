#!/bin/sed -rf

# Dump comments
\%//% d

# Single line invocations of Add{Error,Warning}
/g_errorList.Add\w+\([^)]+\)/ b munge

# Multiline invocations of the same
/g_errorList.Add\w+\(/,/\)/ {
    H
    d
}

# Everything not a multiline invocation
//,// ! {
    g
    /^$/ d          # If no multiline waiting in hold buffer delete
    s/\s*\n\s*/ /g
    x;z;x           # Clear hold buffer
    b munge
}

# Extract parameters of interest
:munge
s/(\s+,|,\s+)/,/g
s/","/ /g
s/.*Add(\w+)\(([[:digit:]]+),[^,]+,(.+)\).*/\1\t\2\t\3/

# Remove any remining lines
/g_errorList/ d

# Fixup strings
:polish
s/",/" + /g
s/,"/ + "/g
