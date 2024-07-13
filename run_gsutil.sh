#!/bin/bash

# Check if the file exists
if /usr/bin/gsutil ls gs://caps-signal-bucket/signal.csv; then
    # Get the content of the file and check the number of lines
    lines=$(/usr/bin/gsutil cat gs://caps-signal-bucket/signal.csv | wc -l)
    if [ "$lines" -gt 1 ]; then
        # Print the last line
        /usr/bin/gsutil cat gs://caps-signal-bucket/signal.csv | tail -n 1
    else
        echo "No signals found"
    fi
else
    echo "File does not exist"
fi
