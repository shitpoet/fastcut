#!/bin/bash
fn=$@

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

w=`ffprobe -v error -select_streams v:0 -show_entries stream=width -of csv=s=x:p=0 $fn`
h=`ffprobe -v error -select_streams v:0 -show_entries stream=height -of csv=s=x:p=0 $fn`
len=`ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 $fn`
ffmpeg -ss 00:00:00 -i $fn -vframes 1 -q:v 1 /tmp/fastcut-frame.jpg -y > /dev/null 2> /dev/null
echo $fn $w $h $len
$DIR/fastcut $fn $w $h $len
