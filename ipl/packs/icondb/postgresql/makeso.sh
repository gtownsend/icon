#!/opt/gnu/bin/bash
CFG=pg_config
bash -c "gcc -shared -o pgsqldb.so -fPIC -I`$CFG --includedir` pgsqldb.c -L`$CFG --libdir` -R`$CFG --libdir` -L/soft/ssl/openssl097-current/lib -R/soft/ssl/openssl097-current/lib -lpq -lssl -lcrypto"
