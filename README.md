# MDP-Hack-ForceBrut

Utilisation SSH :
ls -l ~/.ssh/
Si pas de id.dsa ou id.rsa faire :
ssh-keygen

cd ~/.ssh/
cp -ai id_rsa.pub authorized_keys
ssh-add
ssh destination
