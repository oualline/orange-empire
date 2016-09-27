# Script to set the initial passwords of the various accounts
# Contains fake passwords

chpasswd <<EOF
docent:secret
reboot:secret
date:secret
pi:secret
garden:secret
root:secret
EOF
