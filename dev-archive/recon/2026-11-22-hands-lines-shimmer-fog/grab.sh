# grab.sh <name> [seconds=1] [y0=500] [h=520]: record a strip of the screen (the simulator window, both eyes) at 60 a second, lossless; frames into clips/<name>/
n="$1"; t="${2:-1}"; y="${3:-500}"; h="${4:-520}"; d="$(dirname "$0")/clips/$n"; rm -rf "$d"; mkdir -p "$d"
ffmpeg -y -loglevel error -f gdigrab -framerate 60 -draw_mouse 0 -offset_x 0 -offset_y $y -video_size 3440x$h -t $t -i desktop -c:v libx264rgb -qp 0 -preset ultrafast "$d/clip.mkv"
ffmpeg -y -loglevel error -i "$d/clip.mkv" "$d/%03d.png"; ls "$d" | wc -l
