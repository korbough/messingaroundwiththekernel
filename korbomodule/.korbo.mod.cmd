savedcmd_/home/korbo/kernelpanic/korbo.mod := printf '%s\n'   korbo.o | awk '!x[$$0]++ { print("/home/korbo/kernelpanic/"$$0) }' > /home/korbo/kernelpanic/korbo.mod
