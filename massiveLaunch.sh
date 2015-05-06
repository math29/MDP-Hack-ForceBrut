#!/bin/sh

PORT=${1}
SERVER=`hostname`
HERE=`pwd`
COMMAND="./multiClient.sh ./decryptClient ${SERVER} ${PORT}"

SSH_OPTS="-o ConnectTimeout=1"
SSH_OPTS="${SSH_OPTS} -o PasswordAuthentication=no"
SSH_OPTS="${SSH_OPTS} -o PubkeyAuthentication=yes"
SSH_OPTS="${SSH_OPTS} -o CheckHostIP=no"
SSH_OPTS="${SSH_OPTS} -o StrictHostKeyChecking=no"

cat enib_hosts.txt | while read HOST comment ; do
  case "${HOST}" in
    "")
      continue # ignore empty lines
      ;;
    "#"*)
      continue # ignore comments
      ;;
  esac
  echo "---- ${HOST} ----"
  ssh ${SSH_OPTS} ${HOST} \
      "cd ${HERE} ; ${COMMAND} >/dev/null 2>&1" </dev/null &
done

