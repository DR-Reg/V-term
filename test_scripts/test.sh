printf "\33[?1049h"
printf "\33[H"
for f in {0..7}
do
  for b in {0..7}
  do
    if [[ $f == $b ]]
      then
        continue
    fi
    printf "\33[3${f};4${b}m"
    printf "ESC[3${f};4${b}m"
    printf "%0.s-" {1..30}
    printf "\33[0m\n"
  done
done
read
printf "\33[?1049l"
