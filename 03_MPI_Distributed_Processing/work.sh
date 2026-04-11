for i in $(seq -w 1 30); do
  host="linux$i"
  printf "%-8s " "$host"
  ssh -o ConnectTimeout=1 -o BatchMode=yes "$host" \
    "uptime | sed -n 's/.*load average: //p'" 2>/dev/null || echo "down"
done
