# run.sh <name> [hz=0.6] [amp=0.12]: swing the right hand, record 1.5 s, sheet + zoom of 12 distinct frames
cd "$(dirname "$0")"; n="$1"; (python swing2.py 5 hz=${2:-0.6} amp_m=${3:-0.08} turn_rad=0 > swing.log 2>&1 &)
ping -n 3 127.0.0.1 >/dev/null; bash grab.sh $n 1.5 300 560 >/dev/null; python sheet.py $n 1150 100 520 300 10 8 2 0.7
