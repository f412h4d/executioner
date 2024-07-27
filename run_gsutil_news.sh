#!/bin/bash

# Check if the file exists
if /usr/bin/gsutil ls gs://caps-signal-bucket/signal.csv > /dev/null 2>&1; then
    lines=$(/usr/bin/gsutil cat gs://caps-signal-bucket/news.csv | wc -l)
    if [ "$lines" -gt 1 ]; then
        /usr/bin/gsutil cat gs://caps-signal-bucket/news.csv
    else
        echo "No signals found"
    fi
else
    echo "File does not exist"
fi
