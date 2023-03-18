#!bash

START=$(date +%s);

./a

END=$(date +%s);
echo $((END-START)) | awk '{print int($1/60)":"int($1%60)}'