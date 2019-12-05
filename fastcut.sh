#!/bin/bash
fn=$@
w=`ffprobe -v error -select_streams v:0 -show_entries stream=width -of csv=s=x:p=0 $fn`
h=`ffprobe -v error -select_streams v:0 -show_entries stream=height -of csv=s=x:p=0 $fn`
len=`ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 $fn`
ffmpeg -ss 00:00:00 -i $fn -vframes 1 -q:v 1 /tmp/fastcut-frame.jpg -y > /dev/null 2> /dev/null
echo $fn $w $h $len
./fastcut $fn $w $h $len
