#!bash

START=$(date +%s);

./a

# just a test content
END=$(date +%s);
echo $((END-START)) | awk '{print int($1/60)":"int($1%60)}'